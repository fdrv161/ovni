# TELESCOPE Accessible — manual

TELESCOPE is an audio analyser that also draws conclusions. This manual covers the accessible build: every lens, what it measures and how to read it, and how to use all of it with a keyboard and a screen reader.

The analysis engine, the lenses and the measurement notes come from the original [TELESCOPE by OVNI Audio](https://github.com/ovniaudio/ovni), free and open source under AGPLv3. The keyboard and screen reader layer was added in [this fork](https://github.com/fdrv161/telescope-accessible/tree/accessible). Where this manual explains what a lens measures, it follows the author's manual, shortened. The full original, with every formula and test result, is in [`plugins/telescope/docs/manual/README.md`](plugins/telescope/docs/manual/README.md).

## Contents

- [What this plugin is](#what-this-plugin-is)
- [Installing](#installing)
- [First steps with a screen reader](#first-steps-with-a-screen-reader)
- [The window and the three panels](#the-window-and-the-three-panels)
- [Keyboard reference](#keyboard-reference)
- [The plugin panel: bypass, presets, size, language, theme](#the-plugin-panel)
- The lenses:
  [Loudness](#1--loudness) ·
  [Dynamics](#2--dynamics) ·
  [Spectrum](#3--spectrum) ·
  [Spectrogram](#4--spectrogram) ·
  [Waterfall](#5--waterfall) ·
  [CQT](#6--cqt) ·
  [Spiral](#7--spiral) ·
  [Scope](#8--scope) ·
  [Band correlation](#9--band-correlation) ·
  [Stereo spectrogram](#10--stereo-spectrogram) ·
  [Field](#11--field) ·
  [Tonal balance](#12--tonal-balance) ·
  [Verdict](#13--verdict)
- [Analysing a file](#analysing-a-file)
- [Languages](#languages)
- [Known limits](#known-limits)
- [Licence and credits](#licence-and-credits)

---

## What this plugin is

Thirteen **lenses** over one analysis engine, plus a rules engine (Verdict) that turns what was measured into sentences. Each sentence carries the number behind it.

- **It does not touch your audio.** TELESCOPE reads the left and right channels, analyses them on its own thread and writes nothing back. Zero latency, no tail, bit-exact output. The author verifies this with automated tests.
- **The audio never waits for a measurement.** Analysis runs on a separate thread.
- **Stereo only.** No surround, no Ambisonics.
- **It does not locate sources.** Field and the polar views show energy by pan direction, not where a sound was in a room.
- **No AI, no network, no telemetry.** Verdict is a fixed table of thresholds: the same signal always gives the same report, and nothing leaves your computer.

### What the accessible build adds

The original interface is fully painted, so screen readers found nothing in it. This build puts real, focusable controls on top of the painted interface:

- a **lens list**, a **readings panel** and a **plugin panel**, all reachable with Tab;
- every number of every lens as text rows ("Integrated, -14.2 LUFS");
- every setting adjustable from the keyboard;
- spoken messages when a background analysis finishes;
- speech in seven languages: English, Spanish, Portuguese, French, German, Italian and Russian;
- Russian on screen as well.

For sighted users nothing changes: the mouse works exactly as before, and a mouse click does not move keyboard focus into the plugin, so your DAW shortcuts keep working.

Tested with NVDA in REAPER on Windows. Other screen readers that use UI Automation (Narrator, JAWS) should work too.

---

## Installing

### Windows

A **VST3 plugin for Windows 10 or 11, 64-bit**. This is the tested build.

1. Download [`TELESCOPE-accessible-windows-vst3.zip`](https://github.com/fdrv161/telescope-accessible/releases/latest/download/TELESCOPE-accessible-windows-vst3.zip) from the [latest release](https://github.com/fdrv161/telescope-accessible/releases/latest).
2. Extract the ZIP. Inside is a folder named `TELESCOPE.vst3`. Copy that whole folder into your VST3 folder:
   - the standard one is `C:\Program Files\Common Files\VST3` (Windows asks for administrator permission to copy there);
   - or any folder you added to your DAW's VST3 paths. In REAPER: Options, Preferences, Plug-ins, VST, "VST plug-in paths".
3. Restart your DAW or rescan plugins. In REAPER: Options, Preferences, Plug-ins, VST, "Re-scan".
4. Insert **TELESCOPE** on a track or on the master.

To update, close the DAW and replace the `TELESCOPE.vst3` folder with the new one.

The build is not code-signed. That does not affect a VST3 loaded by a DAW.

### macOS (experimental)

VST3 and AU, universal (Apple Silicon and Intel), macOS 11 or newer. It builds and passes pluginval, but **the accessibility layer has only been tested with NVDA on Windows**. JUCE exposes the same controls to VoiceOver and the announcements use VoiceOver's own mechanism, so it may work well, partly, or not at all. If you try it, please [report how it goes](https://github.com/fdrv161/telescope-accessible/issues).

1. Download [`TELESCOPE-accessible-macos.zip`](https://github.com/fdrv161/telescope-accessible/releases/latest/download/TELESCOPE-accessible-macos.zip) and open it.
2. Copy `TELESCOPE.vst3` into `~/Library/Audio/Plug-Ins/VST3` and `TELESCOPE.component` into `~/Library/Audio/Plug-Ins/Components`.
3. The build is signed ad hoc, not notarized by Apple, so macOS blocks it until you remove the quarantine flag. In Terminal:

   ```bash
   xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/TELESCOPE.vst3 ~/Library/Audio/Plug-Ins/Components/TELESCOPE.component
   ```

4. Rescan plugins in your DAW. Logic may need a restart to list the AU.

On macOS the keys are the same as below, with Option in place of Alt.

---

## First steps with a screen reader

1. Insert TELESCOPE and open its window. In REAPER the FX window opens with the plugin; press Tab if focus is still on the FX list.
2. Focus lands on the **lens list**. NVDA says something like "Lenses, Loudness".
3. Press **Up** and **Down** to choose a lens. The lens switches immediately, the same as clicking it.
4. Press **Enter** to go to the **readings** of that lens. NVDA reads the current row, for example "Integrated, -14.2 LUFS".
5. Use **Up** and **Down** to move through rows, **Left** and **Right** to move between groups (Readings, Settings, Actions and so on).
6. On a setting, press **plus** or **minus** to change it, or **Enter** for edit mode.
7. Press **Escape** to go back to the lens list.
8. Press **F1** anywhere for spoken help about the current panel.

Start playback with **Space**: it goes to the DAW, so you can start and stop the transport without leaving the plugin.

---

## The window and the three panels

The painted window has a strip of thirteen lenses on the left, the open lens in the middle, and a header at the top with bypass, presets, A/B, window size, language and theme.

For the keyboard it is three panels, in this Tab order:

1. **Lens list.** One row per lens. Choosing a row opens the lens.
2. **Readings.** A grid of groups and rows for the open lens: its numbers, its settings and its actions. Each lens remembers where your cursor was, so coming back to a lens returns you to the same row.
3. **Plugin.** The header controls: bypass, presets, A/B, window size, language and theme.

Tab moves forward and Shift+Tab moves back. After the last panel Tab wraps to the first.

Only the modules the open lens needs are computed, so lenses you are not looking at cost nothing. The exception is loudness, which always runs, so integrated loudness, loudness range, the histogram and the clip count never have gaps.

---

## Keyboard reference

### Lens list

- **Up / Down**: choose a lens (it opens at once).
- **A letter**: jump to the next lens whose name starts with that letter.
- **Enter**: go to the readings of the lens.
- **Tab / Shift+Tab**: next or previous panel.
- **F1**: spoken help.

### Readings

Moving:

- **Up / Down**: previous or next row in the group.
- **Left / Right**: previous or next group.
- **Home / End**: first or last row of the group.
- **Ctrl+Home / Ctrl+End**: first or last row of the whole grid.
- **Page Up / Page Down**: 10 rows up or down.
- **A letter or digit**: next row starting with it. With **Shift**: previous.
- **F5**: read the row again with a fresh value. Readings are live, so this is how you follow a changing number.

Changing values:

- **Plus / Minus** (main keyboard or numpad; `=` also counts as plus): one step.
- **Alt+Up / Alt+Down**: one step.
- **Alt+Page Up / Alt+Page Down**: a big step.
- **Alt+Home / Alt+End**: minimum or maximum.
- **Enter on a setting**: edit mode. Then **Up / Down** change the value, **Page Up / Page Down** make a big step, and **Enter** or **Escape** leave edit mode.
- **Enter on an action** (Reset, Load and so on): run it.
- **Enter on a plain reading**: read it again.

When a value changes, only the new value is spoken. At the end of a range nothing is spoken and nothing wraps around: silence means you are at the edge. The same goes for the first and last rows.

Other keys:

- **Escape**: back to the lens list (or out of edit mode).
- **F1**: spoken help. On a row that has its own help (the Spectrum settings, for example) it explains that row; elsewhere it gives the help of the panel.
- **Tab / Shift+Tab**: next or previous panel.

### Keys that go to your DAW

- **Space**: start and stop the transport.
- **Ctrl and Alt shortcuts** that the plugin does not use (Ctrl+S, Alt+F4 and so on).
- **Function keys** other than F1 and F5.

Other plain keys are kept by the plugin while it has focus, so an arrow or Delete never triggers a DAW action by accident.

### Spoken messages

TELESCOPE speaks on its own only when something finishes in the background:

- Tonal balance: "Reference analysed, ..." or "Reference could not be read: ...".
- Verdict: "File analysed, ..." followed by the headline, or "File could not be read: ...".

Everything else is spoken in response to your keys.

---

## The plugin panel

The rows of the third panel, in order:

- **Bypass**: Enter or plus/minus switch it on and off. TELESCOPE passes audio through unchanged either way.
- **Preset**: the current preset name, with "modified" if you changed something. **Enter** opens the preset menu (Production, Sound design, User); arrow keys and Enter choose. Focus returns to this row afterwards.
- **Save preset**: **Enter** opens a dialog with a name field. Type a name and press Enter, or Escape to cancel.
- **A B slot**: which of the two comparison slots is active. Plus selects B, minus selects A.
- **Copy this slot to the other**: Enter copies the active slot over the other one.
- **Window size**: S, M or L. Enter starts edit mode; plus and minus also work. Each size has its own layout, not a scaled drawing.
- **Language**: the language of the screen and of speech, see [Languages](#languages).
- **Theme**: dark or light. Plus selects light.

---

# The lenses

Each lens below has two parts: what it measures and how to read it (from the author's manual), and **With the keyboard**: the groups and rows you will hear.

## 1 · Loudness

![Loudness lens: four level bars, the integrated value and three minutes of history](plugins/telescope/docs/manual/images/loudness.png)

**What it measures.** How loud the programme is, to the broadcast standard (ITU-R BS.1770 and EBU R 128).

- **Integrated** (LUFS): the whole programme since the last reset, with the standard double gate (absolute at -70 LUFS, relative at 10 LU below the mean).
- **Momentary** (LUFS): a 400 ms window.
- **Short-term** (LUFS): a 3 s window, updated 10 times a second.
- **Loudness range** (LU): EBU Tech 3342, the spread between the 10th and 95th percentiles of the short-term values.
- **True peak** (dBTP): BS.1770 Annex 2, measured before the K-weighting, overall and per channel.
- **Maximum momentary and short-term**: since the last reset.

**How to read it.** On screen there are four bars on one 0 to -60 scale: momentary and short-term in LUFS, left and right true peak in dBTP. Below is three minutes of short-term history. If you choose a streaming platform target, its line is drawn and the distance from your integrated value is written out.

**While a window is still filling** (400 ms for momentary, 3 s for short-term) the value is *partial*: the same calculation over the audio there is, not a guess. Once the window fills it matches the official number exactly. Integrated loudness has no partial value: the gate is the gate.

**Worth knowing.** True peak is measured with 4x oversampling, which can under-read slightly, within the standard's own tolerance. If your master reads -1.0 dBTP here, treat it as a little higher.

### With the keyboard

- **Readings**: Integrated, Short-term, Momentary, Loudness range, True peak max (overall, then left and right), Max short-term, Max momentary, Target (the platform and how far you are from it). A value that is still filling is followed by "partial".
- **Actions**:
  - **Platform target**: plus and minus step through the streaming platforms, or "none".
  - **Pause**: Enter pauses or resumes measuring.
  - **Reset**: Enter clears everything and starts again.

---

## 2 · Dynamics

![Dynamics lens: PSR and PLR, the short-term histogram and the clip timeline](plugins/telescope/docs/manual/images/dynamics.png)

**What it measures.** Not how loud the mix is (that is Loudness) but how much headroom is left in it.

- **PSR** (dB): highest true peak of the last 3 seconds minus short-term loudness. The live reading.
- **PLR** (dB): highest true peak since reset minus integrated loudness (AES TD1004).
- **Histogram**: short-term loudness since reset, in 1 LU steps.
- **Clips**: events above the clip threshold (default -1.0 dBTP, range -3 to 0) since reset, and a 10-minute timeline.

**How to read it.** A steady sine reads PSR = PLR = 0 dB, and that is correct: a steady tone has no peak headroom above its own level.

**One clip event is not one sample over the line.** An overshoot that lasts 50 ms crosses the threshold once per cycle, but it is counted as one event: an event starts on the first sample above the threshold and ends only after 100 ms below it.

**Changing the threshold restarts the count**, because a count measured against two different ceilings would mean nothing.

The 8 dB line on screen is a reference, not a judgement. Judgements are Verdict's job, with the rule visible.

### With the keyboard

- **Readings**: PLR, PSR (with "low" when it is under the reference), Short-term, Most frequent short-term (the histogram's peak and how much of the time it held), Clip events (count above the threshold), Clips in the last 10 minutes (and how long ago the latest one was).
- **Actions**:
  - **Clip threshold**: plus and minus move it by 0.1 dB, a big step is 0.5 dB.
  - **Pause** and **Reset**: Enter.

---

## 3 · Spectrum

![Spectrum lens: FFT curve with peak hold](plugins/telescope/docs/manual/images/spectrum.png)

**What it measures.** The frequency spectrum, with an exact dB reference: each bin is in dBFS relative to a full-scale sine, so a sine of a given amplitude reads its true level with every window shape.

**How to read it.** Two consequences are worth knowing:

- **Full-scale white noise does not read 0 dBFS per bin.** Its energy is spread over thousands of bins, so each one reads far lower.
- **Changing the FFT size moves the noise floor, not the tones.** Doubling the size splits each bin in two: a tone keeps its level, noise drops 3 dB per bin.

**Why the default slope is 3 dB per octave.** An FFT bin is a fixed width in hertz, while music and hearing work in octaves. Pink noise, the neutral reference, falls 3 dB per octave in a raw spectrum. With a slope of 3, pink noise reads flat. A typical finished mix falls a little faster, which is why many analysers use 4.5. The slope is a display setting: it does not change the data, and it is not applied to the band modes, which already measure in fractions of an octave.

**Smoothing does not change the data either.** It smooths the drawn curve: off shows every narrow peak (a busy picture, good for hunting resonances), 1/6 octave shows only broad humps and holes (the overall balance).

**A band with no bin in it is not a band at zero.** With a small FFT, low bands can be narrower than one bin. That means "cannot be measured at this resolution", not "no energy". For bass, use a big FFT.

### With the keyboard

Slope and Smoothing exist to help the eye: Slope makes a balanced mix look flat, Smoothing decides how big a detail must be to show. Speech uses them the same way. They never change a spoken level; they change what the level is compared with and how much is listed.

**The line.** A straight line with the Slope setting, at the median height of the curve between 40 Hz and 16 kHz. On screen it would be a flat line through the middle of the curve. With Slope at 4.5, a typical finished mix lies along it: a place well above it has more energy than a balanced mix, a place well below has less.

- **Readings**:
  - **Probe**: a frequency cursor that moves by notes. Plus and minus move it a semitone, a big step is an octave, and the range is about 20 Hz to 20 kHz. It reads the frequency, the note name, the level of the left and right channels (with peak hold if it is on) and how far it is from the line, for example "3 dB above the line". With smoothing on, the level is that of the smoothed curve, as drawn. The crosshair on screen follows the probe, so a sighted helper can see where you are.
  - **Signal tilt**: how fast the level falls from 50 Hz to 10 kHz, for example "falls 4.2 dB per octave", and how that compares with the line: "brighter than the line by 0.3", "darker than the line by 1.5" or "follows the line".
  - **Shape against the line**: the first glance at the picture. The stretches of the curve that stand out above or below the line (see below what counts as standing out), from low to high, with their average distance, for example "35 to 80 Hz +5 dB, 85 to 400 Hz -4 dB, 450 Hz to 2 kHz +3 dB, above 12 kHz -8 dB". A range that is not mentioned follows the line. It always looks at a broad 1/3 octave view, whatever the Smoothing, so the overview does not change while you zoom into details.
  - **Peaks and dips** (up to 10 of each): places where the curve stands out from the neighbouring frequencies, the most prominent first. How much a hump stands out is measured the way the eye sees it: above the higher of the two valleys beside it (for a hole, below the lower of the two crests). Smoothing sets their size:
    - With smoothing off, the peaks are narrow resonances and strong notes: frequency, note with cents, how many dB it stands out, and its level.
    - With smoothing on, they are humps and holes: frequency, where it starts and ends (for example "290 to 400 Hz", handy for an EQ band), and how many dB it stands out.

    The list keeps the 10 most prominent of each; the Smoothing row says how many there are in all. Unsmoothed there are dozens of spikes and the list holds the tallest, as the eye picks them out; at 1/6 octave there are a few humps and usually all of them fit. If nothing stands out, the row says "none". Dips are listed only with smoothing on. **Enter** on a peak or a dip moves the probe there and the cursor to the probe row, so you can walk around it by semitones. While music plays, the list and the shape hold still for a second, so rows read one after another agree.
- **Third-octave bands**: the 30 standard bands from 25 Hz to 20 kHz, with the level of each.
- **Settings**, in this order:
  - **Pause**: Enter holds the analysis of all lenses, so every reading comes from the same moment, like a still picture. It is the same pause as in Loudness and Dynamics.
  - **Reset**: Enter clears the averages, the peak hold and the readings of all lenses.
  - **Slope**: 0 to 4.5 dB per octave. The value is followed by how the signal now sits against the line, so you hear the effect at once.
  - **Smoothing**: off, 1/24, 1/12 or 1/6 octave. The value is followed by how many peaks and dips stand out at this smoothing, for example "1/6 octave, peaks: 7, dips: 5": you hear how busy the picture is.
  - **Average**: off, a running average (its time is spoken), or infinite.
  - **Average time**: 0.1 to 10 seconds for the running average. 2 or 3 seconds keeps the numbers from jumping between key presses. It says "not used now" when Average is off or infinite.
  - **Peak hold**: off, 12 dB per second, 30 dB per second, infinite.
  - **Range**: 60, 90 or 120 dB, how far below 0 dB the plot goes. It also sets what counts as standing out, see below.
  - **FFT size** (1,024 to 32,768 points), **Window** (Hann, Blackman-Harris, Kaiser), **Overlap**, **Channel** (left, right, mid, side, left and right), **Display** (FFT curve, third-octave bars, Bark bars).
- **F1** on the probe, the signal tilt, the shape, a peak or dip, or any setting explains it in a sentence or two.

**What counts as standing out.** An analyser has no threshold: the eye notices a bump when it has some height on screen. The plot always spans the same pixels, so that height depends on the Range. TELESCOPE counts a stretch, a peak or a dip when it stands out by a twentieth of the Range: 3 dB at 60 dB, 4.5 dB at 90, 6 dB at 120. It is the same at every smoothing, so the smoothing alone decides how much detail there is, as on screen. To hear smaller bumps, choose a smaller Range, just as a sighted user would to see them.

A way to use it while mixing:

1. Set Average time to 2 or 3 seconds. For a whole song, set Average to infinite and press Reset at the start.
2. Read Signal tilt and Shape: is the mix brighter or darker than the line, and which ranges stick out or sag?
3. Smoothing at 1/6 octave: the peaks and dips are the humps and holes inside those ranges, with their edges.
4. Smoothing off: the peaks are narrow resonances and strong notes. A resonance shows in both the level and the peak hold; a peak that is only in the hold was a short hit.
5. Enter on a peak, then plus and minus around it to hear its width. Pause when you need every number from the same moment.

The FFT settings are shared with Spectrogram, Waterfall and the other spectral lenses: it is one setting, not a copy.

---

## 4 · Spectrogram

![Spectrogram: frequency over time with level as colour](plugins/telescope/docs/manual/images/spectrogram.png)

**What it measures.** Level over time: frequency up the screen (20 Hz to 20 kHz), time across it (10, 30 or 60 seconds, now on the right), level as colour.

**How to read it.** The colour scales are chosen so that brighter always means louder (except the "spectrum" palette, where hue orders the level). Each row takes the maximum of its bins, so narrow peaks are not flattened. Changing the FFT size, channel, range or history clears the picture, because mixing columns measured in two ways would be a picture that lies.

**The palette is one setting for four lenses**: Spectrogram, Stereo spectrogram, Waterfall and Field.

### With the keyboard

This lens is a picture of the spectrum over time and has no numbers of its own. The **About** row says so. For numbers, use Spectrum.

- **Settings**: History, Range, Channel, Palette (Ovni, Inferno, Viridis, Spectrum). They are shared with Spectrum, so you can set them here too.

---

## 5 · Waterfall

![Waterfall: the spectrum over time in 3D](plugins/telescope/docs/manual/images/waterfall.png)

**What it measures.** The same data as the spectrogram, drawn in depth: frequency across, level as height, time going back into the distance.

**How to read it.** The eye compares heights better than colours, so narrow peaks stand out here. The cost is that a short event can hide behind a later one. Spectrogram and Waterfall complement each other.

### With the keyboard

A picture only; the **About** row says so. For numbers, use Spectrum.

- **Settings**: Lines (60, 90, 120), Tilt (low, medium, high), History, Range, Palette.

---

## 6 · CQT

![CQT: the spectrum by notes over a piano keyboard, with the chromagram below](plugins/telescope/docs/manual/images/cqt.png)

**What it measures.** The spectrum by **note**. A constant-Q transform spaces its bins evenly in octaves rather than hertz: 24 bins per octave (two per semitone) from A0 (27.5 Hz) upwards. Below it is the chromagram, the energy of each of the 12 pitch classes, and the estimated key.

**The bass is late, and the lens says so.** Quarter-tone resolution at 27.5 Hz needs over a second of audio (1.24 s at 48 kHz), and the time halves with every octave up. That is physics, not a shortcut, and the lens shows it.

**The key is never stated alone.** It always comes with a confidence and the share of time it held, for two reasons:

1. Even noise "wins" some key by chance, with a confidence around 0.4 to 0.5. What gives noise away is the share of time: a few percent, against nearly 100 percent for real music.
2. Some music is genuinely ambiguous. A bare C, E, G triad scores slightly higher for E minor than C major; double the C in the bass and C major wins clearly.

Silence reports no key, because "I don't know" is a result.

### With the keyboard

- **Readings**:
  - **Key**: for example "A minor, confidence 0.82, best 64 percent of the time".
  - **Strongest pitch classes**: the three strongest notes of the chromagram.
  - **Note 1 to 5**: the five loudest notes of the spectrum, each with octave, deviation in cents and level.
- **Settings**: Channel, Chroma window (how long the chromagram averages: 0.5, 2 or 5 seconds).

Note names follow the plugin language (letters, do-re-mi, or German with H and B), and sharps are spoken as words.

---

## 7 · Spiral

![Spiral: the constant-Q spectrum coiled one turn per octave](plugins/telescope/docs/manual/images/spiral.png)

**What it measures.** The same constant-Q spectrum, coiled one turn per octave, so the same note in different octaves lines up on one radius. It shows the octave structure of what is playing at a glance, with a chroma wheel and the key in the centre.

### With the keyboard

- **Readings**: Key, Strongest pitch classes.
- **Settings**: Channel, Chroma window.

For individual notes with frequencies, use CQT.

---

## 8 · Scope

![Scope: goniometer, correlation meter and oscilloscope](plugins/telescope/docs/manual/images/scope.png)

**What it measures.** The classic ways of looking at stereo, over a sliding window of 100, 300 or 1,000 ms.

- **Correlation**: +1 is mono, 0 is unrelated channels, -1 is out of phase.
- **Width**: 0 is mono, 1 is two independent sources, above 1 the side signal dominates.
- **Balance** (dB): positive means the right channel is louder.
- **Mono loss** (dB): how much level is lost when the mix is summed to mono. 0 if left equals right, -3 dB for independent channels, a very large loss if they cancel.

**Modes.** *Lissajous* is the goniometer: it scales itself to the signal so you see the shape of the stereo, not the level, and says what the edge level is. *Polar sample* keeps the angle and makes the radius the level. *Polar level* (called Hemisphere in the settings) is a half-circle with mono at the top and left and right on the base, energy drawn as one ray per degree; what is out of phase is folded onto the base and the percentage is printed. The oscilloscope shows 40 ms, optionally triggered on a rising zero crossing.

**Edge cases are reported, never as nonsense numbers.** No signal reads "no signal", which is not the same as perfect mono. With one channel silent the correlation is undefined and shown as 0.

**It is not localisation.** Pan direction by energy, not where a sound comes from in a room.

### With the keyboard

- **Readings**: Correlation (with a word for its meaning, such as "nearly mono" or "out of phase"), Width, Balance ("1.2 dB louder on the right", or "centred"), Mono loss, Out of phase energy (percent), Measured over (the window length).
- **Settings**: Mode (Lissajous, Polar, Hemisphere), Oscilloscope trigger (on, off), Window, Hemisphere decay (12, 24 or 48 dB per second), Hemisphere scale (relative or decibels).

The picture itself is not described in text: its meaning is these numbers.

---

## 9 · Band correlation

![Band correlation: correlation per third-octave band](plugins/telescope/docs/manual/images/band-correlation.png)

**What it measures.** Scope's stereo figures **per third-octave band**: correlation, width, balance and mono loss for each of the 30 bands.

**Why.** On a mix with mono bass and wide highs, a single correlation meter gives an in-between number that says nothing. In the author's test signal (an 80 Hz tone in mono plus out-of-phase noise above 2 kHz) the broadband meter reads +0.24, while the 80 Hz band reads +1.00 and the 4 kHz band -1.00. Two different things are happening, and only a per-band view shows both.

**A band with no FFT bin in it is not zero.** At small FFT sizes the lowest bands cannot be measured, and the lens says so. In the bass the readings also scatter more, because there is less signal per second down there; no analyser can invent it.

### With the keyboard

- **Summary**: Lowest correlation, Most mono loss (the worst bands, the same as the line above the plot), Bands measured.
- **Third-octave bands**: each band with all four numbers at once: "correlation 0.95; width 0.2; balance 0.1 dB; mono loss -0.1 dB". On screen the second line shows only one of the three; here you get all of them.
- **Settings**: Window (0.3, 1 or 3 seconds), Lower graph (which figure the screen shows under the correlation).

---

## 10 · Stereo spectrogram

![Stereo spectrogram: colour is phase, brightness is level](plugins/telescope/docs/manual/images/stereo-spectrogram.png)

**What it measures.** A spectrogram where **colour is phase** (red out of phase, green wide, white mono) and **brightness is level**. A cell with no energy is black, so the noise floor does not paint meaningless colours.

### With the keyboard

A picture only; the **About** row says so. For numbers per band, use Band correlation.

- **Settings**: History, Range, Window, Palette.

---

## 11 · Field

![Field: energy by pan direction and frequency](plugins/telescope/docs/manual/images/field.png)

**What it measures.** Energy by **pan direction and frequency**, with decay and a trail. Pan goes from -1 (left only) through 0 (centre) to +1 (right only).

**What only this lens shows.** A mix with two hard-panned sources and a mix of decorrelated noise give nearly the same correlation and width. In Field the first is two blobs in opposite corners and the second an even cloud across. That difference is the point of the lens.

**What it does not do.**

- It does not measure where sources are. From a finished stereo mix that has no unique answer, so the axis is left, centre, right, never degrees, and the screen says "energy by pan direction, not localisation".
- It does not tell mono from out of phase: both put equal energy in each channel. Band correlation and Stereo spectrogram show that difference; the three lenses are read together.
- The dB here are **relative** to the loudest cell. Absolute levels are in Spectrum.

### With the keyboard

- **Octaves**: one row per octave band, for example "250 Hz octave, centre 10 percent left, spread from 40 percent left to 20 percent right, -6 dB". Centre is where the energy sits on the pan axis, the spread covers the main part of it (10th to 90th percentile), and the dB is relative to the loudest octave. A narrow spread is said as "narrow"; an octave with no energy as "silent".
- **Settings**: Decay (0.5, 1 or 2 seconds), Window (0.3, 1 or 3 seconds), Palette.

---

## 12 · Tonal balance

![Tonal balance: programme and reference curves with the difference below](plugins/telescope/docs/manual/images/tonal-balance.png)

**What it measures.** Not "how loud is it" but **"what colour is it, compared to this other track"**. Load a reference track, yours or a commercial one. TELESCOPE analyses it whole, offline, and compares your live programme against it, band by band.

**It compares tilt, not level.** Both curves are normalised by their own integrated loudness, so the same pink noise at -14 and at -20 LUFS shows no difference at all. Without that the graph would answer "which one is louder", which Loudness already tells you.

**So the difference sums to zero.** Add 6 dB to everything above 2 kHz and the overall loudness rises too. The lens then shows about +1.7 dB in the top and -4.3 dB in the bass: still a 6 dB gap between the two regions, but counted at equal loudness, which is how you compare a mix with a reference.

**How to read it.** A fixed scale, so two captures of the same mix stay comparable. The ±3 dB band on screen is a reference, not a judgement. A band that only one side can measure is never shown as "no difference": the row says why it cannot be compared.

**The reference is stored as a path, not as numbers.** Reopen a session and the file is analysed again. If it is gone, the lens says "reference not found" and shows nothing, rather than a reference that no longer exists.

### With the keyboard

- **Summary**:
  - **State**: what the lens is doing, as the status line on screen says (measuring, analysing the reference and so on).
  - **Reference**: the reference file name, or "none".
  - **Reference section**: the part of the reference being used, or "whole file".
  - **Largest difference 1 to 5**: the bands where your programme differs most from the reference, for example "125 Hz, 3.2 dB louder than the reference".
- **Third-octave bands**: each of the 30 bands with the programme level, the reference level and the difference.
- **Actions**:
  - **Reset averaging**: restarts the programme's average and keeps the reference.
  - **Load reference file**: opens a file dialog.
  - **Remove reference**.
  - **Section start** and **Section end**: choose the part of the reference to compare against, without the mouse. Plus and minus move by one second, a big step is 10 seconds, Alt+Home and Alt+End go to the start or end of the file. The new bounds are spoken as you move; the reference is analysed again once you stop.
  - **Use the whole reference file**: the same as a double click on the strip.

When the reference analysis finishes, TELESCOPE says so: "Reference analysed, ..." with its name and loudness, or the reason it could not be read.

---

## 13 · Verdict

![Verdict: headline, findings by section and the within-range list](plugins/telescope/docs/manual/images/verdict.png)

Twelve lenses show. This one **says**. The rule is that every sentence carries the number that supports it.

**What it is.** A fixed rules engine: no AI, no network. A table of thresholds and the conditions that read them. The same signal gives the same report, word for word, today and in a year. Every threshold with its reasoning is in [`telescope-diccionario.md`](plugins/telescope/docs/telescope-diccionario.md) (Spanish).

**How it reads.** First a **headline**: the count of what was measured, with no adjectives, for example "15 checks within range, 3 to look at, the first at 0:20". Then the sections:

- **Within range**: every rule that was checked and did not fire, with its number and its limit, for example "Transients have room: PSR 9.4 dB (floor 8.0 dB)". A rule that could not be checked says nothing; "within range" is a measurement, not a default.
- **How it will feel**: crushed transients, thin or muddy low mids, harsh presence, no air on top, a hollow centre, and the key.
- **Where it translates**: generic checks for phone, headphones, laptop, car, club (mono sub) and hi-fi.
- **What to check, and where**: dips in bands, quiet and loud sections, clip bursts, bands out of phase, left-right imbalance, DC offset, and streaming platform targets.

**How a finding reads.** The number first, the mixing term in brackets, and at the end where to look, never what to do: "630 Hz dips 26.0 dB below its own average between 0:20 and 0:35. Check what plays in that band there." Severity is problem, warning or note; problem is kept for things that are objectively broken (clip bursts, DC, bands that cancel in mono).

**What it compares against.** With a reference loaded in Tonal balance, against the reference's shape. Without one, against your material's own spectral trend, a straight line fitted through your mix. The second is deliberate, so no hidden "good mix curve" is passed off as measurement, but it is blunt: a mix with a strong intention can fire a rule with nothing to fix. **Loading a reference is strictly better.**

**What it is not.**

- It does not give opinions. It never says "professional" or "ready"; the author's tests scan every phrase in every language for such words.
- The device checks are generic definitions of classes of systems, not measurements or simulations of real speakers.
- **"No findings" is not "it's finished".** When nothing fires, it says: "Nothing outside the ranges of these rules. They measure; what they can't hear is yours."

**Live or file.** In live mode Verdict uses the last 10 minutes since reset, once a second. The spectral part is only measured while Verdict or Tonal balance is open, so if you open Verdict halfway through a track, the report starts there. For exact times over a whole track, use **file** mode: the file analysis is identical to the live one.

### With the keyboard

- **Summary**: State (the status line on screen), Result (the headline).
- **How it will feel**, **Where it translates**, **What to check, and where**: one row per finding. Each row reads the full sentence with its numbers, followed by its severity: problem, warning or note (for the devices: OK, warning or fails). An empty section is one row that says so.
- **Within range**: one row per rule that was checked and passed.
- **Actions**:
  - **Reset analysis**.
  - **Mode**: live or file; plus and minus switch it.
  - **Load a file to judge**: opens a file dialog and switches to file mode.

Section names and findings are spoken in the plugin language. When a file analysis finishes, TELESCOPE says "File analysed" and reads the headline.

---

# The rest

## Analysing a file

Tonal balance (reference) and Verdict (file mode) can analyse a whole audio file in the background, with Enter on their Load row. With the mouse, you can also drag a file onto those lenses.

- **The offline analysis is identical to the live one**, to the bit. The engine counts samples, not host blocks, and the file path uses the same code.
- **Formats**: WAV, AIFF, FLAC and Ogg; on Windows also WMA and MP3. Any file is accepted; if it cannot be read, you are told why.
- **The sample rate is the file's.** Nothing is resampled, and neither third-octave bands nor LUFS depend on sample rate, so a 44.1 kHz reference compares directly with a 48 kHz session.
- **It is fast**: a minute of audio takes well under a second on a modern computer.

## Languages

The **Language** row in the plugin panel switches the whole plugin: the screen and the speech. Seven languages:

- **English** (default) and **Spanish**, reviewed by the author;
- **Portuguese, French, German, Italian**: translated by the author with industry terms, not yet reviewed by native speakers;
- **Russian**: added in this fork, on screen and in speech.

Speech in Spanish, Portuguese, French, German and Italian was translated for this fork and has not been reviewed by native speakers. If a phrase is missing, that phrase falls back to English and the rest stays in your language.

Units (dB, Hz, kHz, ms, LUFS) and industry terms that are normally left in English (Loudness, Scope, Short-term, Momentary, True peak, Peak hold, Bypass) stay in English in every language, as on the author's screens.

**Note names follow each language's convention**: letters (C, D, E) in English and Russian, do-re-mi in Spanish, Portuguese, French and Italian, and the German system with H and B.

## Known limits

- The macOS build is experimental: it builds and passes validation, but has not been tested with VoiceOver.
- Tested with NVDA in REAPER. Other hosts and screen readers should work but have not been tested.
- The main fonts have no Cyrillic. With Russian selected, the system substitutes a font, so Russian text looks different from the rest.
- Polar level (Hemisphere) gives its numbers but not the direction of the strongest energy as text.

## Licence and credits

- **TELESCOPE** is by [OVNI Audio](https://ovniaudio.com), free and open source under the **GNU AGPLv3**. Original source: <https://github.com/ovniaudio/ovni>.
- The accessibility layer, the speech translations and the Russian translation were added in [fdrv161/telescope-accessible](https://github.com/fdrv161/telescope-accessible/tree/accessible), under the same licence. The full source of every release is in that repository, at the release tag.
- Third-party notices: [`NOTICE.md`](NOTICE.md).

**How this was made.** The accessibility layer, the speech translations, the Russian translation, the build scripts and this documentation were written by Claude, an AI model by Anthropic, working in Claude Code at the request of [fdrv161](https://github.com/fdrv161). fdrv161 tested the Windows build by hand with NVDA in REAPER. The macOS build has not been tested by a person.
