The fourth release of **TELESCOPE Accessible**: the TELESCOPE 0.2.0 audio analyser by OVNI Audio, usable with a screen reader and the keyboard.

## Download

- **Windows**: `TELESCOPE-accessible-windows-vst3.zip`, VST3 for Windows 10 and 11, 64-bit. Extract it and copy the `TELESCOPE Accessible.vst3` folder into `C:\Program Files\Common Files\VST3` (or another folder your DAW scans), replacing the old one, then rescan plugins. Projects and settings from 0.2.0-a11y.3 keep working.
- **macOS, experimental**: `TELESCOPE-accessible-macos.zip`, VST3 and AU, Apple Silicon and Intel. The accessibility layer has only been tested with NVDA on Windows. The build is not notarized: see README.txt inside or the manual for the one Terminal command it needs.

Each ZIP contains the manual in English, Russian and Spanish.

## What's new since 0.2.0-a11y.3

- **The DAW no longer stays running after you close it.** Once TELESCOPE had been added to a project, even if it was removed again, REAPER kept running in the background at full load on one CPU core after its window closed. Two images of the interface background lived until the plugin was unloaded, and freeing their video memory at that moment made the graphics driver wait forever (seen with an AMD driver on Windows). They are now freed when the plugin window closes.
- **Background threads are never killed.** Before, when the analysis, file analysis or reference waveform thread took longer than 2 seconds to stop, it was killed by force. A killed thread can leave a lock taken inside the DAW, which can keep the DAW running after it is closed or crash it. This is the same hang OVNI Audio describes in TELESCOPE 0.3. Now the plugin asks the thread to stop and waits until it does.
- **Removing or moving the plugin while a file is being analysed** (the Tonal Balance reference or the Verdict file) is safe: the file analyzers are stopped before the engine they report to.
- The reference waveform in Tonal Balance is read at normal priority, so a busy DAW cannot hold it back when the window closes.

For everything that changed in 0.2.0-a11y.3, see [its release notes](https://github.com/fdrv161/telescope-accessible/releases/tag/telescope-accessible-0.2.0-a11y.3).

Tested with NVDA in REAPER on Windows.

## Manual

[MANUAL.md](https://github.com/fdrv161/telescope-accessible/blob/telescope-accessible-0.2.0-a11y.4/MANUAL.md), also in [Russian](https://github.com/fdrv161/telescope-accessible/blob/telescope-accessible-0.2.0-a11y.4/MANUAL.ru.md) and [Spanish](https://github.com/fdrv161/telescope-accessible/blob/telescope-accessible-0.2.0-a11y.4/MANUAL.es.md): installing, first steps, all keys, and every lens explained.

## Licence and source

GNU AGPLv3. TELESCOPE is by [OVNI Audio](https://ovniaudio.com/telescope); this fork is based on its AGPLv3 release 0.2.0 and adds the accessibility layer. The full source of this release is at the tag [`telescope-accessible-0.2.0-a11y.4`](https://github.com/fdrv161/telescope-accessible/tree/telescope-accessible-0.2.0-a11y.4).

**How this was made.** The accessibility layer, the speech translations, the Russian translation, the build scripts, these fixes and this documentation were written by Claude, an AI model by Anthropic, working in Claude Code at the request of [fdrv161](https://github.com/fdrv161). fdrv161 tested the Windows build by hand with NVDA in REAPER. The macOS build has not been tested by a person.

`SHA256SUMS.txt` holds the checksums of the ZIPs.
