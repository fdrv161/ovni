The first release of **TELESCOPE Accessible**: the TELESCOPE 0.2.0 audio analyser by OVNI Audio, usable with a screen reader and the keyboard.

## Download

- **Windows**: `TELESCOPE-accessible-windows-vst3.zip`, VST3 for Windows 10 and 11, 64-bit. Extract it and copy the `TELESCOPE.vst3` folder into `C:\Program Files\Common Files\VST3` (or another folder your DAW scans), then rescan plugins.
- **macOS, experimental**: `TELESCOPE-accessible-macos.zip`, VST3 and AU, Apple Silicon and Intel. It builds and passes pluginval, but the accessibility layer has only been tested with NVDA on Windows; VoiceOver may or may not work well. The build is not notarized: see README.txt inside or the manual for the one Terminal command it needs.

Each ZIP also contains the manual.

## What is in it

- Three panels reached with Tab: the lens list, the readings of the open lens, and the plugin controls.
- All 13 lenses readable as text: Loudness, Dynamics, Spectrum (with a frequency probe and the five loudest peaks), Spectrogram, Waterfall, CQT, Spiral, Scope, Band correlation, Stereo spectrogram, Field, Tonal balance, Verdict.
- Every setting adjustable from the keyboard: arrows, plus and minus, edit mode with Enter, Alt with arrows.
- Tonal balance: load a reference and choose a section of it from the keyboard.
- Verdict: every finding and every within-range line read in full.
- Spoken messages when a reference or file analysis finishes.
- Speech in English, Spanish, Portuguese, French, German, Italian and Russian; Russian on screen too.
- Space and DAW shortcuts keep working while the plugin has focus. For sighted users nothing changes.

Tested with NVDA in REAPER on Windows. Both builds pass pluginval at strictness 10.

## Manual

[MANUAL.md](https://github.com/fdrv161/telescope-accessible/blob/telescope-accessible-0.2.0-a11y.1/MANUAL.md): installing, first steps, all keys, and every lens explained.

## Licence and source

GNU AGPLv3. TELESCOPE is by [OVNI Audio](https://github.com/ovniaudio/ovni); the accessibility layer was added in this fork. The full source of this release is at the tag [`telescope-accessible-0.2.0-a11y.1`](https://github.com/fdrv161/telescope-accessible/tree/telescope-accessible-0.2.0-a11y.1).

**How this was made.** The accessibility layer, the speech translations, the Russian translation, the build scripts and this documentation were written by Claude, an AI model by Anthropic, working in Claude Code at the request of [fdrv161](https://github.com/fdrv161). fdrv161 tested the Windows build by hand with NVDA in REAPER. The macOS build has not been tested by a person.

`SHA256SUMS.txt` holds the checksums of the ZIPs.
