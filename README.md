# Noise Explorer

A playable laboratory for noise, by **ofsound**. Move continuously from violet through blue, white, pink, and brown noise, sculpt a separate X/Y spectrum, and turn noise into notes with four resonant filters.

Built from the latest Pamplejuce starter checked into this repository, with JUCE **9.0.1** (the same pinned JUCE revision as Chord Runner), C++23, and a native JUCE interface.

## Play

- **Continuous:** press **Play Noise**. Press **Stop Noise** to release the envelope.
- **MIDI instrument:** choose the mode and play a MIDI controller or the on-screen keyboard. Click the keyboard to focus it, then use `A W S E D F T G Y H U J K` on the computer keyboard.
- In the standalone app, select your audio device and MIDI inputs under **Options → Audio/MIDI Settings**.
- In a DAW, load Noise Explorer as an **instrument** and route MIDI into its track. It generates its own audio and requires no audio input.
- **Panic** immediately clears all voices. New instances and recalled sessions open quietly.

The eleven starting points include the five power-law colors, green and gray profiles, Glass Keys, Velvet Organ, Distant Ocean, and a scooped X/Y texture. Presets apply parameter values; edited values are saved in the host session or standalone settings.

## Sound design

**Color dial** sets a continuous spectral exponent from -2 (violet) through blue (-1), white (0), pink (1), to brown/red (2). The color buttons jump to these anchors. **Spectrum X/Y** is a separate selectable mode: horizontal movement changes tilt, vertical movement moves between a broad midrange scoop and focus. Green and gray are selectable spectral profiles.

All source shapes normalize automatically to the same expected RMS power. Changing color or X/Y position redistributes energy instead of adding source volumes. Noise peaks and perceived loudness can still vary; envelopes and filter effects remain audible. The live FFT shows the post-output signal; its dashed curve shows the selected **relative source shape**. See [the spectral model](docs/spectral-model.md) for formulas, finite-band limits, gray weighting, and legacy session migration.

Stereo Width moves from identical left/right channels to independent noise. Saturation adds soft drive. Output defaults to -18 dB and is softly bounded below full scale. Noise is stochastic: levels fluctuate naturally.

The four filters operate **in parallel**, with enabled branches averaged together. With all four off, the source passes directly to the output. Every slot offers low pass, high pass, bell (±18 dB), notch, and resonator. The frequency/Q controls and response plots show the effect of each branch. High Q emphasizes a narrow band; bell gain is available in Bell mode.

**Resonator / MIDI** locks a branch's center frequency to the played note, A4 = 440 Hz, with ±2-semitone pitch bend. Frequency becomes read-only while following notes. Increase Q for a clearer pitch. Each resonator has independent attack, decay, sustain, and release controls. These envelopes shape each branch's amplitude; the master ADSR shapes the direct/classic-filter signal. Each voice remains active until all relevant tails finish.

MIDI mode provides eight voices, velocity response, per-channel sustain pedal, all-notes-off and all-sound-off support. Oldest voices are replaced at the polyphony limit. Continuous mode uses one voice, initially tuned to C4; incoming notes retune it while Play is active.

## Build

```sh
git submodule update --init --recursive
cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build Builds --parallel 4
ctest --test-dir Builds --output-on-failure
```

Formats: **Standalone, VST3, Audio Unit** (macOS). Identity: `com.ofsound.noiseexplorer`, manufacturer `Osnd`, plugin `Nexp`. The UI, custom icon, and audio engine are bundled locally; there is no web server or runtime network dependency.

Builds default to Debug for development. Use Release for music-making and CPU measurements:

```sh
cmake -B Builds/Release -G Ninja -DCMAKE_BUILD_TYPE=Release -DNOISE_EXPLORER_COPY_PLUGINS=OFF
cmake --build Builds/Release --parallel 4
```

By default CMake copies plugins into user plugin folders. Disable with `-DNOISE_EXPLORER_COPY_PLUGINS=OFF`. Standalone output is in `Builds/NoiseExplorer_artefacts/Debug/Standalone/Noise Explorer.app`, or under `Builds/Release/NoiseExplorer_artefacts/Release/Standalone/` for Release.

Install an already-built Release version for the current user:

```sh
./scripts/install-macos.sh
```

This installs the standalone into `~/Applications` and AU/VST3 into `~/Library/Audio/Plug-Ins`. Bundles receive ad-hoc local signatures after resources are embedded; no developer certificate is used.

macOS development CI builds all formats and runs tests, without IPP or distribution signing/notarization. Local builds are for development; cross-platform builds are configured but have not been validated on Windows/Linux.

## Verification

Catch2 covers silence/start/stop, sample-accurate MIDI, sustain/channel isolation, independent resonator tails, high-Q polyphony at several sample rates, state recall and migration, fractional spectral slopes, RMS normalization across profiles and sample rates, spectrum sweeps, resonator tuning, and the UI keyboard queue. `./Builds/Tests '[.preview]'` writes a UI snapshot to `docs/noise-explorer-preview.png` for layout review.

See [the contributor guide](CLAUDE.md) for architecture and realtime constraints, and [icon provenance](docs/icon.md) for the custom artwork.
