**Periscope** is the TELESCOPE 0.2.0 audio analyser by OVNI Audio with speech and full keyboard control.

## Download

- **Windows**: `Periscope-windows-vst3.zip`, VST3 for Windows 10 and 11, 64-bit. Extract it and copy the `Periscope.vst3` folder into `C:\Program Files\Common Files\VST3` (or another folder your DAW scans), replacing the old one, then rescan plugins. Projects and settings from earlier versions keep working.
- **macOS, experimental**: `Periscope-macos.zip`, VST3 and AU, Apple Silicon and Intel. Speech has only been tested with NVDA on Windows. The build is not notarized: see README.txt inside or the manual for the one Terminal command it needs.

Each ZIP contains the manual in English, Russian and Spanish.

## What's new

- **The Q of a peak or dip.** In the Spectrum readings, press **Q** on a peak or a dip to hear the Q of an EQ bell of the same width, for example "Q 5.3". It is counted from the width at half the height in dB, the way the RBJ cookbook defines a peaking band and most EQs use it (Equalizer APO and others): one octave is Q 1.41. FabFilter Pro-Q counts about 1.4 times higher. Smoothing makes narrow peaks wider; when that limits the number, Periscope says so, and smoothing off or 1/24 measures a narrow resonance. On other rows Q still searches by first letter.
- **1/3 octave smoothing** in the Spectrum lens, after Off, 1/24, 1/12 and 1/6. It leaves only the broad humps and holes: the usual view for judging the overall balance and comparing mixes, as in TDR Prism, Voxengo SPAN and iZotope Insight.
- **Copy readings.** In any readings panel, **Ctrl+C** copies the current row as text, the way it is spoken, and **Ctrl+Shift+C** copies the whole group: its name, then one row per line. Paste all the Spectrum readings into notes or a chat instead of retyping them.
- The spoken help (F1) and the three manuals describe all three. The manuals also mention Alt+arrows and Alt+Page Up / Page Down in the first steps.

Tested with NVDA in REAPER on Windows.

## Manual

[MANUAL.md](https://github.com/fdrv161/periscope/blob/periscope-1.2.0/MANUAL.md), also in [Russian](https://github.com/fdrv161/periscope/blob/periscope-1.2.0/MANUAL.ru.md) and [Spanish](https://github.com/fdrv161/periscope/blob/periscope-1.2.0/MANUAL.es.md): installing, first steps, all keys, and every lens explained.

## Licence and source

GNU AGPLv3. TELESCOPE is by [OVNI Audio](https://ovniaudio.com/telescope); Periscope is based on its AGPLv3 release 0.2.0. The full source of this release is at the tag [`periscope-1.2.0`](https://github.com/fdrv161/periscope/tree/periscope-1.2.0).

**How this was made.** The speech and keyboard layer, its translations, the Russian translation, the fixes, the new Spectrum features, the build scripts and this documentation were written by Claude, an AI model by Anthropic, working in Claude Code at the request of [fdrv161](https://github.com/fdrv161). fdrv161 tested the Windows build by hand with NVDA in REAPER. The macOS build has not been tested by a person.

`SHA256SUMS.txt` holds the checksums of the ZIPs.
