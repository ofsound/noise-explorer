#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
    void addFloat (Layout& layout, const juce::String& id, const juce::String& name, float low, float high, float value, float skew = 1.0f)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> { low, high, 0.0f, skew }, value));
    }
    juce::ADSR::Parameters envelopeFrom (const std::array<std::atomic<float>*, 4>& values)
    {
        return { values[0]->load(), values[1]->load(), values[2]->load(), values[3]->load() };
    }
    void setEnvelope (juce::ADSR& env, const juce::ADSR::Parameters& p)
    {
        const auto& old = env.getParameters();
        if (!juce::approximatelyEqual (old.attack, p.attack) || !juce::approximatelyEqual (old.decay, p.decay) || !juce::approximatelyEqual (old.sustain, p.sustain) || !juce::approximatelyEqual (old.release, p.release))
            env.setParameters (p);
    }
    void addEnvelope (Layout& layout, const juce::String& prefix, const juce::String& name)
    {
        addFloat (layout, prefix + "Attack", name + " Attack", 0.002f, 5.0f, 0.025f, 0.3f);
        addFloat (layout, prefix + "Decay", name + " Decay", 0.01f, 5.0f, 0.3f, 0.35f);
        addFloat (layout, prefix + "Sustain", name + " Sustain", 0.0f, 1.0f, 0.75f);
        addFloat (layout, prefix + "Release", name + " Release", 0.01f, 10.0f, 0.8f, 0.3f);
    }
}

Layout PluginProcessor::createParameters()
{
    Layout layout;
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 }, "Mode", juce::StringArray { "Continuous", "Instrument" }, 0));
    addFloat (layout, "white", "Legacy white (migration)", 0.0f, 1.0f, 0.0f);
    addFloat (layout, "pink", "Legacy pink (migration)", 0.0f, 1.0f, 1.0f);
    addFloat (layout, "brown", "Legacy brown (migration)", 0.0f, 1.0f, 0.0f);
    addFloat (layout, "width", "Stereo width", 0.0f, 1.0f, 0.65f);
    addFloat (layout, "drive", "Saturation", 0.0f, 1.0f, 0.0f);
    addFloat (layout, "volume", "Output level", -60.0f, 0.0f, -18.0f);
    addEnvelope (layout, "amp", "Amplitude");
    for (int i = 0; i < 4; ++i)
    {
        const auto prefix = "Filter " + juce::String (i + 1);
        layout.add (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { filterId (i, "On"), 1 }, prefix + " Enabled", false));
        layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { filterId (i, "Type"), 1 }, prefix + " Type", juce::StringArray { "Low pass", "High pass", "Bell", "Notch", "Resonator" }, i));
        addFloat (layout, filterId (i, "Freq"), prefix + " Frequency", 20.0f, 20000.0f, i == 0 ? 6000.0f : (i == 1 ? 120.0f : 1000.0f), 0.22f);
        addFloat (layout, filterId (i, "Q"), prefix + " Resonance", 0.5f, 80.0f, 0.707f, 0.3f);
        addFloat (layout, filterId (i, "Gain"), prefix + " Bell gain", -18.0f, 18.0f, 0.0f);
        addEnvelope (layout, filterId (i, ""), prefix);
    }
    // Keep the original parameter slots for session compatibility; new sessions use color.
    addFloat (layout, "color", "Spectral exponent", -2.0f, 2.0f, 1.0f);
    addFloat (layout, "focus", "Midrange contour", -24.0f, 24.0f, 0.0f);
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "sourceMode", 1 }, "Spectrum control", juce::StringArray { "Color", "Spectrum XY" }, 0));
    layout.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "profile", 1 }, "Spectral profile", juce::StringArray { "Power law", "Green band", "Gray weighting" }, 0));
    return layout;
}

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "NoiseExplorerState", createParameters())
{
    const auto raw = [this] (const juce::String& id) { return parameters.getRawParameterValue (id); };
    modePointer = raw ("mode");
    widthPointer = raw ("width");
    drivePointer = raw ("drive");
    volumePointer = raw ("volume");
    colorPointer = raw ("color");
    focusPointer = raw ("focus");
    sourceModePointer = raw ("sourceMode");
    profilePointer = raw ("profile");
    ampPointers = { raw ("ampAttack"), raw ("ampDecay"), raw ("ampSustain"), raw ("ampRelease") };
    for (int i = 0; i < 4; ++i)
    {
        auto& p = filterPointers[static_cast<size_t> (i)];
        p = { raw (filterId (i, "On")), raw (filterId (i, "Type")), raw (filterId (i, "Freq")), raw (filterId (i, "Q")), raw (filterId (i, "Gain")), { raw (filterId (i, "Attack")), raw (filterId (i, "Decay")), raw (filterId (i, "Sustain")), raw (filterId (i, "Release")) } };
    }
    keyboard.addListener (this);
}
PluginProcessor::~PluginProcessor() { keyboard.removeListener (this); }

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    rate = sampleRate;
    sourceSpectrum.prepare (rate, colorPointer->load(), sourceModePointer->load() > 0.5f ? focusPointer->load() : 0.0f, static_cast<int> (profilePointer->load()));
    for (size_t i = 0; i < voices.size(); ++i)
        voices[i].prepare (rate, 1234567u + static_cast<uint32_t> (i) * 31337u, sourceSpectrum, static_cast<int> (i));
    for (auto* smooth : { &width, &drive, &volume })
        smooth->reset (rate, 0.025);
    for (auto* smooth : { &color, &focus })
        smooth->reset (rate, 0.08);
    for (auto* group : { &frequency, &resonance, &filterGain, &filterEnabled })
        for (auto& smooth : *group)
            smooth.reset (rate, 0.04);
    updateParameters();
    for (auto* smooth : { &width, &drive, &volume })
        smooth->setCurrentAndTargetValue (smooth->getTargetValue());
    for (auto* smooth : { &color, &focus })
        smooth->setCurrentAndTargetValue (smooth->getTargetValue());
    for (auto* group : { &frequency, &resonance, &filterGain, &filterEnabled })
        for (auto& smooth : *group)
            smooth.setCurrentAndTargetValue (smooth.getTargetValue());
    previousMode = static_cast<int> (modePointer->load());
    resetVoices();
    controlCounter = 0;
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layout) const
{
    return layout.getMainInputChannelSet().isDisabled()
           && (layout.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() || layout.getMainOutputChannelSet() == juce::AudioChannelSet::mono());
}
void PluginProcessor::resetVoices()
{
    for (auto& voice : voices)
        voice.reset();
    sustain.fill (false);
    pitchBend.fill (0.0f);
    droneWasPlaying = false;
}
void PluginProcessor::updateParameters()
{
    color.setTargetValue (colorPointer->load());
    focus.setTargetValue (sourceModePointer->load() > 0.5f ? focusPointer->load() : 0.0f);
    width.setTargetValue (widthPointer->load());
    drive.setTargetValue (drivePointer->load());
    volume.setTargetValue (juce::Decibels::decibelsToGain (volumePointer->load()));
    const auto amp = envelopeFrom (ampPointers);
    for (auto& voice : voices)
        setEnvelope (voice.amplitude, amp);
    for (size_t i = 0; i < settings.size(); ++i)
    {
        const auto& p = filterPointers[i];
        auto& s = settings[i];
        const auto newType = static_cast<int> (p.type->load());
        if (newType != s.type)
            for (auto& voice : voices)
                for (auto& f : voice.filters[i])
                    f.reset();
        s.enabled = p.enabled->load() > 0.5f;
        s.type = newType;
        s.frequency = p.frequency->load();
        s.q = p.q->load();
        s.gain = p.gain->load();
        s.envelope = envelopeFrom (p.envelope);
        frequency[i].setTargetValue (s.frequency);
        resonance[i].setTargetValue (s.q);
        filterGain[i].setTargetValue (s.gain);
        filterEnabled[i].setTargetValue (s.enabled ? 1.0f : 0.0f);
        for (auto& voice : voices)
            setEnvelope (voice.envelopes[i], s.envelope);
    }
}
void PluginProcessor::enqueueKey (KeyEvent event)
{
    const auto scope = keyFifo.write (1);
    if (scope.blockSize1 > 0)
        keyEvents[static_cast<size_t> (scope.startIndex1)] = event;
    else
        panicRequested.store (true); // overflow must not leave a held note
}
void PluginProcessor::handleNoteOn (juce::MidiKeyboardState*, int channel, int note, float velocity)
{
    enqueueKey ({ note, channel, velocity, true });
}
void PluginProcessor::handleNoteOff (juce::MidiKeyboardState*, int channel, int note, float velocity)
{
    enqueueKey ({ note, channel, velocity, false });
}

void PluginProcessor::handleMidi (const juce::MidiMessage& message, bool instrument)
{
    const auto channel = message.getChannel();
    if (channel < 1 || channel > 16)
        return;
    const auto c = static_cast<size_t> (channel - 1);
    if (message.isNoteOn())
    {
        lastNote.store (message.getNoteNumber());
        if (!instrument)
        {
            voices[0].note = message.getNoteNumber();
            voices[0].channel = channel;
            controlCounter = 0;
            return;
        }
        auto* chosen = &voices[0];
        for (auto& voice : voices)
        {
            if (!voice.active)
            {
                chosen = &voice;
                break;
            }
            if (voice.age < chosen->age)
                chosen = &voice;
        }
        chosen->start (message.getNoteNumber(), channel, message.getFloatVelocity(), ++ageCounter);
        controlCounter = 0;
    }
    else if (message.isNoteOff() && instrument)
    {
        for (auto& voice : voices)
            if (voice.active && voice.held && voice.note == message.getNoteNumber() && voice.channel == channel)
            {
                voice.held = false;
                if (sustain[c])
                    voice.sustained = true;
                else
                    voice.stop();
            }
    }
    else if (message.isSustainPedalOn())
        sustain[c] = true;
    else if (message.isSustainPedalOff())
    {
        sustain[c] = false;
        for (auto& voice : voices)
            if (voice.channel == channel && voice.sustained)
                voice.stop();
    }
    else if (message.isAllSoundOff())
    {
        for (auto& voice : voices)
            if (voice.channel == channel)
                voice.reset();
        sustain[c] = false;
    }
    else if (message.isAllNotesOff())
    {
        for (auto& voice : voices)
            if (voice.channel == channel)
                voice.stop();
        sustain[c] = false;
    }
    else if (message.isPitchWheel())
    {
        pitchBend[c] = static_cast<float> (message.getPitchWheelValue() - 8192) / 8192.0f * 2.0f;
        controlCounter = 0;
    }
}
void PluginProcessor::updateCoefficients()
{
    for (auto& voice : voices)
    {
        if (!voice.active)
            continue;
        for (size_t i = 0; i < settings.size(); ++i)
        {
            const auto tuned = 440.0f * std::pow (2.0f, (static_cast<float> (voice.note) - 69.0f + pitchBend[static_cast<size_t> (voice.channel - 1)]) / 12.0f);
            const auto hz = settings[i].type == 4 ? tuned : frequency[i].getCurrentValue();
            for (auto& filter : voice.filters[i])
                filter.configure (rate, hz, resonance[i].getCurrentValue(), filterGain[i].getCurrentValue(), settings[i].type);
        }
    }
}
void PluginProcessor::publishScope (float value)
{
    const auto scope = scopeFifo.write (1);
    if (scope.blockSize1 > 0)
        scopeData[static_cast<size_t> (scope.startIndex1)] = value;
}
int PluginProcessor::readScope (float* destination, int maximum)
{
    const auto scope = scopeFifo.read (maximum);
    std::copy_n (scopeData.data() + scope.startIndex1, scope.blockSize1, destination);
    std::copy_n (scopeData.data() + scope.startIndex2, scope.blockSize2, destination + scope.blockSize1);
    return scope.blockSize1 + scope.blockSize2;
}
void PluginProcessor::render (juce::AudioBuffer<float>& buffer, int start, int end)
{
    for (int sample = start; sample < end; ++sample)
    {
        const auto stereo = width.getNextValue();
        const auto saturation = drive.getNextValue();
        const auto level = volume.getNextValue();
        float activeFilters = 0.0f;
        std::array<float, 4> enabled;
        for (size_t i = 0; i < settings.size(); ++i)
        {
            frequency[i].getNextValue();
            resonance[i].getNextValue();
            filterGain[i].getNextValue();
            enabled[i] = filterEnabled[i].getNextValue();
            activeFilters += enabled[i];
        }
        if (controlCounter-- <= 0)
        {
            updateCoefficients();
            controlCounter = 15;
        }
        std::array<float, 2> output {};
        for (auto& voice : voices)
        {
            // Keep independent streams warm so MIDI attacks never wait for a spectral frame.
            std::array<float, 2> raw { voice.generators[0].next(), voice.generators[1].next() };
            if (!voice.active)
                continue;
            const auto amp = voice.amplitude.getNextSample();
            std::array<float, 4> envelopes;
            bool alive = voice.amplitude.isActive();
            for (size_t i = 0; i < envelopes.size(); ++i)
            {
                envelopes[i] = voice.envelopes[i].getNextSample();
                alive = alive || (enabled[i] > 0.001f && settings[i].type == 4 && voice.envelopes[i].isActive());
            }
            if (!alive)
            {
                voice.active = false;
                continue;
            }
            const auto right = raw[0] * std::cos (stereo * juce::MathConstants<float>::halfPi) + raw[1] * std::sin (stereo * juce::MathConstants<float>::halfPi);
            raw[1] = right;
            for (size_t ch = 0; ch < output.size(); ++ch)
            {
                float filtered = raw[ch] * amp * juce::jmax (0.0f, 1.0f - activeFilters);
                for (size_t i = 0; i < settings.size(); ++i)
                {
                    if (enabled[i] < 0.00001f)
                        continue;
                    const auto env = settings[i].type == 4 ? envelopes[i] : amp;
                    filtered += voice.filters[i][ch].process (raw[ch]) * env * enabled[i];
                }
                output[ch] += filtered / juce::jmax (1.0f, activeFilters) * voice.velocity;
            }
        }
        for (auto& value : output)
        {
            // Smooth analog-style saturation plus a final bounded output safety stage.
            const auto driven = std::tanh (value * (1.0f + saturation * 7.0f)) / (1.0f + saturation * 2.0f);
            value = (value + saturation * (driven - value)) * level;
            // Unity below -0.9 dBFS, with a soft knee only on hot peaks.
            const auto magnitude = std::abs (value);
            if (magnitude > 0.9f)
                value = std::copysign (0.9f + 0.099f * std::tanh ((magnitude - 0.9f) / 0.099f), value);
            if (!std::isfinite (value))
            {
                value = 0.0f;
                resetVoices();
            }
            peak = juce::jmax (peak, std::abs (value));
        }
        if (buffer.getNumChannels() == 1)
            buffer.setSample (0, sample, (output[0] + output[1]) * 0.5f);
        else if (buffer.getNumChannels() >= 2)
        {
            buffer.setSample (0, sample, output[0]);
            buffer.setSample (1, sample, output[1]);
        }
        publishScope ((output[0] + output[1]) * 0.5f);
    }
}
void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    updateParameters();
    sourceSpectrum.update (color.skip (buffer.getNumSamples()), focus.skip (buffer.getNumSamples()), static_cast<int> (profilePointer->load()));
    const auto mode = static_cast<int> (modePointer->load());
    const auto shouldPanic = panicRequested.exchange (false);
    if (mode != previousMode || shouldPanic)
    {
        resetVoices();
        // Discard events queued before a panic/mode transition, including overflow.
        const auto discardedKeys = keyFifo.read (keyFifo.getNumReady());
        juce::ignoreUnused (discardedKeys);
        previousMode = mode;
    }
    const bool instrument = mode == 1;
    const auto play = playing.load() && !instrument;
    if (play && !droneWasPlaying)
        voices[0].start (lastNote.load(), 1, 1.0f, ++ageCounter);
    else if (!play && droneWasPlaying)
        voices[0].stop();
    droneWasPlaying = play;
    {
        const auto keys = keyFifo.read (keyFifo.getNumReady());
        keys.forEach ([&] (int index) {
            const auto& event = keyEvents[static_cast<size_t> (index)];
            handleMidi (event.on ? juce::MidiMessage::noteOn (event.channel, event.note, event.velocity) : juce::MidiMessage::noteOff (event.channel, event.note), instrument);
        });
    }
    peak = 0.0f;
    int cursor = 0;
    for (const auto metadata : midi)
    {
        const auto position = juce::jlimit (cursor, buffer.getNumSamples(), metadata.samplePosition);
        render (buffer, cursor, position);
        // Ignore sysex without constructing a potentially allocating MidiMessage.
        if (metadata.numBytes <= 3)
            handleMidi (metadata.getMessage(), instrument);
        cursor = position;
    }
    render (buffer, cursor, buffer.getNumSamples());
    midi.clear();
    outputPeak.store (peak);
    int active = 0;
    for (const auto& voice : voices)
        if (voice.active)
            ++active;
    soundingVoices.store (active);
}
juce::AudioProcessorEditor* PluginProcessor::createEditor() { return new PluginEditor (*this); }
void PluginProcessor::getStateInformation (juce::MemoryBlock& destination)
{
    const auto xml = parameters.copyState().createXml();
    copyXmlToBinary (*xml, destination);
}
void PluginProcessor::setStateInformation (const void* data, int size)
{
    if (size <= 0 || size > 1024 * 1024)
        return;
    const auto xml = getXmlFromBinary (data, size);
    if (xml == nullptr || !xml->hasTagName (parameters.state.getType()))
        return;
    auto state = juce::ValueTree::fromXml (*xml);
    for (auto child : state)
    {
        const auto id = child.getProperty ("id").toString();
        auto* parameter = parameters.getParameter (id);
        const auto value = static_cast<float> (child.getProperty ("value"));
        if (parameter == nullptr || !std::isfinite (value))
            return;
        const auto& range = parameter->getNormalisableRange();
        child.setProperty ("value", juce::jlimit (range.start, range.end, value), nullptr);
    }
    if (!state.getChildWithProperty ("id", "color").isValid())
    {
        // Early three-source sessions migrate to the nearest weighted exponent.
        // A mixed spectrum is not mathematically identical to a power-law spectrum.
        const auto value = [&] (const char* id) { return static_cast<float> (state.getChildWithProperty ("id", id).getProperty ("value", 0.0f)); };
        const auto total = value ("white") + value ("pink") + value ("brown");
        for (const auto* id : { "color", "focus", "sourceMode", "profile" })
        {
            juce::ValueTree child ("PARAM");
            child.setProperty ("id", id, nullptr);
            child.setProperty ("value", juce::String (id) == "color" && total > 0.0f ? (value ("pink") + 2.0f * value ("brown")) / total : 0.0f, nullptr);
            state.appendChild (child, nullptr);
        }
    }
    parameters.replaceState (state);
    panic(); // sessions always reopen quietly
}
void PluginProcessor::applyPreset (int index)
{
    panic();
    const auto set = [this] (const juce::String& id, float value) {
        if (auto* p = parameters.getParameter (id))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 (value));
            p->endChangeGesture();
        }
    };
    for (auto* p : getParameters())
        if (auto* ranged = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            ranged->beginChangeGesture();
            ranged->setValueNotifyingHost (ranged->getDefaultValue());
            ranged->endChangeGesture();
        }
    if (index == 1)
    {
        set ("color", 0);
    }
    if (index == 2)
    {
        set ("color", 2);
        set ("width", 0.35f);
    }
    if (index == 3)
    {
        set ("mode", 1);
        set ("color", 0.35f);
        set ("f1On", 1);
        set ("f1Type", 4);
        set ("f1Q", 55);
        set ("f1Attack", 0.003f);
        set ("f1Decay", 1.2f);
        set ("f1Sustain", 0.0f);
        set ("f1Release", 1.8f);
        set ("volume", -12);
    }
    if (index == 4)
    {
        set ("mode", 1);
        set ("color", 1.4f);
        set ("f1On", 1);
        set ("f1Type", 4);
        set ("f1Q", 22);
        set ("f1Attack", 1.2f);
        set ("f1Release", 3.5f);
        set ("width", 1);
        set ("volume", -12);
    }
    if (index == 5)
    {
        set ("color", 0.7f);
        set ("f1On", 1);
        set ("f1Type", 0);
        set ("f1Freq", 1600);
        set ("f1Q", 2.5f);
        set ("drive", 0.25f);
        set ("ampAttack", 1.5f);
        set ("ampRelease", 2.5f);
    }
    if (index == 6)
        set ("color", -1);
    if (index == 7)
        set ("color", -2);
    if (index == 8 || index == 9)
    {
        set ("color", 0);
        set ("profile", static_cast<float> (index - 7));
    }
    if (index == 10)
    {
        set ("sourceMode", 1);
        set ("color", 0.5f);
        set ("focus", -18);
    }
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PluginProcessor(); }
