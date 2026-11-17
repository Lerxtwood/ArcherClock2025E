#!/usr/bin/env python3
"""Mirror GitHub Release images into a same-origin GitHub Pages installer."""

import argparse
import json
import os
from pathlib import Path
import shutil
import struct
import urllib.parse
import urllib.request

FACTORY_IMAGE = "archer-clock-factory.bin"
OTA_IMAGE = "archer-clock-ota.bin"
NVS_START = 0x9000
NVS_END = 0xE000
# Keep every write sector-aligned and leave the Preferences partition untouched.
FLASH_PARTS = (
    ("bootloader.bin", 0x1000, 0x8000),
    ("partitions.bin", 0x8000, NVS_START),
    ("boot_app0.bin", NVS_END, 0x10000),
    ("firmware.bin", 0x10000, None),
)


def split_factory_image(image, directory):
    """Extract flash parts without the merged image's destructive NVS padding.

    These offsets match this project's default ESP32 partition layout. Refuse a
    changed layout rather than silently overwriting or relocating saved settings.
    """
    data = image.read_bytes()
    if len(data) <= 0x10000 or data[0x1000] != 0xE9 or data[0x10000] != 0xE9:
        raise ValueError("Invalid ESP32 factory image")
    partitions = []
    for address in range(0x8000, 0x8C00, 32):
        magic, kind, subtype, offset, size, label, flags = struct.unpack_from("<HBBII16sI", data, address)
        if magic in (0xFFFF, 0xEBEB):  # End marker or partition-table MD5 record.
            break
        if magic != 0x50AA:
            raise ValueError("Invalid ESP32 partition table")
        partitions.append((kind, subtype, offset, size, label.rstrip(b"\0")))
    nvs = [entry for entry in partitions if entry[:2] == (1, 2)]
    ota = [entry for entry in partitions if entry[:2] == (1, 0)]
    app = [entry for entry in partitions if entry[:2] == (0, 0x10)]
    if (len(nvs) != 1 or nvs[0][2:] != (NVS_START, NVS_END - NVS_START, b"nvs") or
            len(ota) != 1 or ota[0][2:4] != (NVS_END, 0x2000) or
            len(app) != 1 or app[0][2] != 0x10000 or len(data) > app[0][2] + app[0][3]):
        raise ValueError("Unsupported partition layout; cannot guarantee settings preservation")
    parts = []
    for filename, start, end in FLASH_PARTS:
        (directory / filename).write_bytes(data[start:end])
        parts.append({"path": filename, "offset": start})
    return parts


def request_json(url):
    headers = {"Accept": "application/vnd.github+json", "User-Agent": "ArcherClock-installer"}
    if os.environ.get("GITHUB_TOKEN"):
        headers["Authorization"] = "Bearer " + os.environ["GITHUB_TOKEN"]
    with urllib.request.urlopen(urllib.request.Request(url, headers=headers), timeout=45) as response:
        return json.load(response)


def download(url, destination):
    # Release downloads are public. Never forward the API token to asset hosts.
    with urllib.request.urlopen(url, timeout=60) as response, destination.open("wb") as output:
        shutil.copyfileobj(response, output)


def build_site(site, repository, limit=12):
    releases = request_json(f"https://api.github.com/repos/{repository}/releases?per_page=100")
    entries = []
    for release in releases:
        if release.get("draft"):
            continue
        assets = {asset["name"]: asset for asset in release.get("assets", [])}
        if FACTORY_IMAGE not in assets:
            continue  # Application-only releases cannot initialize a new board.
        tag = release["tag_name"]
        # Encode the complete tag as one directory, including tags containing '/'.
        directory = "release-" + urllib.parse.quote(tag, safe="")
        version_dir = site / "releases" / directory
        version_dir.mkdir(parents=True, exist_ok=True)
        image = version_dir / FACTORY_IMAGE
        asset = assets[FACTORY_IMAGE]
        download(asset["browser_download_url"], image)
        if image.stat().st_size != asset["size"] or image.stat().st_size == 0:
            raise ValueError(f"Incomplete factory image for {tag}")
        parts = split_factory_image(image, version_dir)
        manifest = {
            "name": "Archer Clock 2025E",
            "version": tag.removeprefix("v"),
            "new_install_prompt_erase": True,
            "new_install_improv_wait_time": 0,
            "builds": [{"chipFamily": "ESP32", "improv": False,
                        "parts": parts}],
        }
        (version_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        # Encode '%' again for URL paths so encoded tag characters remain literal filenames.
        path = "releases/" + urllib.parse.quote(directory, safe="")
        entries.append({
            "tag": tag, "name": release.get("name") or tag,
            "prerelease": bool(release.get("prerelease")),
            "published": release.get("published_at"),
            "url": release["html_url"],
            "manifest": path + "/manifest.json",
            "factory": path + "/" + FACTORY_IMAGE,
            "ota": assets.get(OTA_IMAGE, {}).get("browser_download_url"),
        })
        if len(entries) >= limit:
            break
    # GitHub returns newest first. Prefer the newest stable over a newer prerelease.
    entries.sort(key=lambda entry: entry["prerelease"])
    index = {"repository": repository, "releases": entries}
    site.mkdir(parents=True, exist_ok=True)
    (site / "release-index.json").write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")
    (site / ".nojekyll").touch()
    print(f"Prepared {len(entries)} installable releases from {repository}")
    return index


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--site", type=Path, default=Path("site"))
    parser.add_argument("--repository", default=os.environ.get("GITHUB_REPOSITORY"))
    parser.add_argument("--limit", type=int, default=12)
    args = parser.parse_args()
    if not args.repository or args.limit < 1:
        parser.error("--repository (or GITHUB_REPOSITORY) and a positive --limit are required")
    build_site(args.site, args.repository, args.limit)
