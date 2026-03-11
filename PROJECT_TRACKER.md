# LiminalSynth4 — Project Tracker

> Ambient/pad-focused polyphonic wavetable synth. JUCE C++, Xcode, Mac.
> Target: Standalone app + VST3/AU. Inspired by Prophet-5 warmth.

---

## Architecture

**Signal flow:**
```
MIDI → 8×SynthVoice [OSC1/2/3 → Morph → Saturation → Filter1/2 (blend) → Amp Env]
     → Filter 3 (master HP) → Effects (Delay + Reverb) → Master Vol/Pan → Output
```

**Key files:**
| File | Role |
|---|---|
| `SynthVoice.h` | All per-voice DSP: oscillators, filters, envelopes, LFO |
| `AudioEngine.h/cpp` | Synthesiser host, Filter 3, effects, master output |
| `MainComponent.h/cpp` | UI, device setup, wiring entry point |
| `OscWiring.cpp` | OSC 1/2/3 UI callbacks |
| `FilterWiring.cpp` | Filter 1/2/3 UI callbacks + blend |
| `AmpWiring.cpp` | Amp env + Env 3 (free) UI |
| `LFOWiring.cpp` | LFO 1/2 UI wiring |
| `EffectsChain.h/cpp` | Delay + reverb |
| `Styles.h` | Custom LookAndFeel (cyan/dark) |

---

## Phase History

### ✅ Phase 1 — MIDI Input
Set up `MidiMessageCollector`, MIDI note on/off logging. Resolved CoreAudio device config issues (BlackHole).

### ✅ Phase 2 — Single Oscillator Voice
`juce::dsp::Oscillator<float>` sine wave. `juce::Synthesiser` + `SynthesiserVoice` framework chosen for automatic voice allocation.

### ✅ Phase 3 — ADSR Envelope
`juce::ADSR` applied per-voice. Later replaced by `CurvedADSR` (see Phase 21).

### ✅ Phase 4 — Polyphony
8-voice polyphony via `juce::Synthesiser`. Voice stealing handled automatically by JUCE.

### ✅ Phase 5 — Wire AMP ENV Knobs
Amp envelope A/D/S/R sliders connected to all voices. Skewed ranges for musical feel.

### ✅ Phase 6 — Wire OSC 1 Vol and Pan
Per-voice volume and stereo pan with `LinearSmoothedValue` to prevent clicks.

### ✅ Phase 7 — Waveform Buttons + Morph
Popup waveform selector (Sine, Square, Saw, Triangle, Noise). Linear morph between two waveforms per oscillator pair using smoothed blend.

### ✅ Phase 8 — Master Volume and Pan
Global output gain and pan in top bar. Linear crossfade pan law.

### ✅ Phase 9 — OSC 1 Tuning (Oct / Semi / Fine)
Real-time frequency recalculation. Live retuning on held notes. Formula:
`freq = 440 × 2^((note + oct×12 + semi + fine/100 − 69) / 12)`

### ✅ Phase 10 — Filter 1
`StateVariableTPTFilter` (LP/HP/BP). Per-voice (not master bus) for polyphonic expression. Cutoff skewed from 1kHz midpoint. Resonance 0.1–10, drive (tanh) 1–10. Keytrack + velocity sensitivity.

### ✅ Phase 11 — Filter 1 ADSR Envelope
Per-voice filter envelope with Amount knob (±1 = ±20kHz modulation range). Atomic parameters updated per-block for lock-free UI changes.

### ✅ Phase 12 — Custom LookAndFeel (Styles.h)
Cyan accent, dark background theme. Custom rotary and label rendering.

### ✅ Phase 13 — OSC 2 and OSC 3 Wiring
Full waveform/morph/tuning/vol/pan controls for oscillator pairs C/D and E/F.

### ✅ Phase 14 — Effects (Reverb + Delay)
`EffectsChain` class: stereo delay with feedback, JUCE DSP reverb. Post-filter, pre-master.
Initially caused crash — fixed by stabilising oscillator init and per-sample filter envelope processing.

### ✅ Phase 15 — LFO 1 Wiring
Rate (0.01–20Hz), depth, delay (ms), 5 shapes (sine/triangle/square/saw up/saw down).
Pitch modulation only. Randomised per-voice start phase (free-running, not note-synced).

### ✅ Phase 16 — AudioEngine Refactor
Extracted `AudioEngine.h/.cpp` from ~800-line `MainComponent`. Clean separation: UI vs DSP.

### ✅ Phase 17 — Filter 2 + Series/Parallel Blend
Second independent filter with own ADSR. Blend knob: 0=parallel, 1=series.
⚠️ **Known bug:** At intermediate blend values, filters are double-processed (same sample fed to both parallel and series instances). Deferred fix.

### ✅ Phase 18 — Filter 3 HP Master
Global high-pass on master output. On/off toggle, frequency control, smoothed per-sample.

### ✅ Phase 19 — Latency Improvements
Multiple rounds of latency reduction. Portamento glitch fixed. Filter smoothing refined.

### ✅ Phase 20 — Filter ADSR + Pre-filter Soft Saturation
Filter envelopes fully wired. `tanh` drive on oscillator mix pre-filter.
Drive normalised: `tanh(x × drive) / tanh(drive)`.

### ✅ Phase 21 — Exponential Envelopes (CurvedADSR)
Replaced `juce::ADSR` with custom `CurvedADSR` class. Exponential attack/decay/release using capacitor-style curves:
`level += (target − level) × (1 − exp(−3 / timeSamples))`
Applied to amp envelope and both filter envelopes.

---

## Current Backlog

### 🔲 Phase 22 — Per-Voice Parameter Scatter *(Prophet-5 Warmth)*
Add subtle randomisation per voice at note-on:
- Pitch: ±3–8 cents per voice
- Filter cutoff: ±2–5%
- Envelope times: ±1–3%

### 🔲 Phase 23 — Slow Pitch Drift Per Voice
Low-frequency noise oscillator per voice. Independent, slow drift (not LFO-rate). Adds organic instability to sustained pads.

### 🔲 Phase 24 — Voice Filter Cutoff Drift
Slow independent drift of filter cutoff per voice. Mimics analogue component variance between voices.

### 🔲 Phase 25 — Mod Matrix
4 mod rows (source → destination → amount). UI exists, completely unwired.
Sources: LFO1, LFO2, Env1, Env2, Env3, Velocity, Keytrack
Destinations: Pitch, Cutoff 1/2, Resonance, Amp, Pan

### 🔲 Phase 26 — Patch Save / Load
JSON or binary serialisation of all parameters. File browser for patches.

### 🔲 Phase 27 — Init Patch Button
Reset all parameters to default values. Button exists, does nothing.

### 🔲 Phase 28 — LFO 2 Wiring
LFO 2 UI exists (range set), but has no callbacks connected at all. Also: comment and `addAndMakeVisible` calls incorrectly reference `lfo1` (copy-paste bug in `LFOWiring.cpp`).

---

## Known Bugs

| Bug | Location | Status |
|---|---|---|
| Metallic harmonics on sine wave | `SynthVoice.h` — wavetable size capped at 128 samples due to JUCE 8.0.12 build bug with larger tables | Open |
| Series/parallel blend double-processes filters at mid values | `SynthVoice.h:457–468` | Open (deferred) |
| Filter RES overdrives at high values | `SynthVoice.h` — resonance range 0.1–10, no ceiling compensation | Open |
| `smoothedDrive2` declared but never used — Filter 2 drive not applied | `SynthVoice.h` | Open |
| LFO2 `addAndMakeVisible` calls reference `lfo1` not `lfo2` | `LFOWiring.cpp:71–73` | Open |
| LFO2 section comment says "LFO 1" | `LFOWiring.cpp:66` | Minor |
| `DBG` statement left in `startNote()` | `SynthVoice.h:320` | Minor |

---

## Technical Decisions

| Decision | Rationale |
|---|---|
| `StateVariableTPTFilter` over biquad | Better stability at high resonance; supports self-oscillation |
| Per-voice filtering | True polyphonic expression; keytrack/velocity independent per note |
| `LinearSmoothedValue` everywhere | Prevents zipper noise on all parameter changes |
| `CurvedADSR` over `juce::ADSR` | Exponential curves match analogue capacitor behaviour |
| `tanh` saturation | Musical soft clipping; mimics analogue drive |
| Logarithmic cutoff curve | Matches human pitch perception; sweep feels natural |
| Two oscillators per pair with morph | Single morphing voice with no discontinuity |
| `AudioEngine` encapsulation | Clean UI/DSP separation; `MainComponent` stays UI-only |
| 8 voices | Sufficient polyphony for pad textures without CPU excess |
