# Validation

Reviewed October 5, 2026. Noise Explorer 0.1.0, macOS arm64, JUCE 9.0.1.

| Check | Result |
| --- | --- |
| Debug: Standalone, AU, VST3 | Built successfully; no compiler warnings |
| Release: Standalone, AU, VST3 | Built successfully; no compiler warnings |
| Catch2 via CTest, Debug | 17 of 17 tests passed |
| Catch2 via CTest, Release | 17 of 17 tests passed |
| Apple AU validator: `auval -v aumu Nexp Osnd` | AU VALIDATION SUCCEEDED |
| pluginval 1.0.4, Release VST3, strictness 5 | SUCCESS |
| Editor captures | Color, resonator and Spectrum X/Y views rendered and visually inspected |
| Local bundle signatures | App, AU, and VST3 verified with `codesign --verify --deep --strict` |
| Installed standalone | Reopened quietly; legacy patch migrated; color anchor, mode selector and X/Y drag checked |

The audio tests cover continuous start/stop, sample-accurate MIDI, sustain and channel isolation, independent resonator release, eight-voice high-Q processing at multiple sample rates, session recall and blend migration, nine fractional power-density slopes, X/Y contour agreement with the displayed curve, normalized RMS across 108 profile/tilt/contour/sample-rate combinations, hot spectrum sweeps, concert-pitch resonator tuning, keyboard events, preset transitions, queue overflow, mono output, and empty buffers.

Installed Release artifacts:

- `~/Applications/Noise Explorer.app`
- `~/Library/Audio/Plug-Ins/Components/Noise Explorer.component`
- `~/Library/Audio/Plug-Ins/VST3/Noise Explorer.vst3`

These are local development builds with ad-hoc signatures. Distribution signing/notarization, Windows/Linux execution, physical MIDI controllers, and end-to-end sessions in individual DAWs were not verified in this first pass.

The normalized source update replaces the three visible blend controls with continuous Color and selectable Spectrum X/Y modes. Tests measured source RMS within 0.025 of its 0.4 target across 108 combinations at 32, 48 and 96 kHz. Fractional PSD slopes matched their expected three-octave differences within 1 dB. Both builds passed all 17 tests; all three formats were rebuilt and reinstalled after the update.

The RMS specification applies before envelopes, filters, saturation and master gain; it does not promise identical subjective loudness or instantaneous peaks. The final soft limiter bounds output. See [the spectral model](spectral-model.md).
