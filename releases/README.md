# Build 0.4

[Amazing-Alex-Vita-v0.4.vpk](Amazing-Alex-Vita-v0.4.vpk) contains the Vita loader
and finished launcher artwork. It does not include the Android game library
or playable game data. See the [installation instructions](../README.md#first-installation).

- Touchscreen gameplay, with Start/Circle mapped to Back.
- Menu and first-level gameplay tested on a physical Vita.
- Double-buffered audio; clean music confirmed in the menu and a level.
- Offline HTTP failure handling and disabled browser/analytics integration.
- Custom bubble icon, LiveArea, launch tile and splash.

This is an early build with bounded diagnostics enabled. The VPK's system
metadata remains version `01.00`; `0.4` is the port's development build number.
Full-game completion and suspend/resume are not verified.

`SHA256SUMS` records the installer checksum for download verification on a PC.
It does not add a checksum step to game startup.
