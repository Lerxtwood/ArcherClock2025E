# Archer Clock 2025E Firmware

## Project Overview
**Project**: Clock with hypnotic motion and time synchronization via Wi-Fi and NTP.  
**Author**: Archer  
**Date**: 25/09/2024

## Description
This clock is inspired by the light effects of the **Corpus Clock**, located on the campus of the University of Cambridge.

## Features
- Hypnotic LED movement of the hands.
- Time synchronization via Wi-Fi to NTP (Network Time Protocol) server.
- Browser-based first-boot Wi-Fi configuration page.
- Arduino OTA firmware update support.

## First Boot Setup
If no Wi-Fi credentials are saved, or the saved network cannot be reached, the clock starts a setup access point named `ArcherClock-XXXXXX`. Connect to that network and open `http://192.168.4.1/` to save the Wi-Fi name and password. The clock stores those credentials in ESP32 non-volatile storage and restarts.

The setup page also has a firmware update link. Use it to upload a local PlatformIO firmware binary such as `.pio/build/wemos_d1_mini32/firmware.bin`.

With OTA enabled, the clock keeps Wi-Fi connected after NTP sync and advertises itself with the hostname configured in `src/config.hpp`.

The Settings page includes a Brightness section below Themes. Global LED brightness defaults to `255`, and marker LED brightness defaults to `128`; both accept whole numbers from `0` to `255`. Select a theme, adjust its brightness, and save display settings to remember those values across restarts. Selecting that theme again restores its saved brightness, which also applies when the theme runs on a schedule.

### Time zone and daylight saving

In **Settings → Time Zone**, choose your zone (for example, `Europe/Paris`) and select **Save time settings**. The page suggests your phone or computer's time zone when the clock has no saved choice. Use **Use this device's time zone** to detect it again or choose a zone manually. Saving applies the setting and keeps it across restarts. Clocks without a saved choice continue to use `America/Denver` until a zone is saved.

**Automatic** follows the selected zone's daylight saving dates. **Standard time** and **Daylight time** use the zone's corresponding seasonal offset year-round. Zones with a fixed offset use that offset in all three modes.

The firmware bundles IANA time zone data for 2024–2099 and applies it locally after synchronizing UTC over Wi-Fi. If a jurisdiction changes its rules, update the bundled data by running `python tools/build_time_zones.py path/to/tzdata.whl` with an updated tzdata wheel, then rebuild the firmware.

## Browser Firmware Installer

At startup, the clock displays its installed version for five seconds before Wi-Fi synchronization: blue hour markers show the major version, amber minute markers show the minor version, and green second markers show the patch version. An unlit ring means zero. For v0.9.3, the hour ring is dark, nine minute markers are amber, and three second markers are green. Each ring can display up to twelve markers. The display uses a fixed brightness of 128, then restores the saved brightness.

[Open the hosted firmware installer](https://lerxtwood.github.io/ArcherClock2025E/) · [GitHub Releases](https://github.com/Lerxtwood/ArcherClock2025E/releases)

The installer in `web/flash/` uses ESP Web Tools to install firmware over USB on the WEMOS D1 Mini ESP32. It lists up to 12 compatible GitHub Releases, defaults to the newest stable version, and labels prereleases. Install with a USB data cable using desktop Chrome or Edge over HTTPS (or localhost).

The release-notes panel updates with the selected firmware version. Notes are copied from GitHub Releases during site deployment and displayed on the installer page, with a link to the original release. Releases without notes show an explicit empty state.

The installer workflow copies full-install images from GitHub Releases into the Pages site. Manifests and firmware are served from the same origin, so browser flashing does not depend on cross-origin GitHub asset downloads. Older releases without `archer-clock-factory.bin` are excluded. Repository links and release discovery use the repository running the workflow, so forks need no hardcoded owner changes.

For browser installs, the workflow splits each factory image into bootloader, partition table, OTA initialization, and application files. The manifest writes only those sections and leaves the ESP32 NVS settings partition (`0x9000`–`0xDFFF`) untouched. Keep **Erase device** unchecked to retain Wi-Fi credentials, theme brightness, and all saved display and schedule settings. Selecting **Erase device** clears them. The workflow validates the partition layout and refuses images whose layout would make preservation unsafe.

[v0.9.1](https://github.com/Lerxtwood/ArcherClock2025E/releases/tag/v0.9.1) also removes the repeated animation timing numbers from the serial console. Select this release in the refreshed installer to test both fixes. The updated flash layout also preserves settings when installing v0.9.0; settings erased by an earlier installation must be entered again.

### Enable hosting and publish firmware

1. Push the installer files and workflows to the repository's default branch.
2. In GitHub **Settings → Pages → Build and deployment**, select **GitHub Actions**. Allow release tags to deploy in the `github-pages` environment if its deployment protection rules restrict refs.
3. Push a version tag, for example `git tag v0.9.0` followed by `git push <remote> v0.9.0`. The **Release firmware** workflow builds the tagged source, publishes release assets, and calls **Deploy firmware installer**. Tags containing a hyphen, such as `v0.9.0-rc1`, create prereleases.
4. Open the Pages URL shown in the deployment. It normally follows `https://<owner>.github.io/<repository>/`.

The installer also refreshes when releases are published, edited, or deleted, and when its source changes on the default branch. Run **Deploy firmware installer** manually to refresh the list. A repository with no compatible releases displays an empty state with installation disabled.

Release assets:

| File | Purpose |
| --- | --- |
| `archer-clock-factory.bin` | Full image for manual flashing at offset `0x0`. Writing it directly also overwrites settings; use the browser installer to preserve them. |
| `archer-clock-ota.bin` | Application-only image for the clock's existing Firmware page. |
| `SHA256SUMS` | SHA-256 checksums of both images. |

During USB installation, choose whether to erase saved settings. Erasing removes saved Wi-Fi and theme settings. After a fresh install, join `ArcherClock-XXXXXX` and open `http://192.168.4.1/` to configure Wi-Fi.

### Local checks

Run `python -m unittest discover -s tools/tests -v` to test release filtering, image validation, flash manifests, safe version paths, and settings preservation with and without device erasure.

To prepare a real installer locally, copy `web/flash/` into a temporary site directory, then run `python tools/build_installer.py --site <site-directory> --repository <owner>/<repository>`. Set `GITHUB_TOKEN` in the environment if needed for API limits. Serve that directory with `python -m http.server 8000 --directory <site-directory>` and open `http://localhost:8000/`.

## 3D Model
https://makerworld.com/en/models/901892#profileId-861085

## License
GNU GENERAL PUBLIC LICENSE Version 3
