**Periscope** is the TELESCOPE 0.2.0 audio analyser by OVNI Audio with speech and full keyboard control.

## Download

- **Windows**: `Periscope-windows-vst3.zip`, VST3 for Windows 10 and 11, 64-bit. Extract it and copy the `Periscope.vst3` folder into `C:\Program Files\Common Files\VST3` (or another folder your DAW scans), replacing the old one, then rescan plugins. Projects and settings from 1.0.0 keep working.
- **macOS, experimental**: `Periscope-macos.zip`, VST3 and AU, Apple Silicon and Intel. Speech has only been tested with NVDA on Windows. The build is not notarized: see README.txt inside or the manual for the one Terminal command it needs.

Each ZIP contains the manual in English, Russian and Spanish.

## What's new in the Spectrum lens

- **Weighting.** A new setting shows the spectrum through a standard curve instead of the Slope tilt: A, B, C and D (sound level meters), K (the loudness curve of LUFS), ITU-R 468, ISO 226 at 80 phon, and the inverse cinema X and N curves, which also draw pink noise flat. Every curve is 0 dB at 1 kHz. The plot, the bars, the mouse readout and every spoken level follow it: probe, peaks, shape, signal tilt and the third-octave bands. Slope is one choice of the same setting and works as before. It is a display setting only: Tonal balance and Verdict do not change.
- **Masking.** An estimate of auditory masking, after the psychoacoustic model of AAC encoders (3GPP TS 26.403): which frequency ranges louder sounds nearby cover up, and which lie below the threshold of hearing. Masked bands are darkened on screen. Speech: a Masking row in the readings ("masked: 870 Hz to 1.3 kHz; below hearing: above 18 kHz"), and the probe, the peaks and the dips say "masked" when their frequency is. It follows the curve as drawn, so the Average setting decides how fast it changes.
- Both are new settings in the lens, with spoken help (F1) and translations in all seven languages.

Both follow TDR Prism's Analyzer Weighting and Auditory Masking. They are written from public standards and papers, not from Prism's code, and were checked against screenshots of Prism with test signals: the weighting curves agree within about 1 dB (ISO 226 up to 10 kHz), and around a 1 kHz tone over pink noise both mask about 870 Hz to 1.3 kHz. The shape of the N-curve, for which public sources give only a few points, was measured on Prism's display.

Tested with NVDA in REAPER on Windows.

## Manual

[MANUAL.md](https://github.com/fdrv161/periscope/blob/periscope-1.1.0/MANUAL.md), also in [Russian](https://github.com/fdrv161/periscope/blob/periscope-1.1.0/MANUAL.ru.md) and [Spanish](https://github.com/fdrv161/periscope/blob/periscope-1.1.0/MANUAL.es.md): installing, first steps, all keys, and every lens explained.

## Licence and source

GNU AGPLv3. TELESCOPE is by [OVNI Audio](https://ovniaudio.com/telescope); Periscope is based on its AGPLv3 release 0.2.0. The full source of this release is at the tag [`periscope-1.1.0`](https://github.com/fdrv161/periscope/tree/periscope-1.1.0).

**How this was made.** The speech and keyboard layer, its translations, the Russian translation, the fixes, the new Spectrum features, the build scripts and this documentation were written by Claude, an AI model by Anthropic, working in Claude Code at the request of [fdrv161](https://github.com/fdrv161). fdrv161 tested the Windows build by hand with NVDA in REAPER and took the Prism screenshots. The macOS build has not been tested by a person.

`SHA256SUMS.txt` holds the checksums of the ZIPs.
