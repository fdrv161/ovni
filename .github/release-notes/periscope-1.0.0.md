**Periscope** is the TELESCOPE 0.2.0 audio analyser by OVNI Audio with speech and full keyboard control. Until this release it was called TELESCOPE Accessible.

## Download

- **Windows**: `Periscope-windows-vst3.zip`, VST3 for Windows 10 and 11, 64-bit. Extract it and copy the `Periscope.vst3` folder into `C:\Program Files\Common Files\VST3` (or another folder your DAW scans), then rescan plugins.
- **macOS, experimental**: `Periscope-macos.zip`, VST3 and AU, Apple Silicon and Intel. Speech has only been tested with NVDA on Windows. The build is not notarized: see README.txt inside or the manual for the one Terminal command it needs.

Each ZIP contains the manual in English, Russian and Spanish.

## What's new

- **New name.** In the DAW the plugin is now "Periscope (fdrv161)", on disk `Periscope.vst3` (and `Periscope.component` on macOS). The plugin codes did not change, so your projects and settings keep working.
- **Updating from TELESCOPE Accessible**: delete the old `TELESCOPE Accessible.vst3` folder and copy `Periscope.vst3` in its place.
- Version numbers start again at 1.0.0. This release also has the last fixes of TELESCOPE Accessible: REAPER no longer stays in the background after you close it, background threads are never killed, and removing or moving the plugin while a file is being analysed is safe.

Tested with NVDA in REAPER on Windows.

## Manual

[MANUAL.md](https://github.com/fdrv161/periscope/blob/periscope-1.0.0/MANUAL.md), also in [Russian](https://github.com/fdrv161/periscope/blob/periscope-1.0.0/MANUAL.ru.md) and [Spanish](https://github.com/fdrv161/periscope/blob/periscope-1.0.0/MANUAL.es.md): installing, first steps, all keys, and every lens explained.

## Licence and source

GNU AGPLv3. TELESCOPE is by [OVNI Audio](https://ovniaudio.com/telescope); Periscope is based on its AGPLv3 release 0.2.0. The full source of this release is at the tag [`periscope-1.0.0`](https://github.com/fdrv161/periscope/tree/periscope-1.0.0).

**How this was made.** The speech and keyboard layer, its translations, the Russian translation, the fixes, the build scripts and this documentation were written by Claude, an AI model by Anthropic, working in Claude Code at the request of [fdrv161](https://github.com/fdrv161). fdrv161 tested the Windows build by hand with NVDA in REAPER. The macOS build has not been tested by a person.

`SHA256SUMS.txt` holds the checksums of the ZIPs.
