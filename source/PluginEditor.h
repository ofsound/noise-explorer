#pragma once
#include "PluginProcessor.h"

namespace theme
{
    inline const juce::Colour background { 0xffe9ede9 }, panel { 0xfff7f8f2 }, ink { 0xff182d33 }, muted { 0xff71827f }, line { 0xffd4ded7 };
    inline const juce::Colour teal { 0xff176d64 }, whiteNoise { 0xff718d9e }, pink { 0xffcc726c }, brown { 0xffb48652 };
    void text (juce::Graphics&, const juce::String&, juce::Rectangle<float>, float, juce::Colour, int justification = juce::Justification::centredLeft);
    void card (juce::Graphics&, juce::Rectangle<float>);
    juce::Colour colorAt (float);
    juce::String colorName (float);
}
class ExplorerLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    ExplorerLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
};
class ParameterControl final : public juce::Component
{
public:
    ParameterControl (PluginProcessor&, const juce::String& id, const juce::String& label, juce::Colour colour, bool compact = false);
    void paint (juce::Graphics&) override;
    void resized() override;
    juce::Slider slider;

private:
    juce::String caption;
    juce::Colour accent;
    bool small;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};
class SpectrumPad final : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit SpectrumPad (PluginProcessor&);
    ~SpectrumPad() override;
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    PluginProcessor& processorRef;
    juce::RangedAudioParameter *color, *focus;
    bool dragging = false;
    void setPosition (juce::Point<float>);
};
class FilterCard final : public juce::Component
{
public:
    FilterCard (PluginProcessor&, int);
    void paint (juce::Graphics&) override;
    void resized() override;
    void refresh();

private:
    PluginProcessor& processorRef;
    int index;
    juce::TextButton enabled { "OFF" };
    juce::ComboBox type;
    ParameterControl frequency, resonance, gain;
    std::array<std::unique_ptr<ParameterControl>, 4> envelope;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> onAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    juce::Rectangle<float> graph;
};
class SpectrumDisplay final : public juce::Component
{
public:
    explicit SpectrumDisplay (PluginProcessor&);
    void paint (juce::Graphics&) override;
    void tick();

private:
    PluginProcessor& processorRef;
    juce::dsp::FFT fft { 12 };
    juce::dsp::WindowingFunction<float> window { 4096, juce::dsp::WindowingFunction<float>::hann };
    std::array<float, 4096> samples {};
    std::array<float, 8192> transform {};
    std::array<float, 2048> magnitudes {};
    int cursor = 0;
};
class PluginEditor final : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    PluginProcessor& processorRef;
    ExplorerLookAndFeel lookAndFeel;
    juce::TooltipWindow tooltips { this, 650 };
    SpectrumDisplay spectrum;
    SpectrumPad spectrumPad;
    ParameterControl color;
    std::array<juce::TextButton, 5> colorStops;
    ParameterControl width, drive, output;
    std::array<std::unique_ptr<ParameterControl>, 4> ampControls;
    std::array<std::unique_ptr<FilterCard>, 4> filters;
    juce::ComboBox mode, presets, sourceMode, profile;
    juce::TextButton play { "PLAY NOISE" }, panic { "Panic" };
    juce::MidiKeyboardComponent keys;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment, sourceModeAttachment, profileAttachment;
    juce::Rectangle<int> sourceBounds, ampBounds, outputBounds;
    float meter = 0.0f;
    void timerCallback() override;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
