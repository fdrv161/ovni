The second release of **TELESCOPE Accessible**: the TELESCOPE 0.2.0 audio analyser by OVNI Audio, usable with a screen reader and the keyboard.

## Download

- **Windows**: `TELESCOPE-accessible-windows-vst3.zip`, VST3 for Windows 10 and 11, 64-bit. Extract it and copy the `TELESCOPE.vst3` folder into `C:\Program Files\Common Files\VST3` (or another folder your DAW scans), then rescan plugins. To update, close the DAW and replace the old folder.
- **macOS, experimental**: `TELESCOPE-accessible-macos.zip`, VST3 and AU, Apple Silicon and Intel. The accessibility layer has only been tested with NVDA on Windows. The build is not notarized: see README.txt inside or the manual for the one Terminal command it needs.

Each ZIP contains the manual in English, Russian and Spanish.

## What's new since 0.2.0-a11y.1

- **Spectrum** reads the picture the way the eye does:
  - every level is the one the plot shows, with the Slope;
  - "the line", Signal tilt and Shape against the line describe the overall balance;
  - a list of peaks and dips, sized by Smoothing; Enter moves the probe there;
  - what counts as standing out follows the Range, as on screen;
  - F1 explains each setting.
- **Keyboard probes**, the same numbers as the mouse: Spectrogram and Stereo spectrogram (frequency and time), Waterfall, CQT and Spiral (by notes). The crosshair follows the probe.
- **Loudness**: a History row describes the three minutes on screen; the "analysis behind" warning is spoken.
- **Dynamics**: a Most of the time row, the width of the histogram.
- **Scope**: a Picture row says what the goniometer shows; Directions in Hemisphere mode.
- Speech offers only what sighted users have on screen; rows with no on-screen equivalent were removed.
- All translations reviewed: terms match the screen, grammar fixes in several languages.
- The manual is now also in Russian and Spanish.

Tested with NVDA in REAPER on Windows.

## Manual

[MANUAL.md](https://github.com/fdrv161/telescope-accessible/blob/telescope-accessible-0.2.0-a11y.2/MANUAL.md), also in [Russian](https://github.com/fdrv161/telescope-accessible/blob/telescope-accessible-0.2.0-a11y.2/MANUAL.ru.md) and [Spanish](https://github.com/fdrv161/telescope-accessible/blob/telescope-accessible-0.2.0-a11y.2/MANUAL.es.md): installing, first steps, all keys, and every lens explained.

## Licence and source

GNU AGPLv3. TELESCOPE is by [OVNI Audio](https://github.com/ovniaudio/ovni); the accessibility layer was added in this fork. The full source of this release is at the tag [`telescope-accessible-0.2.0-a11y.2`](https://github.com/fdrv161/telescope-accessible/tree/telescope-accessible-0.2.0-a11y.2).

**How this was made.** The accessibility layer, the speech translations, the Russian translation, the build scripts and this documentation were written by Claude, an AI model by Anthropic, working in Claude Code at the request of [fdrv161](https://github.com/fdrv161). fdrv161 tested the Windows build by hand with NVDA in REAPER. The macOS build has not been tested by a person.

`SHA256SUMS.txt` holds the checksums of the ZIPs.
