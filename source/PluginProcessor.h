#pragma once
#include "dsp/NoiseEngine.h"
#include <juce_audio_utils/juce_audio_utils.h>

class PluginProcessor final : public juce::AudioProcessor, private juce::MidiKeyboardStateListener
{
public:
    PluginProcessor();
    ~PluginProcessor() override;
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "Noise Explorer"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Noise Explorer"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    juce::AudioProcessorValueTreeState parameters;
    juce::MidiKeyboardState keyboard;
    std::atomic<float> outputPeak { 0.0f };
    std::atomic<int> soundingVoices { 0 }, lastNote { 60 };
    std::atomic<bool> playing { false };
    void setPlaying (bool shouldPlay) { playing.store (shouldPlay); }
    void panic()
    {
        panicRequested.store (true);
        playing.store (false);
    }
    int readScope (float* destination, int maximum);
    void applyPreset (int index);
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    static juce::String filterId (int index, const juce::String& suffix) { return "f" + juce::String (index + 1) + suffix; }

private:
    struct FilterPointers
    {
        std::atomic<float>* enabled;
        std::atomic<float>* type;
        std::atomic<float>* frequency;
        std::atomic<float>* q;
        std::atomic<float>* gain;
        std::array<std::atomic<float>*, 4> envelope;
    };
    std::array<FilterPointers, 4> filterPointers;
    std::array<std::atomic<float>*, 4> ampPointers;
    std::atomic<float>*colorPointer, *focusPointer, *sourceModePointer, *profilePointer;
    std::atomic<float>* modePointer;
    std::atomic<float>* widthPointer;
    std::atomic<float>* drivePointer;
    std::atomic<float>* volumePointer;
    noise::Spectrum sourceSpectrum;
    std::array<noise::Voice, 8> voices;
    std::array<noise::FilterSettings, 4> settings;
    juce::SmoothedValue<float> color, focus;
    std::array<juce::SmoothedValue<float>, 4> frequency, resonance, filterGain, filterEnabled;
    juce::SmoothedValue<float> width, drive, volume;
    std::array<bool, 16> sustain {};
    std::array<float, 16> pitchBend {};
    struct KeyEvent
    {
        int note = 60, channel = 1;
        float velocity = 0.0f;
        bool on = false;
    };
    juce::AbstractFifo keyFifo { 256 };
    std::array<KeyEvent, 256> keyEvents;
    void handleNoteOn (juce::MidiKeyboardState*, int channel, int note, float velocity) override;
    void handleNoteOff (juce::MidiKeyboardState*, int channel, int note, float velocity) override;
    void enqueueKey (KeyEvent);
    juce::AbstractFifo scopeFifo { 16384 };
    std::array<float, 16384> scopeData {};
    std::atomic<bool> panicRequested { false };
    double rate = 44100.0;
    uint64_t ageCounter = 0;
    int previousMode = 0, controlCounter = 0;
    bool droneWasPlaying = false;
    float peak = 0.0f;
    void updateParameters();
    void handleMidi (const juce::MidiMessage&, bool instrument);
    void updateCoefficients();
    void render (juce::AudioBuffer<float>&, int start, int end);
    void resetVoices();
    void publishScope (float);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
