import importlib.util
import json
from pathlib import Path
import tempfile
import struct
import unittest
from unittest.mock import patch
from urllib.parse import unquote

spec = importlib.util.spec_from_file_location("installer", Path(__file__).parents[1] / "build_installer.py")
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)


def factory_image(nvs_offset=0x9000, app_size=0x140000):
    data = bytearray(b"\xff" * 0x10020)
    data[0x1000] = data[0x10000] = 0xE9
    for i, entry in enumerate(((1, 2, nvs_offset, 0x5000, b"nvs"),
                               (1, 0, 0xE000, 0x2000, b"otadata"),
                               (0, 0x10, 0x10000, app_size, b"app0"))):
        struct.pack_into("<HBBII16sI", data, 0x8000 + i * 32, 0x50AA, *entry, 0)
    return bytes(data)


def release(tag, prerelease=False, factory=True, draft=False):
    return {
        "tag_name": tag, "name": tag, "prerelease": prerelease, "draft": draft,
        "html_url": "https://github.com/example/clock/releases/tag/" + tag,
        "assets": [{"name": installer.FACTORY_IMAGE if factory else installer.OTA_IMAGE,
                    "browser_download_url": "https://example.com/image.bin", "size": len(factory_image())}],
    }


class InstallerTests(unittest.TestCase):
    def build(self, site, releases, data=None, limit=12):
        if data is None:
            data = factory_image()
        with patch.object(installer, "request_json", return_value=releases), patch.object(
            installer, "download", side_effect=lambda url, path: path.write_bytes(data)
        ):
            return installer.build_site(site, "example/clock", limit)

    def test_filters_and_prefers_stable_with_correct_flash_offset(self):
        with tempfile.TemporaryDirectory() as directory:
            site = Path(directory)
            index = self.build(site, [release("v2-rc1", True), release("v1"),
                                      release("old-app-only", factory=False), release("draft", draft=True)])
            self.assertEqual([r["tag"] for r in index["releases"]], ["v1", "v2-rc1"])
            selected = index["releases"][0]
            manifest = json.loads((site / selected["manifest"]).read_text())
            self.assertEqual(manifest["builds"][0]["chipFamily"], "ESP32")
            self.assertEqual([part["offset"] for part in manifest["builds"][0]["parts"]],
                             [0x1000, 0x8000, 0xE000, 0x10000])
            self.assertTrue(manifest["new_install_prompt_erase"])
            self.assertEqual(manifest["new_install_improv_wait_time"], 0)
            self.assertEqual(json.loads((site / "release-index.json").read_text()), index)

    def test_update_preserves_settings_unless_device_is_erased(self):
        with tempfile.TemporaryDirectory() as directory:
            site = Path(directory)
            entry = self.build(site, [release("v1")])["releases"][0]
            manifest_path = site / entry["manifest"]
            manifest = json.loads(manifest_path.read_text())
            original = factory_image()
            saved_settings = bytes(range(256)) * 80  # Entire 20 KiB NVS region.
            for erase_device in (False, True):
                flash = bytearray(b"\xff" * 0x20000)
                flash[0x9000:0xE000] = saved_settings
                if erase_device:
                    flash[:] = b"\xff" * len(flash)
                for part in manifest["builds"][0]["parts"]:
                    start = part["offset"]
                    data = (manifest_path.parent / part["path"]).read_bytes()
                    end = start + len(data)
                    erase_start = start // 4096 * 4096
                    erase_end = (end + 4095) // 4096 * 4096
                    self.assertTrue(erase_end <= 0x9000 or erase_start >= 0xE000)
                    # Simulate sector erasure as well as writing the image bytes.
                    flash[erase_start:erase_end] = b"\xff" * (erase_end - erase_start)
                    flash[start:end] = data
                    self.assertEqual(data, original[start:end])
                expected = b"\xff" * 0x5000 if erase_device else saved_settings
                self.assertEqual(flash[0x9000:0xE000], expected)
                self.assertEqual(flash[0x10000:len(original)], original[0x10000:])

    def test_changed_partition_layout_stops_publication(self):
        with tempfile.TemporaryDirectory() as directory:
            site = Path(directory)
            with self.assertRaisesRegex(ValueError, "Unsupported partition layout"):
                self.build(site, [release("v1")], data=factory_image(nvs_offset=0xA000))
            self.assertFalse((site / "release-index.json").exists())

    def test_application_overflow_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, "Unsupported partition layout"):
                self.build(Path(directory), [release("v1")], data=factory_image(app_size=0x10))

    def test_invalid_factory_image_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            data = bytearray(factory_image())
            data[0x1000] = 0
            with self.assertRaisesRegex(ValueError, "Invalid ESP32 factory image"):
                self.build(Path(directory), [release("v1")], data=bytes(data))

    def test_empty_repository_produces_empty_state(self):
        with tempfile.TemporaryDirectory() as directory:
            self.assertEqual(self.build(Path(directory), [])["releases"], [])

    def test_tag_path_cannot_escape_site_and_urls_resolve(self):
        with tempfile.TemporaryDirectory() as directory:
            site = Path(directory)
            entry = self.build(site, [release("../../v1")])["releases"][0]
            manifest = (site / unquote(entry["manifest"])).resolve()
            self.assertTrue(manifest.is_relative_to(site.resolve()))
            self.assertTrue(manifest.is_file())
            self.assertTrue((site / unquote(entry["factory"])).is_file())

    def test_truncated_download_stops_publication(self):
        with tempfile.TemporaryDirectory() as directory:
            site = Path(directory)
            with self.assertRaisesRegex(ValueError, "Incomplete factory image"):
                self.build(site, [release("v1")], data=b"bad")
            self.assertFalse((site / "release-index.json").exists())

    def test_release_limit(self):
        with tempfile.TemporaryDirectory() as directory:
            index = self.build(Path(directory), [release("v3"), release("v2"), release("v1")], limit=2)
            self.assertEqual(len(index["releases"]), 2)

    def test_notes_stay_with_their_selected_release(self):
        with tempfile.TemporaryDirectory() as directory:
            beta = release("v2-rc1", prerelease=True)
            beta["body"] = "### Preview\n\n- Test the new display."
            stable = release("v1")
            stable["body"] = "### Stable\n\nKeep **Erase device** unchecked."
            empty = release("v0")
            empty["body"] = None
            index = self.build(Path(directory), [beta, stable, empty])
            self.assertEqual([(entry["tag"], entry["notes"]) for entry in index["releases"]],
                             [("v1", stable["body"]), ("v0", ""), ("v2-rc1", beta["body"])])


if __name__ == "__main__":
    unittest.main()
