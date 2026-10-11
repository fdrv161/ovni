# TELESCOPE Accessible

**An audio analyser that works with a screen reader.** Loudness, dynamics, spectrum, stereo, key, tonal balance against a reference, and a rules engine that writes down what it measured, all readable with NVDA and fully usable from the keyboard.

This is an accessible build of [TELESCOPE by OVNI Audio](https://github.com/ovniaudio/ovni), a free and open-source analyser plugin. The original draws everything as pictures, so screen readers found nothing in it. This fork adds real controls on top: every number as text, every setting from the keyboard, spoken messages when an analysis finishes. For sighted users the plugin looks and works exactly as before.

## Download

**[Download TELESCOPE Accessible for Windows](https://github.com/fdrv161/ovni/releases/latest/download/TELESCOPE-accessible-windows-vst3.zip)**: a ZIP with the VST3 plugin and the manual, for Windows 10 and 11, 64-bit.

Release notes and older versions: [Releases](https://github.com/fdrv161/ovni/releases).

Install: extract the ZIP and copy the `TELESCOPE.vst3` folder into `C:\Program Files\Common Files\VST3` (or another folder your DAW scans), then rescan plugins.

## Manual

**[Read the manual](MANUAL.md)**: installing, first steps with a screen reader, all keys, and every lens explained, from what it measures to the rows you will hear.

## In short

- Thirteen lenses: Loudness, Dynamics, Spectrum, Spectrogram, Waterfall, CQT, Spiral, Scope, Band correlation, Stereo spectrogram, Field, Tonal balance, Verdict.
- Three panels, reached with Tab: the lens list, the readings of the open lens, and the plugin controls (bypass, presets, A/B, size, language, theme).
- In the readings: arrows move through rows and groups, plus and minus change a setting, Enter opens edit mode or runs an action, F5 reads a live value again, F1 gives spoken help, Escape goes back.
- Space and your DAW's shortcuts keep working while the plugin has focus.
- Speech in English, Spanish, Portuguese, French, German, Italian and Russian. The screen in the same languages.
- It never changes your audio: zero latency, bit-exact pass-through.

Tested with NVDA in REAPER on Windows.

## Source and licence

TELESCOPE is free and open source under the **GNU AGPLv3**. The original is by [OVNI Audio](https://ovniaudio.com). This fork, with the accessibility layer, is on the [`accessible`](https://github.com/fdrv161/ovni/tree/accessible) branch, and the full source of every release is available at its tag.

- Plugin source: [`plugins/telescope`](plugins/telescope)
- Accessibility code: [`plugins/telescope/source/a11y`](plugins/telescope/source/a11y)
- The author's original manual, with every formula and test result: [`plugins/telescope/docs/manual/README.md`](plugins/telescope/docs/manual/README.md)
- The repository also holds OVNI's other plugins, unchanged in this fork. For them, see the [original OVNI README](https://github.com/ovniaudio/ovni#readme).
- Third-party notices: [`NOTICE.md`](NOTICE.md)

## Feedback

Found something that is not readable, a key that does not work, or a phrase that sounds wrong in your language? [Open an issue](https://github.com/fdrv161/ovni/issues).
