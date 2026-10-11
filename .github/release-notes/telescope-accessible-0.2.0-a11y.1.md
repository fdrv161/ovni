The first release of **TELESCOPE Accessible**: the TELESCOPE 0.2.0 audio analyser by OVNI Audio, usable with a screen reader and the keyboard.

## Download

`TELESCOPE-accessible-windows-vst3.zip` below: VST3 for Windows 10 and 11, 64-bit. Extract it and copy the `TELESCOPE.vst3` folder into `C:\Program Files\Common Files\VST3` (or another folder your DAW scans), then rescan plugins. The ZIP also contains the manual.

## What is in it

- Three panels reached with Tab: the lens list, the readings of the open lens, and the plugin controls.
- All 13 lenses readable as text: Loudness, Dynamics, Spectrum (with a frequency probe and the five loudest peaks), Spectrogram, Waterfall, CQT, Spiral, Scope, Band correlation, Stereo spectrogram, Field, Tonal balance, Verdict.
- Every setting adjustable from the keyboard: arrows, plus and minus, edit mode with Enter, Alt with arrows.
- Tonal balance: load a reference and choose a section of it from the keyboard.
- Verdict: every finding and every within-range line read in full.
- Spoken messages when a reference or file analysis finishes.
- Speech in English, Spanish, Portuguese, French, German, Italian and Russian; Russian on screen too.
- Space and DAW shortcuts keep working while the plugin has focus. For sighted users nothing changes.

Tested with NVDA in REAPER. The build passes pluginval at strictness 10.

## Manual

[MANUAL.md](https://github.com/fdrv161/telescope-accessible/blob/telescope-accessible-0.2.0-a11y.1/MANUAL.md): installing, first steps, all keys, and every lens explained.

## Licence and source

GNU AGPLv3. TELESCOPE is by [OVNI Audio](https://github.com/ovniaudio/ovni); the accessibility layer was added in this fork. The full source of this release is at the tag [`telescope-accessible-0.2.0-a11y.1`](https://github.com/fdrv161/telescope-accessible/tree/telescope-accessible-0.2.0-a11y.1).

`SHA256SUMS.txt` holds the checksum of the ZIP.
