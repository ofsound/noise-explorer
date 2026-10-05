# Noise Explorer contributor guide

Noise Explorer by ofsound is a playable noise synthesizer built with the current Pamplejuce template, JUCE 9, C++23, CMake, Catch2, and a native JUCE interface. `AGENTS.md` is a symlink to this file.

## Configuration

- Internal target: `NoiseExplorer`; display name: `Noise Explorer`.
- Company: `ofsound`; bundle: `com.ofsound.noiseexplorer`.
- Manufacturer: `Osnd`; unique plugin code: `Nexp`.
- Formats: Standalone, AU (MusicDevice), VST3 (Instrument/Synth).
- MIDI input, no audio input, mono/stereo output. Eight-voice polyphony.
- Keep JUCE 9 and the template submodule versions pinned. These match Chord Runner's JUCE revision.
- Debug/Ninja builds use `Builds/`, matching the siblings' VS Code / Cursor setup.
- macOS development CI only; no IPP or distribution signing/notarization.
- `packaging/icon.png` is the custom generated source artwork. CMake embeds it into all app/plugin bundles and refreshes the ICNS when it changes.

## Commands

```sh
git submodule update --init --recursive
cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build Builds --parallel 4
ctest --test-dir Builds --output-on-failure
# Optional editor image from the repository root:
./Builds/Tests '[.preview]'
# Optimized music-making build:
cmake -B Builds/Release -G Ninja -DCMAKE_BUILD_TYPE=Release -DNOISE_EXPLORER_COPY_PLUGINS=OFF
cmake --build Builds/Release --parallel 4
```

Build **all three formats** after source or configuration changes. Fix all compiler warnings in owned code; warnings are errors for source and tests. Do not patch vendored libraries to silence warnings. Plugin copying can be disabled with `NOISE_EXPLORER_COPY_PLUGINS=OFF`. Installation into the user plugin folders or Applications may require filesystem permission.

## Architecture

- `source/dsp/NoiseEngine.h`: seeded noise streams, TPT state variable filters, fixed voice storage.
- `source/PluginProcessor.*`: APVTS parameters/state, MIDI scheduling, eight voices, smoothing, scope FIFO.
- `source/PluginEditor.*`: native UI, parameter attachments, FFT display, per-filter response plots, keyboard.
- `tests/PluginBasics.cpp`: audio/MIDI/state/spectral tests and an opt-in preview capture.
- `cmake_project/`: project-owned CMake additions. `cmake/` and `JUCE/` are submodules.

Filters are parallel branches, averaged when multiple are enabled; all off yields the direct source. Classic branches use the master amplitude ADSR. Each resonator uses its own ADSR and follows the voice's MIDI pitch (A4 = 440 Hz). The master envelope does not truncate resonator tails. Continuous mode uses one voice and the last MIDI pitch, initially C4. Instrument mode uses note on/off, velocity, sustain and +/-2-semitone pitch bend.

The source uses a normalized finite-band `f^-alpha` power spectrum, alpha -2 to +2. Color mode sets tilt; Spectrum X/Y adds a signed midrange contour. Green and gray are optional multiplicative profiles. Fixed 4096-point inverse FFT frames overlap with square-root Hann windows. Each voice/channel has an independent seeded stream; all 16 streams stay warm with staggered frame boundaries. A shared spectral plan caches weights and expected-RMS normalization. Color/contour smooth over 80 ms. See `docs/spectral-model.md` for exact definitions and limits. Dashed UI curves show the relative selected source shape; the solid trace is the measured post-output FFT in dBFS/bin.

Keep legacy white/pink/brown parameter IDs and slots for migration. They are not sound controls in the new source. New parameters are `color`, `focus`, `sourceMode`, and `profile`; early blend state maps to a weighted exponent, not an exact reconstruction of its mixture.

## Realtime and state rules

Never allocate, lock, do file I/O, or call UI code in the audio callback. Cache parameter pointers, use fixed voice/filter storage, and preserve normal floating-point semantics (no fast-math). UI keyboard events use a bounded SPSC FIFO; never call MidiKeyboardState processing methods from the audio callback because they lock. Host MIDI is processed at its sample offset; skip sysex without constructing an allocating message. Display data is published via a second SPSC FIFO and atomics.

Play is transient and is not restored from sessions. State recall and panic silence voices. APVTS state serialization belongs outside the audio callback. Parameters are host-automatable and saved in DAW sessions. Keep all IDs stable once released.

## Style

Allman braces, four spaces, repository `.clang-format`. Use the sibling projects as references without editing them. Preserve the source/artifact attribution in `docs/icon.md`.
