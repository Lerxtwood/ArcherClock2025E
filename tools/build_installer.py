#!/usr/bin/env python3
"""Mirror GitHub Release images into a same-origin GitHub Pages installer."""

import argparse
import json
import os
from pathlib import Path
import shutil
import urllib.parse
import urllib.request

FACTORY_IMAGE = "archer-clock-factory.bin"
OTA_IMAGE = "archer-clock-ota.bin"


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
        manifest = {
            "name": "Archer Clock 2025E",
            "version": tag.removeprefix("v"),
            "new_install_prompt_erase": True,
            "new_install_improv_wait_time": 0,
            "builds": [{"chipFamily": "ESP32", "improv": False,
                        "parts": [{"path": FACTORY_IMAGE, "offset": 0}]}],
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
