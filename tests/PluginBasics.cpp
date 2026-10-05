#include <PluginEditor.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{
    void set (PluginProcessor& processor, const juce::String& id, float value)
    {
        auto* p = processor.parameters.getParameter (id);
        REQUIRE (p != nullptr);
        p->setValueNotifyingHost (p->convertTo0to1 (value));
    }
    float render (PluginProcessor& p, int blocks = 1, int blockSize = 256)
    {
        juce::AudioBuffer<float> audio (2, blockSize);
        juce::MidiBuffer midi;
        float peak = 0.0f;
        for (int i = 0; i < blocks; ++i)
        {
            p.processBlock (audio, midi);
            peak = juce::jmax (peak, audio.getMagnitude (0, audio.getNumSamples()));
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < blockSize; ++sample)
                {
                    REQUIRE (std::isfinite (audio.getSample (channel, sample)));
                    REQUIRE (std::abs (audio.getSample (channel, sample)) <= 1.0f);
                }
        }
        return peak;
    }
    void send (PluginProcessor& p, const juce::MidiMessage& message)
    {
        juce::AudioBuffer<float> audio (2, 64);
        juce::MidiBuffer midi;
        midi.addEvent (message, 0);
        p.processBlock (audio, midi);
    }
}
TEST_CASE ("Instrument identity and output layouts", "[plugin]")
{
    PluginProcessor p;
    REQUIRE (p.getName() == "Noise Explorer");
    REQUIRE (p.acceptsMidi());
    REQUIRE_FALSE (p.producesMidi());
    REQUIRE_FALSE (p.isMidiEffect());
    REQUIRE (p.getProgramName (0).isNotEmpty());
    REQUIRE (p.getTotalNumInputChannels() == 0);
    REQUIRE (p.getTotalNumOutputChannels() == 2);
}
TEST_CASE ("Continuous start, release, and panic", "[audio]")
{
    PluginProcessor p;
    p.prepareToPlay (48000, 256);
    REQUIRE (render (p) == 0.0f);
    p.setPlaying (true);
    REQUIRE (render (p, 5) > 0.005f);
    p.setPlaying (false);
    REQUIRE (render (p) > 0.0f);
    render (p, 200);
    REQUIRE (render (p) == 0.0f);
    p.setPlaying (true);
    REQUIRE (render (p, 5) > 0.0f);
    p.panic();
    REQUIRE (render (p) == 0.0f);
}
TEST_CASE ("MIDI note timing is sample accurate and output MIDI is cleared", "[midi]")
{
    PluginProcessor p;
    set (p, "mode", 1);
    p.prepareToPlay (48000, 512);
    juce::AudioBuffer<float> audio (2, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 69, 1.0f), 127);
    p.processBlock (audio, midi);
    REQUIRE (audio.getMagnitude (0, 0, 127) == 0.0f);
    REQUIRE (audio.getMagnitude (0, 128, 384) > 0.0f);
    REQUIRE (midi.isEmpty());
    REQUIRE (p.lastNote.load() == 69);
    REQUIRE (p.soundingVoices.load() == 1);
}
TEST_CASE ("Sustain pedal, channel isolation and all sound off", "[midi]")
{
    PluginProcessor p;
    set (p, "mode", 1);
    set (p, "ampRelease", 0.01f);
    p.prepareToPlay (48000, 256);
    send (p, juce::MidiMessage::noteOn (2, 60, 1.0f));
    send (p, juce::MidiMessage::noteOff (1, 60));
    render (p, 10);
    REQUIRE (p.soundingVoices.load() == 1);
    send (p, juce::MidiMessage::controllerEvent (2, 64, 127));
    send (p, juce::MidiMessage::noteOff (2, 60));
    render (p, 10);
    REQUIRE (p.soundingVoices.load() == 1);
    send (p, juce::MidiMessage::controllerEvent (2, 64, 0));
    render (p, 10);
    REQUIRE (p.soundingVoices.load() == 0);
    send (p, juce::MidiMessage::noteOn (1, 60, 1.0f));
    send (p, juce::MidiMessage::allSoundOff (1));
    REQUIRE (render (p) == 0.0f);
}
TEST_CASE ("Resonator has an independent release envelope", "[audio][midi]")
{
    PluginProcessor p;
    set (p, "mode", 1);
    set (p, "ampRelease", 0.01f);
    set (p, "f1On", 1);
    set (p, "f1Type", 4);
    set (p, "f1Q", 50);
    set (p, "f1Release", 1.0f);
    p.prepareToPlay (48000, 256);
    send (p, juce::MidiMessage::noteOn (1, 69, 1.0f));
    render (p, 20);
    send (p, juce::MidiMessage::noteOff (1, 69));
    render (p, 20);
    REQUIRE (render (p) > 0.0001f);
    render (p, 200);
    REQUIRE (render (p) == 0.0f);
}
TEST_CASE ("Eight voices remain finite at high resonance across sample rates", "[audio]")
{
    for (const auto rate : { 32000.0, 44100.0, 48000.0, 96000.0 })
    {
        PluginProcessor p;
        set (p, "mode", 1);
        set (p, "drive", 1);
        set (p, "volume", 0);
        for (int f = 0; f < 4; ++f)
        {
            set (p, PluginProcessor::filterId (f, "On"), 1);
            set (p, PluginProcessor::filterId (f, "Type"), 4);
            set (p, PluginProcessor::filterId (f, "Q"), 80);
        }
        p.prepareToPlay (rate, 257);
        for (int note = 24; note < 100; note += 5)
            send (p, juce::MidiMessage::noteOn (1, note, 1.0f));
        REQUIRE (p.soundingVoices.load() == 8);
        REQUIRE (render (p, 40, 257) > 0.0f);
    }
}
TEST_CASE ("Session roundtrip restores parameters but never starts noise", "[state]")
{
    PluginProcessor original;
    set (original, "color", -0.73f);
    set (original, "sourceMode", 1);
    set (original, "focus", -12);
    set (original, "profile", 2);
    set (original, "f3Type", 4);
    set (original, "f3Release", 3.25f);
    original.setPlaying (true);
    juce::MemoryBlock state;
    original.getStateInformation (state);
    PluginProcessor restored;
    restored.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    REQUIRE (restored.parameters.getRawParameterValue ("color")->load() == Catch::Approx (-0.73f));
    REQUIRE (restored.parameters.getRawParameterValue ("focus")->load() == Catch::Approx (-12));
    REQUIRE (restored.parameters.getRawParameterValue ("sourceMode")->load() == Catch::Approx (1));
    REQUIRE (restored.parameters.getRawParameterValue ("profile")->load() == Catch::Approx (2));
    REQUIRE (restored.parameters.getRawParameterValue ("f3Release")->load() == Catch::Approx (3.25f));
    REQUIRE_FALSE (restored.playing.load());
    restored.prepareToPlay (48000, 256);
    REQUIRE (render (restored) == 0.0f);
}
TEST_CASE ("Continuous spectral exponents produce the specified power-density slopes", "[dsp]")
{
    constexpr int size = 16384;
    juce::dsp::FFT fft (14);
    juce::dsp::WindowingFunction<float> window (size, juce::dsp::WindowingFunction<float>::hann);
    for (const auto alpha : { -2.0f, -1.5f, -1.0f, -0.5f, 0.0f, 0.5f, 1.0f, 1.5f, 2.0f })
    {
        noise::Spectrum spectrum;
        spectrum.prepare (48000, alpha, 0, 0);
        noise::Generator generator;
        generator.prepare (spectrum, 837194);
        std::array<double, 2> energy {};
        for (int frame = 0; frame < 12; ++frame)
        {
            std::array<float, size * 2> data {};
            for (int sample = 0; sample < size; ++sample)
                data[static_cast<size_t> (sample)] = generator.next();
            window.multiplyWithWindowingTable (data.data(), size);
            fft.performFrequencyOnlyForwardTransform (data.data());
            for (size_t band = 0; band < 2; ++band)
            {
                const auto low = band == 0 ? 200 : 1600;
                const auto begin = low * size / 48000, end = low * 2 * size / 48000;
                for (int bin = begin; bin < end; ++bin)
                    energy[band] += static_cast<double> (data[static_cast<size_t> (bin)]) * data[static_cast<size_t> (bin)] / (end - begin);
            }
        }
        const auto difference = 10.0 * std::log10 (energy[1] / energy[0]);
        INFO ("alpha " << alpha << " three-octave PSD difference: " << difference);
        REQUIRE (difference == Catch::Approx (-9.0309 * alpha).margin (1.0));
    }
}
TEST_CASE ("XY midrange contour matches the graph and measured spectrum", "[dsp]")
{
    constexpr int size = 16384;
    juce::dsp::FFT fft (14);
    juce::dsp::WindowingFunction<float> window (size, juce::dsp::WindowingFunction<float>::hann);
    for (const auto focus : { -24.0f, 0.0f, 24.0f })
    {
        noise::Spectrum spectrum;
        spectrum.prepare (48000, 0, focus, 0);
        noise::Generator generator;
        generator.prepare (spectrum, 837194);
        std::array<double, 2> energy {};
        for (int frame = 0; frame < 12; ++frame)
        {
            std::array<float, size * 2> data {};
            for (int sample = 0; sample < size; ++sample)
                data[static_cast<size_t> (sample)] = generator.next();
            window.multiplyWithWindowingTable (data.data(), size);
            fft.performFrequencyOnlyForwardTransform (data.data());
            for (size_t band = 0; band < 2; ++band)
            {
                const auto centre = band == 0 ? 1000 : 8000;
                for (int bin = (centre - 100) * size / 48000; bin < (centre + 100) * size / 48000; ++bin)
                    energy[band] += static_cast<double> (data[static_cast<size_t> (bin)]) * data[static_cast<size_t> (bin)];
            }
        }
        // The Gaussian contour is 1 at 1 kHz and exp(-2.88) three octaves above.
        const auto expected = static_cast<double> (focus) * (1.0 - std::exp (-2.88));
        REQUIRE (10.0 * std::log10 (energy[0] / energy[1]) == Catch::Approx (expected).margin (1.0));
        const auto graphDb = 10.0 * std::log10 (noise::Spectrum::powerAt (1000, 0, focus, 0) / noise::Spectrum::powerAt (8000, 0, focus, 0));
        REQUIRE (graphDb == Catch::Approx (expected).margin (0.01));
    }
}
TEST_CASE ("Color, green, gray and XY contours share the same RMS power", "[dsp]")
{
    for (const auto rate : { 32000.0, 48000.0, 96000.0 })
        for (int profile = 0; profile < 3; ++profile)
            for (const auto alpha : { -2.0f, 0.0f, 1.0f, 2.0f })
                for (const auto focus : { -24.0f, 0.0f, 24.0f })
                {
                    noise::Spectrum spectrum;
                    spectrum.prepare (rate, alpha, focus, profile);
                    noise::Generator generator;
                    generator.prepare (spectrum, 936217);
                    double energy = 0;
                    constexpr int count = 262144;
                    for (int sample = 0; sample < count; ++sample)
                    {
                        const auto value = generator.next();
                        energy += static_cast<double> (value) * value;
                    }
                    const auto rms = std::sqrt (energy / count);
                    INFO ("Fs " << rate << " alpha " << alpha << " focus " << focus << " profile " << profile << " RMS " << rms);
                    REQUIRE (rms == Catch::Approx (noise::Spectrum::targetRms).margin (0.025));
                }
}
TEST_CASE ("Spectrum sweeps and profile switches stay finite and output stays bounded", "[audio][dsp]")
{
    PluginProcessor p;
    set (p, "volume", 0);
    set (p, "ampSustain", 1);
    set (p, "sourceMode", 1);
    p.prepareToPlay (48000, 257);
    p.setPlaying (true);
    for (int step = 0; step < 180; ++step)
    {
        set (p, "color", -2.0f + 4.0f * static_cast<float> (step % 60) / 59.0f);
        set (p, "focus", step % 2 == 0 ? -24.0f : 24.0f);
        set (p, "profile", static_cast<float> (step / 60));
        REQUIRE (render (p, 2, 257) > 0.0f);
    }
}
TEST_CASE ("Early blend sessions migrate quietly to a continuous color", "[state]")
{
    PluginProcessor original;
    auto state = original.parameters.copyState();
    for (const auto* id : { "color", "focus", "sourceMode", "profile" })
        state.removeChild (state.getChildWithProperty ("id", id), nullptr);
    state.getChildWithProperty ("id", "white").setProperty ("value", 0.0f, nullptr);
    state.getChildWithProperty ("id", "pink").setProperty ("value", 0.25f, nullptr);
    state.getChildWithProperty ("id", "brown").setProperty ("value", 0.75f, nullptr);
    juce::MemoryBlock binary;
    juce::AudioProcessor::copyXmlToBinary (*state.createXml(), binary);
    PluginProcessor restored;
    restored.setStateInformation (binary.getData(), static_cast<int> (binary.getSize()));
    REQUIRE (restored.parameters.getRawParameterValue ("color")->load() == Catch::Approx (1.75f));
    REQUIRE (restored.parameters.getRawParameterValue ("sourceMode")->load() == Catch::Approx (0));
    REQUIRE_FALSE (restored.playing.load());
}
TEST_CASE ("Resonator impulse peak follows concert pitch", "[dsp]")
{
    constexpr int size = 65536;
    juce::dsp::FFT fft (16);
    for (const auto hz : { 110.0f, 440.0f, 1760.0f })
    {
        noise::Filter filter;
        filter.configure (48000, hz, 80, 0, 4);
        std::vector<float> data (size * 2, 0.0f);
        for (int sample = 0; sample < size; ++sample)
            data[static_cast<size_t> (sample)] = filter.process (sample == 0 ? 1.0f : 0.0f);
        fft.performFrequencyOnlyForwardTransform (data.data());
        const auto peak = std::max_element (data.begin(), data.begin() + size / 2);
        const auto measured = static_cast<float> (std::distance (data.begin(), peak)) * 48000.0f / size;
        REQUIRE (measured == Catch::Approx (hz).margin (1.0f));
    }
}
TEST_CASE ("Onscreen keyboard uses the bounded event queue", "[midi]")
{
    PluginProcessor p;
    set (p, "mode", 1);
    set (p, "ampRelease", 0.01f);
    p.prepareToPlay (48000, 256);
    p.keyboard.noteOn (1, 60, 1.0f);
    REQUIRE (render (p, 4) > 0.0f);
    p.keyboard.noteOff (1, 60, 0.0f);
    render (p, 8);
    REQUIRE (render (p) == 0.0f);
}
TEST_CASE ("Preset mode changes consume panic once without dropping the next note", "[midi]")
{
    PluginProcessor p;
    p.prepareToPlay (48000, 256);
    p.applyPreset (3);
    render (p);
    p.keyboard.noteOn (1, 69, 1.0f);
    REQUIRE (render (p, 10) > 0.0f);
    REQUIRE (p.lastNote.load() == 69);
}
TEST_CASE ("UI queue overflow cannot replay stuck notes after panic", "[midi]")
{
    PluginProcessor p;
    set (p, "mode", 1);
    p.prepareToPlay (48000, 256);
    for (int i = 0; i < 300; ++i)
        p.keyboard.noteOn (1, i % 128, 1.0f);
    REQUIRE (render (p) == 0.0f);
    REQUIRE (p.soundingVoices.load() == 0);
}
TEST_CASE ("Mono output and empty blocks are supported", "[audio]")
{
    PluginProcessor p;
    auto layout = p.getBusesLayout();
    layout.outputBuses.set (0, juce::AudioChannelSet::mono());
    REQUIRE (p.setBusesLayout (layout));
    p.prepareToPlay (44100, 256);
    p.setPlaying (true);
    juce::AudioBuffer<float> audio (1, 256), empty (1, 0);
    juce::MidiBuffer midi;
    p.processBlock (empty, midi);
    p.processBlock (audio, midi);
    REQUIRE (audio.getMagnitude (0, 256) > 0.0f);
}
TEST_CASE ("Render editor preview", "[.preview]")
{
    PluginProcessor p;
    p.setRateAndBufferSizeDetails (48000, 512);
    p.prepareToPlay (48000, 512);
    std::unique_ptr<juce::AudioProcessorEditor> editor (p.createEditor());
    for (int view = 0; view < 3; ++view)
    {
        if (view == 1)
        {
            p.applyPreset (3);
            render (p); // process the preset's pending panic
            p.keyboard.noteOn (1, 69, 1.0f);
            for (int block = 0; block < 40; ++block)
            {
                render (p, 1, 512);
                juce::MessageManager::getInstance()->runDispatchLoopUntil (2);
            }
            juce::MessageManager::getInstance()->runDispatchLoopUntil (80);
            REQUIRE (p.lastNote.load() == 69);
            REQUIRE (p.soundingVoices.load() == 1);
        }
        if (view == 2)
        {
            p.applyPreset (10);
            p.setPlaying (true);
            for (int block = 0; block < 80; ++block)
            {
                render (p, 1, 512);
                juce::MessageManager::getInstance()->runDispatchLoopUntil (2);
            }
            juce::MessageManager::getInstance()->runDispatchLoopUntil (80);
        }
        const auto snapshot = editor->createComponentSnapshot (editor->getLocalBounds(), true, 1.5f);
        const auto file = juce::File::getCurrentWorkingDirectory().getChildFile (view == 0 ? "docs/noise-explorer-preview.png" : (view == 1 ? "docs/noise-explorer-resonator.png" : "docs/noise-explorer-spectrum-xy.png"));
        file.deleteFile();
        juce::FileOutputStream stream (file);
        juce::PNGImageFormat png;
        REQUIRE (png.writeImageToStream (snapshot, stream));
    }
}
