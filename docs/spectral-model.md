# Source spectrum and normalization

Noise Explorer offers two controls for one source spectrum. **Color** sets a continuous power-law exponent from -2 to +2. **Spectrum X/Y** adds a signed, broad midrange contour: X controls the same exponent; Y moves from a scoop to a focus around 1 kHz, from -24 to +24 dB with a 1.25-octave Gaussian width. The color buttons select exponent anchors; they do not add generators or gain.

| Color | Exponent alpha | Ideal PSD tilt |
| --- | --- | --- |
| Violet / purple | -2 | +6.02 dB/octave |
| Blue | -1 | +3.01 dB/octave |
| White | 0 | 0 dB/octave |
| Pink | 1 | -3.01 dB/octave |
| Brown / red | 2 | -6.02 dB/octave |

Power density is proportional to `f^-alpha`; synthesis amplitudes use its square root. Arbitrary exponents between the anchors produce intermediate slopes. This distinction between power and amplitude follows [Julius O. Smith's spectrum analysis treatment](https://www.dsprelated.com/freebooks/sasp/Spectrum_Analysis_Noise.html). Mixing white and brown would produce a sum of spectra rather than the uniform slope of pink.

Two optional profiles multiply that spectrum:

- **Green band:** a log-frequency Gaussian centered at 600 Hz, with a 1.4-octave width. This is our defined broad midrange color; it is not a standardized power-law exponent.
- **Gray weighting:** a bounded inverse-A-weighting power contour, capped between -12 and +18 dB. This is an approximation to a perceptual contour, not a calibrated equal-loudness guarantee for every listener or listening level. A-weighting frequency sensitivity is described in [MathWorks' weighting filter documentation](https://www.mathworks.com/help/audio/ug/audio-weighting-filters.html).

Color tilt and the X/Y contour apply to every profile. Green and gray presets use alpha = 0 so their profile shapes can be heard directly.

## Constant source power

For every color/profile/contour combination, the total spectral energy determines a normalization gain. Each independent mono stream has an expected RMS of 0.4 before envelopes, filters, drive and master output. The stereo width uses equal-power coefficients. There is no user blend-gain sum and no signal-following AGC that would swell between notes.

This holds **average source power** steady, rather than forcing every sample or short window to the same level. Noise peaks fluctuate; different spectra can also sound differently loud. Envelopes, velocity, filter choices, saturation and master level remain intentional sound-design controls. A transparent output stage below 0.9 full scale and a soft knee above it bounds output below 0.999 full scale, including polyphony and high resonance.

## Finite-band synthesis

A fixed 4096-point inverse transform synthesizes random Hermitian spectra. Independent real and imaginary components have variance 1/3; normalization includes both conjugate bins and the inverse-transform scale. Successive independent frames overlap at 50% with square-root Hann windows whose squared sum is one, preserving expected power through transitions. There are no allocations, locks, backend FFT initialization, or UI calls during audio processing.

The spectral model flattens below 20 Hz, excludes DC and Nyquist bins, and rolls off over the top 5% of `min(20 kHz, 0.45 * sample rate)`. Finite windows and frequency resolution make this an approximation to the ideal continuous PSD. Color and contour changes smooth over 80 ms and update through the overlapping frames. Prewarmed, staggered streams let MIDI envelopes start at their sample offsets without waiting for an FFT frame.

The dashed observatory curve is the **relative selected source shape**, anchored at 1 kHz. It is not a target dBFS trace. The solid curve measures the actual output after envelopes, filters, drive and master level.

## Sessions

New source parameters are saved and automatable. Original white/pink/brown parameter slots remain for compatibility but no longer control the sound. Early blend sessions migrate to a weighted exponent: `(pink + 2*brown) / (white + pink + brown)`. This approximates their tone and cannot exactly preserve non-power-law mixtures. Playback remains stopped on recall.
