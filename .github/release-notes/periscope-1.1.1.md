**Periscope** is the TELESCOPE 0.2.0 audio analyser by OVNI Audio with speech and full keyboard control.

This is a fixes-only release. The fixes come from a review of how the plugin starts, stops, runs its background threads and restores its state.

## Download

- **Windows**: `Periscope-windows-vst3.zip`, VST3 for Windows 10 and 11, 64-bit. Extract it and copy the `Periscope.vst3` folder into `C:\Program Files\Common Files\VST3` (or another folder your DAW scans), replacing the old one, then rescan plugins. Projects and settings from 1.0.0 and 1.1.0 keep working.
- **macOS, experimental**: `Periscope-macos.zip`, VST3 and AU, Apple Silicon and Intel. Speech has only been tested with NVDA on Windows. The build is not notarized: see README.txt inside or the manual for the one Terminal command it needs.

Each ZIP contains the manual in English, Russian and Spanish.

## Fixed

- **REAPER no longer stays in the task manager after closing.** 1.1.0 fixed one cause, but another one remained: a settings file the plugin kept open until it was unloaded, together with a JUCE timer thread that Windows had already ended at exit. The settings file is now opened only for each read or write.
- **The first A/B press no longer loses your project settings.** Slot A was taken when the plugin was created, before the project was loaded, so the first switch could bring back English, no reference and default lens settings.
- **Undo no longer re-analyses files.** REAPER restores the plugin state on every undo; each time, the Tonal balance reference and the Verdict file were analysed again and announced again. Now the same file and section keep their result. Presets and A/B behave the same way.
- **A mono input is measured correctly.** With a mono input the analysis saw the left channel and silence: balance hard left, wrong correlation and width, loudness about 3 dB too low. It is now measured as identical left and right.
- **Rare crashes on closing.** Closing a lens or the plugin window while a file dialog (Tonal balance, Verdict) or the Save preset dialog was open could crash the DAW.
- **Reset during playback** could leave the left and right channels of the analysis one block apart, so correlation and width were wrong until the next reset. The sound itself was never affected.
- **Preset names**: characters a file name cannot have are removed, so a name with a slash or a colon no longer fails silently or saves into another folder.
- **The shared settings file** (theme, language, window size) is waited for at most one second. Before, a frozen program holding it could freeze the DAW when a Periscope window opened.
- **A damaged project** with a value that is not a number no longer makes the Spectrum lens rebuild itself endlessly.
- Smaller: less work on the audio thread, and an idle plugin wakes up five times less often.

## Manual

The manuals were checked against the code: F6 in REAPER's FX chain (with OSARA), what the plugin says on its own, the preset menu and preset names, what is remembered and where, mono input. [MANUAL.md](https://github.com/fdrv161/periscope/blob/periscope-1.1.1/MANUAL.md), also in [Russian](https://github.com/fdrv161/periscope/blob/periscope-1.1.1/MANUAL.ru.md) and [Spanish](https://github.com/fdrv161/periscope/blob/periscope-1.1.1/MANUAL.es.md).

Tested with NVDA in REAPER on Windows.

## Licence and source

GNU AGPLv3. TELESCOPE is by [OVNI Audio](https://ovniaudio.com/telescope); Periscope is based on its AGPLv3 release 0.2.0. The full source of this release is at the tag [`periscope-1.1.1`](https://github.com/fdrv161/periscope/tree/periscope-1.1.1).

**How this was made.** The speech and keyboard layer, its translations, the Russian translation, the fixes, the build scripts and this documentation were written by Claude, an AI model by Anthropic, working in Claude Code at the request of [fdrv161](https://github.com/fdrv161). fdrv161 tested the Windows build by hand with NVDA in REAPER. The macOS build has not been tested by a person.

`SHA256SUMS.txt` holds the checksums of the ZIPs.
