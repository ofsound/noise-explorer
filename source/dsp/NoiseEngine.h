#pragma once
#include <juce_dsp/juce_dsp.h>

namespace noise
{
    // A finite-band spectral model, shared by independent random streams.
    // The desired POWER density is f^-alpha; synthesis amplitudes are its square root.
    struct Spectrum
    {
        static constexpr int size = 4096, hop = size / 2;
        static constexpr float targetRms = 0.4f;
        std::array<std::complex<float>, hop> roots;
        std::array<unsigned, size> reversal;
        std::array<float, size> window;
        std::array<float, hop + 1> weights {}, logFrequency {}, profileGain {}, focusCurve {}, taper {};
        double rate = 48000.0;
        float alpha = 1.0f, focusDb = 0.0f;
        int profile = 0;

        // Green is a defined mid-band profile, gray a bounded inverse-A contour.
        // They are explicit design choices, not additional power-law exponents.
        static float profilePower (float frequency, int kind)
        {
            const auto f = juce::jmax (20.0f, frequency);
            if (kind == 1)
            {
                const auto octaves = std::log2 (f / 600.0f);
                return std::exp (-0.5f * octaves * octaves / (1.4f * 1.4f));
            }
            if (kind == 2)
            {
                const double f2 = static_cast<double> (f) * f;
                const auto response = 12194.0 * 12194.0 * f2 * f2
                                      / ((f2 + 20.6 * 20.6) * std::sqrt ((f2 + 107.7 * 107.7) * (f2 + 737.9 * 737.9)) * (f2 + 12194.0 * 12194.0));
                const auto aDb = 2.0 + 20.0 * std::log10 (response);
                return static_cast<float> (std::pow (10.0, juce::jlimit (-12.0, 18.0, -aDb) / 10.0));
            }
            return 1.0f;
        }
        static float powerAt (float frequency, float exponent, float contourDb, int kind)
        {
            const auto f = juce::jmax (20.0f, frequency);
            const auto octaves = std::log2 (f / 1000.0f);
            const auto contour = std::exp (-0.5f * octaves * octaves / (1.25f * 1.25f));
            return std::pow (f / 1000.0f, -exponent) * profilePower (f, kind) * juce::Decibels::decibelsToGain (contourDb * contour) * juce::Decibels::decibelsToGain (contourDb * contour);
        }
        void prepare (double sampleRate, float exponent, float contourDb, int kind)
        {
            rate = sampleRate;
            for (int i = 0; i < size; ++i)
            {
                unsigned bits = static_cast<unsigned> (i), reverse = 0;
                for (int bit = 0; bit < 12; ++bit)
                {
                    reverse = (reverse << 1) | (bits & 1u);
                    bits >>= 1;
                }
                reversal[static_cast<size_t> (i)] = reverse;
                window[static_cast<size_t> (i)] = std::sin (juce::MathConstants<float>::pi * (static_cast<float> (i) + 0.5f) / size);
            }
            for (int i = 0; i < hop; ++i)
            {
                const auto angle = 2.0f * juce::MathConstants<float>::pi * static_cast<float> (i) / size;
                roots[static_cast<size_t> (i)] = { std::cos (angle), std::sin (angle) };
            }
            const auto high = static_cast<float> (juce::jmin (20000.0, rate * 0.45));
            for (int bin = 1; bin < hop; ++bin)
            {
                const auto index = static_cast<size_t> (bin);
                const auto hz = static_cast<float> (rate * bin / size);
                const auto f = juce::jmax (20.0f, hz);
                logFrequency[index] = std::log (f / 1000.0f);
                const auto octave = std::log2 (f / 1000.0f);
                focusCurve[index] = std::exp (-0.5f * octave * octave / (1.25f * 1.25f));
                taper[index] = hz < high * 0.95f ? 1.0f : (hz >= high ? 0.0f : std::cos ((hz / high - 0.95f) * 10.0f * juce::MathConstants<float>::pi));
            }
            update (exponent, contourDb, kind, true);
        }
        void update (float exponent, float contourDb, int kind, bool force = false)
        {
            if (!force && juce::approximatelyEqual (alpha, exponent) && juce::approximatelyEqual (focusDb, contourDb) && profile == kind)
                return;
            const auto changeProfile = force || profile != kind;
            alpha = exponent;
            focusDb = contourDb;
            profile = kind;
            double energy = 0.0;
            for (int bin = 1; bin < hop; ++bin)
            {
                const auto index = static_cast<size_t> (bin);
                if (changeProfile)
                    profileGain[index] = std::sqrt (profilePower (static_cast<float> (rate * bin / size), profile));
                weights[index] = std::exp (-0.5f * alpha * logFrequency[index] + focusDb * focusCurve[index] * 2.302585093f / 20.0f) * profileGain[index] * taper[index];
                energy += static_cast<double> (weights[index]) * weights[index];
            }
            // Uniform independent complex components have variance 1/3 each.
            // Hermitian synthesis plus square-root Hann overlap preserves this power.
            const auto scale = static_cast<float> (targetRms * size / std::sqrt (4.0 / 3.0 * juce::jmax (energy, 1.0e-20)));
            for (auto& weight : weights)
                weight *= scale;
        }
    };

    struct Generator
    {
        uint32_t seed = 1;
        const Spectrum* spectrum = nullptr;
        std::array<std::complex<float>, Spectrum::size> frame {};
        std::array<float, Spectrum::hop> output {}, tail {};
        int cursor = 0;
        float random()
        {
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            return static_cast<float> (seed >> 8) / 8388608.0f - 1.0f;
        }
        void prepare (const Spectrum& plan, uint32_t initialSeed, int phaseOffset = 0)
        {
            spectrum = &plan;
            seed = initialSeed;
            cursor = phaseOffset;
            tail.fill (0.0f);
            generate();
            generate(); // warm the overlap outside rendering
        }
        void generate()
        {
            frame[0] = frame[Spectrum::hop] = { 0.0f, 0.0f };
            for (int bin = 1; bin < Spectrum::hop; ++bin)
            {
                const auto weight = spectrum->weights[static_cast<size_t> (bin)];
                frame[static_cast<size_t> (bin)] = { random() * weight, random() * weight };
                frame[static_cast<size_t> (Spectrum::size - bin)] = std::conj (frame[static_cast<size_t> (bin)]);
            }
            // Fixed radix-2 inverse transform. No backend locks, allocations or lazy setup.
            for (unsigned i = 0; i < Spectrum::size; ++i)
                if (spectrum->reversal[i] > i)
                    std::swap (frame[i], frame[spectrum->reversal[i]]);
            for (int length = 2; length <= Spectrum::size; length <<= 1)
            {
                const auto half = length / 2, step = Spectrum::size / length;
                for (int start = 0; start < Spectrum::size; start += length)
                    for (int bin = 0; bin < half; ++bin)
                    {
                        const auto index = static_cast<size_t> (start + bin);
                        const auto even = frame[index];
                        const auto odd = frame[index + static_cast<size_t> (half)] * spectrum->roots[static_cast<size_t> (bin * step)];
                        frame[index] = even + odd;
                        frame[index + static_cast<size_t> (half)] = even - odd;
                    }
            }
            for (int sample = 0; sample < Spectrum::hop; ++sample)
            {
                const auto i = static_cast<size_t> (sample), j = i + Spectrum::hop;
                output[i] = frame[i].real() / Spectrum::size * spectrum->window[i] + tail[i];
                tail[i] = frame[j].real() / Spectrum::size * spectrum->window[j];
            }
        }
        float next()
        {
            if (cursor >= Spectrum::hop)
            {
                generate();
                cursor = 0;
            }
            return output[static_cast<size_t> (cursor++)];
        }
    };

    // Topology-preserving state variable filter. Coefficients are interpolated at
    // the control rate upstream; state is independent for each voice and channel.
    struct Filter
    {
        float ic1 = 0.0f, ic2 = 0.0f;
        float a1 = 1.0f, a2 = 0.0f, a3 = 0.0f, k = 1.414f;
        float bellGain = 0.0f;
        int type = 0;
        void reset() { ic1 = ic2 = 0.0f; }
        void configure (double rate, float hz, float q, float gainDb, int newType)
        {
            type = newType;
            const auto g = std::tan (juce::MathConstants<float>::pi * juce::jlimit (5.0f, static_cast<float> (rate * 0.45), hz) / static_cast<float> (rate));
            k = 1.0f / juce::jlimit (0.5f, 80.0f, q);
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
            bellGain = juce::Decibels::decibelsToGain (gainDb) - 1.0f;
        }
        float process (float x)
        {
            const auto v3 = x - ic2;
            const auto v1 = a1 * ic1 + a2 * v3;
            const auto v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            switch (type)
            {
                case 0:
                    return v2;
                case 1:
                    return x - k * v1 - v2;
                case 2:
                    return x + bellGain * k * v1;
                case 3:
                    return x - k * v1;
                case 4:
                    return v1 * std::sqrt (k); // energy compensation for a playable noise resonator
                default:
                    return x;
            }
        }
    };

    struct FilterSettings
    {
        bool enabled = false;
        int type = 0;
        float frequency = 1000.0f, q = 0.707f, gain = 0.0f;
        juce::ADSR::Parameters envelope { 0.01f, 0.3f, 0.7f, 0.8f };
    };

    struct Voice
    {
        std::array<Generator, 2> generators;
        std::array<std::array<Filter, 2>, 4> filters;
        juce::ADSR amplitude;
        std::array<juce::ADSR, 4> envelopes;
        int note = 60, channel = 1;
        uint64_t age = 0;
        bool active = false, held = false, sustained = false;
        float velocity = 1.0f;

        void prepare (double rate, uint32_t seed, const Spectrum& spectrum, int voiceIndex)
        {
            for (size_t ch = 0; ch < generators.size(); ++ch)
                generators[ch].prepare (spectrum, seed + static_cast<uint32_t> (ch) * 7919u, (voiceIndex * 2 + static_cast<int> (ch)) * Spectrum::hop / 16);
            amplitude.setSampleRate (rate);
            for (auto& env : envelopes)
                env.setSampleRate (rate);
            reset();
        }
        void reset()
        {
            active = held = sustained = false;
            amplitude.reset();
            for (auto& env : envelopes)
                env.reset();
            for (auto& pair : filters)
                for (auto& f : pair)
                    f.reset();
        }
        void start (int newNote, int newChannel, float newVelocity, uint64_t newAge)
        {
            reset();
            note = newNote;
            channel = newChannel;
            velocity = newVelocity;
            age = newAge;
            active = held = true;
            amplitude.noteOn();
            for (auto& env : envelopes)
                env.noteOn();
        }
        void stop()
        {
            held = sustained = false;
            amplitude.noteOff();
            for (auto& env : envelopes)
                env.noteOff();
        }
    };
}
