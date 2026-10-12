The third release of **TELESCOPE Accessible**: the TELESCOPE 0.2.0 audio analyser by OVNI Audio, usable with a screen reader and the keyboard.

## Download

- **Windows**: `TELESCOPE-accessible-windows-vst3.zip`, VST3 for Windows 10 and 11, 64-bit. Extract it and copy the `TELESCOPE Accessible.vst3` folder into `C:\Program Files\Common Files\VST3` (or another folder your DAW scans), then rescan plugins. Updating from an older version: see below.
- **macOS, experimental**: `TELESCOPE-accessible-macos.zip`, VST3 and AU, Apple Silicon and Intel. The accessibility layer has only been tested with NVDA on Windows. The build is not notarized: see README.txt inside or the manual for the one Terminal command it needs.

Each ZIP contains the manual in English, Russian and Spanish.

## What's new since 0.2.0-a11y.2

- **Its own name in the DAW**: the plugin is now listed as "TELESCOPE Accessible (fdrv161)" and installs as `TELESCOPE Accessible.vst3`, so it can sit next to the original TELESCOPE by OVNI.
- **Updating**: delete the old `TELESCOPE.vst3` folder unless it is the original plugin you also use. In projects that used an older version, insert TELESCOPE Accessible again: the old settings do not carry over. Saved user presets stay available.

For everything that changed in 0.2.0-a11y.2, see [its release notes](https://github.com/fdrv161/telescope-accessible/releases/tag/telescope-accessible-0.2.0-a11y.2).

Tested with NVDA in REAPER on Windows.

## Manual

[MANUAL.md](https://github.com/fdrv161/telescope-accessible/blob/telescope-accessible-0.2.0-a11y.3/MANUAL.md), also in [Russian](https://github.com/fdrv161/telescope-accessible/blob/telescope-accessible-0.2.0-a11y.3/MANUAL.ru.md) and [Spanish](https://github.com/fdrv161/telescope-accessible/blob/telescope-accessible-0.2.0-a11y.3/MANUAL.es.md): installing, first steps, all keys, and every lens explained.

## Licence and source

GNU AGPLv3. TELESCOPE is by [OVNI Audio](https://github.com/ovniaudio/ovni); the accessibility layer was added in this fork. The full source of this release is at the tag [`telescope-accessible-0.2.0-a11y.3`](https://github.com/fdrv161/telescope-accessible/tree/telescope-accessible-0.2.0-a11y.3).

**How this was made.** The accessibility layer, the speech translations, the Russian translation, the build scripts and this documentation were written by Claude, an AI model by Anthropic, working in Claude Code at the request of [fdrv161](https://github.com/fdrv161). fdrv161 tested the Windows build by hand with NVDA in REAPER. The macOS build has not been tested by a person.

`SHA256SUMS.txt` holds the checksums of the ZIPs.
