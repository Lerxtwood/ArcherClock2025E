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

## 3D Model
https://makerworld.com/en/models/901892#profileId-861085

## License
GNU GENERAL PUBLIC LICENSE Version 3
