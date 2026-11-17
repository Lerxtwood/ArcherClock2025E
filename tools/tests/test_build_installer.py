import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from urllib.parse import unquote

spec = importlib.util.spec_from_file_location("installer", Path(__file__).parents[1] / "build_installer.py")
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)


def release(tag, prerelease=False, factory=True, draft=False):
    return {
        "tag_name": tag, "name": tag, "prerelease": prerelease, "draft": draft,
        "html_url": "https://github.com/example/clock/releases/tag/" + tag,
        "assets": [{"name": installer.FACTORY_IMAGE if factory else installer.OTA_IMAGE,
                    "browser_download_url": "https://example.com/image.bin", "size": 4}],
    }


class InstallerTests(unittest.TestCase):
    def build(self, site, releases, data=b"test", limit=12):
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
            self.assertEqual(manifest["builds"][0]["parts"], [{"path": installer.FACTORY_IMAGE, "offset": 0}])
            self.assertTrue(manifest["new_install_prompt_erase"])
            self.assertEqual(manifest["new_install_improv_wait_time"], 0)
            self.assertEqual(json.loads((site / "release-index.json").read_text()), index)

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


if __name__ == "__main__":
    unittest.main()
