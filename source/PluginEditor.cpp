#include "PluginEditor.h"

namespace theme
{
    void text (juce::Graphics& g, const juce::String& value, juce::Rectangle<float> bounds, float size, juce::Colour colour, int justification)
    {
        g.setColour (colour);
        g.setFont (juce::FontOptions { size });
        g.drawText (value, bounds, justification, true);
    }
    void card (juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        g.setColour (panel);
        g.fillRoundedRectangle (bounds, 12.0f);
        g.setColour (line);
        g.drawRoundedRectangle (bounds.reduced (0.5f), 12.0f, 1.0f);
    }
    juce::Colour colorAt (float alpha)
    {
        const std::array<juce::Colour, 5> stops { juce::Colour (0xff9667ba), juce::Colour (0xff487bbb), whiteNoise, pink, brown };
        const auto position = juce::jlimit (0.0f, 4.0f, alpha + 2.0f);
        const auto index = juce::jmin (3, static_cast<int> (position));
        return stops[static_cast<size_t> (index)].interpolatedWith (stops[static_cast<size_t> (index + 1)], position - static_cast<float> (index));
    }
    juce::String colorName (float alpha)
    {
        const std::array<juce::String, 5> names { "Violet", "Blue", "White", "Pink", "Brown / Red" };
        const auto index = juce::jlimit (0, 4, static_cast<int> (std::round (alpha + 2.0f)));
        if (std::abs (alpha - static_cast<float> (index - 2)) < 0.025f)
            return names[static_cast<size_t> (index)];
        const auto lower = juce::jlimit (0, 3, static_cast<int> (std::floor (alpha + 2.0f)));
        return names[static_cast<size_t> (lower)] + " / " + names[static_cast<size_t> (lower + 1)];
    }

}
ExplorerLookAndFeel::ExplorerLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, theme::ink);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::thumbColourId, theme::teal);
    setColour (juce::ComboBox::backgroundColourId, theme::panel);
    setColour (juce::ComboBox::textColourId, theme::ink);
    setColour (juce::ComboBox::outlineColourId, theme::line);
    setColour (juce::ComboBox::arrowColourId, theme::teal);
    setColour (juce::PopupMenu::backgroundColourId, theme::panel);
    setColour (juce::PopupMenu::textColourId, theme::ink);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, theme::teal);
    setColour (juce::PopupMenu::highlightedTextColourId, theme::panel);
    setColour (juce::TextButton::buttonColourId, theme::line);
    setColour (juce::TextButton::buttonOnColourId, theme::teal);
    setColour (juce::TextButton::textColourOffId, theme::ink);
    setColour (juce::TextButton::textColourOnId, theme::panel);
    setColour (juce::TooltipWindow::backgroundColourId, theme::ink);
    setColour (juce::TooltipWindow::textColourId, theme::panel);
}
juce::Font ExplorerLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return juce::Font { juce::FontOptions { 13.0f } }; }
juce::Font ExplorerLookAndFeel::getComboBoxFont (juce::ComboBox&) { return juce::Font { juce::FontOptions { 13.0f } }; }
void ExplorerLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& colour, bool hover, bool down)
{
    auto fill = button.getToggleState() ? button.findColour (juce::TextButton::buttonOnColourId) : colour;
    if (hover)
        fill = fill.brighter (0.07f);
    if (down)
        fill = fill.darker (0.08f);
    g.setColour (fill.withMultipliedAlpha (button.isEnabled() ? 1.0f : 0.4f));
    g.fillRoundedRectangle (button.getLocalBounds().toFloat(), 7.0f);
}
void ExplorerLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float position, float start, float end, juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (5.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto accent = slider.findColour (juce::Slider::thumbColourId).withMultipliedAlpha (slider.isEnabled() ? 1.0f : 0.3f);
    juce::Path track, arc;
    track.addCentredArc (centre.x, centre.y, radius - 2, radius - 2, 0, start, end, true);
    g.setColour (theme::line);
    g.strokePath (track, juce::PathStrokeType (3.0f));
    const auto angle = start + position * (end - start);
    arc.addCentredArc (centre.x, centre.y, radius - 2, radius - 2, 0, start, angle, true);
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const auto body = juce::Rectangle<float> (radius * 1.5f, radius * 1.5f).withCentre (centre);
    g.setColour (theme::line.withAlpha (0.6f));
    g.fillEllipse (body.translated (0, 2));
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, body.getTopLeft(), juce::Colour (0xffe2e8e1), body.getBottomRight(), false));
    g.fillEllipse (body);
    g.setColour (theme::line);
    g.drawEllipse (body, 1.0f);
    const auto tip = centre.getPointOnCircumference (radius * 0.58f, angle);
    const auto inner = centre.getPointOnCircumference (radius * 0.29f, angle);
    g.setColour (accent);
    g.drawLine ({ inner, tip }, 3.0f);
}
void ExplorerLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float position, float, float, juce::Slider::SliderStyle, juce::Slider& slider)
{
    const auto mid = static_cast<float> (y) + static_cast<float> (h) * 0.5f;
    g.setColour (theme::line);
    g.fillRoundedRectangle (static_cast<float> (x), mid - 2, static_cast<float> (w), 4, 2);
    g.setColour (slider.findColour (juce::Slider::thumbColourId).withMultipliedAlpha (slider.isEnabled() ? 1.0f : 0.3f));
    g.fillRoundedRectangle (static_cast<float> (x), mid - 2, juce::jmax (1.0f, position - static_cast<float> (x)), 4, 2);
    g.fillEllipse (position - 4, mid - 4, 8, 8);
}

ParameterControl::ParameterControl (PluginProcessor& p, const juce::String& id, const juce::String& label, juce::Colour colour, bool compact)
    : caption (label), accent (colour), small (compact)
{
    slider.setSliderStyle (small ? juce::Slider::LinearHorizontal : juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 82, 18);
    slider.setColour (juce::Slider::thumbColourId, accent);
    slider.setColour (juce::Slider::textBoxTextColourId, theme::ink);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setName (label);
    slider.setTitle (label);
    slider.setTooltip (p.parameters.getParameter (id)->getName (100) + ". Drag to adjust; double-click to reset; click the value to type.");
    const auto* parameter = p.parameters.getParameter (id);
    slider.setDoubleClickReturnValue (true, parameter->convertFrom0to1 (parameter->getDefaultValue()));
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (p.parameters, id, slider);
    if (id.endsWith ("Freq"))
    {
        slider.textFromValueFunction = [] (double value) { return value >= 1000.0 ? juce::String (value / 1000.0, 2) + " kHz" : juce::String (value, 0) + " Hz"; };
        slider.valueFromTextFunction = [] (const juce::String& value) { return value.getDoubleValue() * (value.containsIgnoreCase ("k") ? 1000.0 : 1.0); };
    }
    else if (id.endsWith ("Attack") || id.endsWith ("Decay") || id.endsWith ("Release"))
    {
        slider.textFromValueFunction = [] (double value) { return value < 1.0 ? juce::String (value * 1000.0, 0) + " ms" : juce::String (value, 2) + " s"; };
        slider.valueFromTextFunction = [] (const juce::String& value) { return value.getDoubleValue() / (value.containsIgnoreCase ("ms") ? 1000.0 : 1.0); };
    }
    else if (id == "volume" || id.endsWith ("Gain"))
    {
        slider.textFromValueFunction = [] (double value) { return juce::String (value, 1) + " dB"; };
        slider.valueFromTextFunction = [] (const juce::String& value) { return value.getDoubleValue(); };
    }
    else if (id == "color")
    {
        slider.textFromValueFunction = [] (double value) { return juce::String (value, 2); };
        slider.valueFromTextFunction = [] (const juce::String& value) { return value.getDoubleValue(); };
    }
    else if (id.endsWith ("Q"))
    {
        slider.textFromValueFunction = [] (double value) { return juce::String (value, 2); };
        slider.valueFromTextFunction = [] (const juce::String& value) { return value.getDoubleValue(); };
    }
    else
    {
        slider.textFromValueFunction = [] (double value) { return juce::String (value * 100.0, 0) + "%"; };
        slider.valueFromTextFunction = [] (const juce::String& value) { return value.getDoubleValue() / 100.0; };
    }
    slider.updateText();
    addAndMakeVisible (slider);
}
void ParameterControl::paint (juce::Graphics& g)
{
    theme::text (g, caption, { 0, 0, static_cast<float> (getWidth()), 18 }, small ? 10.0f : 12.0f, isEnabled() ? theme::muted : theme::muted.withAlpha (0.4f), juce::Justification::centred);
}
void ParameterControl::resized() { slider.setBounds (getLocalBounds().withTrimmedTop (18)); }

SpectrumPad::SpectrumPad (PluginProcessor& p) : processorRef (p), color (p.parameters.getParameter ("color")), focus (p.parameters.getParameter ("focus"))
{
    setTitle ("Spectrum XY: color horizontally, midrange focus vertically");
    setTooltip ("X: Violet through Blue, White, Pink, Brown. Y: scoop to focus, -24 to +24 dB around 1 kHz. Source RMS normalizes automatically.");
}
SpectrumPad::~SpectrumPad()
{
    if (dragging)
    {
        color->endChangeGesture();
        focus->endChangeGesture();
    }
}
void SpectrumPad::paint (juce::Graphics& g)
{
    const auto area = getLocalBounds().toFloat().reduced (1);
    juce::ColourGradient gradient (theme::colorAt (-2).withAlpha (0.18f), area.getTopLeft(), theme::colorAt (2).withAlpha (0.18f), area.getTopRight(), false);
    for (int i = 1; i < 4; ++i)
        gradient.addColour (static_cast<double> (i) / 4.0, theme::colorAt (static_cast<float> (i - 2)).withAlpha (0.18f));
    g.setGradientFill (gradient);
    g.fillRoundedRectangle (area, 8);
    g.setColour (theme::line);
    g.drawRoundedRectangle (area, 8, 1);
    for (int i = 1; i < 4; ++i)
        g.drawVerticalLine (static_cast<int> (area.getX() + area.getWidth() * static_cast<float> (i) / 4.0f), area.getY(), area.getBottom());
    g.drawHorizontalLine (static_cast<int> (area.getCentreY()), area.getX(), area.getRight());
    theme::text (g, "FOCUS", area.reduced (8).withHeight (15), 9, theme::teal);
    theme::text (g, "SCOOP", area.reduced (8).withTop (area.getBottom() - 23), 9, theme::teal);
    theme::text (g, "AUTO RMS", area.reduced (8).withTop (area.getBottom() - 23), 9, theme::teal, juce::Justification::centredRight);
    const auto alpha = processorRef.parameters.getRawParameterValue ("color")->load();
    const auto contour = processorRef.parameters.getRawParameterValue ("focus")->load();
    const auto centre = juce::Point<float> { area.getX() + (alpha + 2.0f) / 4.0f * area.getWidth(), area.getY() + (24.0f - contour) / 48.0f * area.getHeight() };
    g.setColour (theme::panel);
    g.fillEllipse (juce::Rectangle<float> (17, 17).withCentre (centre));
    g.setColour (theme::colorAt (alpha));
    g.fillEllipse (juce::Rectangle<float> (11, 11).withCentre (centre));
    theme::text (g, "a " + juce::String (alpha, 2) + "  /  " + juce::String (contour, 1) + " dB", area.reduced (8).withHeight (15), 10, theme::ink, juce::Justification::centredRight);
}
void SpectrumPad::setPosition (juce::Point<float> position)
{
    const auto area = getLocalBounds().toFloat().reduced (1);
    color->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, (position.x - area.getX()) / area.getWidth()));
    focus->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, 1.0f - (position.y - area.getY()) / area.getHeight()));
    repaint();
}
void SpectrumPad::mouseDown (const juce::MouseEvent& event)
{
    dragging = true;
    color->beginChangeGesture();
    focus->beginChangeGesture();
    setPosition (event.position);
}
void SpectrumPad::mouseDrag (const juce::MouseEvent& event) { setPosition (event.position); }
void SpectrumPad::mouseUp (const juce::MouseEvent&)
{
    if (dragging)
    {
        color->endChangeGesture();
        focus->endChangeGesture();
        dragging = false;
    }
}

FilterCard::FilterCard (PluginProcessor& p, int slot)
    : processorRef (p), index (slot), frequency (p, PluginProcessor::filterId (slot, "Freq"), "FREQUENCY", theme::teal), resonance (p, PluginProcessor::filterId (slot, "Q"), "RESONANCE / Q", theme::teal), gain (p, PluginProcessor::filterId (slot, "Gain"), "BELL GAIN", theme::teal, true)
{
    enabled.setClickingTogglesState (true);
    enabled.setTooltip ("Enable this parallel filter branch. With all four off, hear the unfiltered source.");
    type.addItemList ({ "Low pass", "High pass", "Bell", "Notch", "Resonator / MIDI" }, 1);
    type.setTooltip ("Resonator follows MIDI pitch exactly (A4 = 440 Hz) and uses its own amplitude envelope. Pitch bend: +/- 2 semitones.");
    for (auto* component : std::initializer_list<juce::Component*> { &enabled, &type, &frequency, &resonance, &gain })
        addAndMakeVisible (component);
    const std::array<juce::String, 4> ids { "Attack", "Decay", "Sustain", "Release" };
    const std::array<juce::String, 4> labels { "A", "D", "S", "R" };
    for (size_t i = 0; i < envelope.size(); ++i)
    {
        envelope[i] = std::make_unique<ParameterControl> (p, PluginProcessor::filterId (slot, ids[i]), labels[i], theme::teal, true);
        addAndMakeVisible (*envelope[i]);
    }
    onAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.parameters, PluginProcessor::filterId (slot, "On"), enabled);
    typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.parameters, PluginProcessor::filterId (slot, "Type"), type);
    refresh();
}
void FilterCard::refresh()
{
    const auto resonator = type.getSelectedId() == 5;
    enabled.setButtonText (enabled.getToggleState() ? "ON" : "OFF");
    frequency.setVisible (!resonator);
    gain.setVisible (type.getSelectedId() == 3);
    for (auto& env : envelope)
        env->setVisible (resonator);
    repaint();
}
void FilterCard::paint (juce::Graphics& g)
{
    theme::card (g, getLocalBounds().toFloat());
    const auto active = enabled.getToggleState();
    g.setColour (active ? theme::teal : theme::line);
    g.fillEllipse (16, 19, 6, 6);
    theme::text (g, "FILTER 0" + juce::String (index + 1), { 29, 11, 150, 22 }, 12, theme::ink);
    const auto which = type.getSelectedId() - 1;
    // Plot the actual linear transfer function of the SVF at the current controls.
    const auto hz = which == 4 ? static_cast<float> (juce::MidiMessage::getMidiNoteInHertz (processorRef.lastNote.load())) : static_cast<float> (frequency.slider.getValue());
    const auto q = resonance.slider.getValue();
    const auto fs = processorRef.getSampleRate() > 0.0 ? processorRef.getSampleRate() : 44100.0;
    const auto cutoff = std::tan (juce::MathConstants<double>::pi * juce::jlimit (5.0, fs * 0.45, static_cast<double> (hz)) / fs);
    juce::Path curve;
    for (int pixel = 0; pixel < static_cast<int> (graph.getWidth()); ++pixel)
    {
        const auto f = 20.0 * std::pow (1000.0, static_cast<double> (pixel) / graph.getWidth());
        const auto ratio = std::tan (juce::MathConstants<double>::pi * juce::jmin (f, fs * 0.45) / fs) / cutoff;
        const std::complex<double> s { 0, ratio };
        const auto denominator = s * s + s / q + 1.0;
        std::complex<double> response = 1.0 / denominator;
        if (which == 1)
            response = s * s / denominator;
        if (which == 2)
            response = 1.0 + (juce::Decibels::decibelsToGain (gain.slider.getValue()) - 1.0) * s / q / denominator;
        if (which == 3)
            response = (s * s + 1.0) / denominator;
        if (which == 4)
            response = s / std::sqrt (q) / denominator;
        const auto db = juce::Decibels::gainToDecibels (static_cast<float> (std::abs (response)), -48.0f);
        const auto point = juce::Point<float> { graph.getX() + static_cast<float> (pixel), graph.getBottom() - juce::jlimit (0.0f, 1.0f, (db + 36.0f) / 60.0f) * graph.getHeight() };
        if (pixel == 0)
            curve.startNewSubPath (point);
        else
            curve.lineTo (point);
    }
    g.setColour (theme::line);
    g.drawHorizontalLine (static_cast<int> (graph.getY() + graph.getHeight() * 0.4f), graph.getX(), graph.getRight());
    g.setColour ((active ? theme::teal : theme::muted).withAlpha (0.8f));
    g.strokePath (curve, juce::PathStrokeType (1.8f));
    if (which == 4)
    {
        const auto noteArea = frequency.getBounds().toFloat();
        theme::text (g, "MIDI PITCH", noteArea.withHeight (18), 11, theme::muted, juce::Justification::centred);
        theme::text (g, juce::MidiMessage::getMidiNoteName (processorRef.lastNote.load(), true, true, 4), noteArea.withTrimmedTop (20).withHeight (42), 29, theme::teal, juce::Justification::centred);
        theme::text (g, juce::String (hz, 1) + " Hz", noteArea.withTrimmedTop (64).withHeight (20), 11, theme::muted, juce::Justification::centred);
        theme::text (g, "PER-NOTE ENVELOPE", { 10, 219, static_cast<float> (getWidth() - 20), 20 }, 10, theme::teal, juce::Justification::centred);
    }
    else if (which != 2)
    {
        const std::array<juce::String, 4> descriptions { "Keep the body. Soften the air.", "Clear the lows. Reveal the air.", "Sculpt a focused band of energy.", "Carve a narrow pocket of silence." };
        theme::text (g, descriptions[static_cast<size_t> (juce::jlimit (0, 3, which))], { 14, 232, static_cast<float> (getWidth() - 28), 20 }, 11, theme::muted, juce::Justification::centred);
        theme::text (g, "Select Resonator for a MIDI envelope", { 14, 254, static_cast<float> (getWidth() - 28), 20 }, 10, theme::muted, juce::Justification::centred);
    }
}
void FilterCard::resized()
{
    const auto w = getWidth();
    enabled.setBounds (w - 63, 12, 47, 23);
    type.setBounds (16, 43, w - 32, 29);
    graph = juce::Rectangle<int> (18, 84, w - 36, 38).toFloat();
    frequency.setBounds (20, 127, (w - 40) / 2, 89);
    resonance.setBounds (w / 2, 127, (w - 40) / 2, 89);
    gain.setBounds (25, 225, w - 50, 53);
    for (size_t i = 0; i < envelope.size(); ++i)
        envelope[i]->setBounds (12 + static_cast<int> (i) * (w - 24) / 4, 240, (w - 24) / 4, 53);
}

SpectrumDisplay::SpectrumDisplay (PluginProcessor& p) : processorRef (p) { magnitudes.fill (-100.0f); }
void SpectrumDisplay::tick()
{
    std::array<float, 8192> incoming;
    const auto count = processorRef.readScope (incoming.data(), static_cast<int> (incoming.size()));
    for (int i = 0; i < count; ++i)
    {
        samples[static_cast<size_t> (cursor++)] = incoming[static_cast<size_t> (i)];
        if (cursor == 4096)
        {
            std::copy (samples.begin(), samples.end(), transform.begin());
            std::fill (transform.begin() + 4096, transform.end(), 0.0f);
            window.multiplyWithWindowingTable (transform.data(), 4096);
            fft.performFrequencyOnlyForwardTransform (transform.data());
            for (size_t bin = 0; bin < magnitudes.size(); ++bin)
            {
                const auto db = juce::Decibels::gainToDecibels (transform[bin] / 1024.0f, -100.0f);
                magnitudes[bin] = magnitudes[bin] * 0.65f + db * 0.35f;
            }
            cursor = 0;
        }
    }
    repaint();
}
void SpectrumDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (theme::ink);
    g.fillRoundedRectangle (bounds, 12);
    theme::text (g, "SPECTRAL OBSERVATORY", { 24, 16, 300, 23 }, 12, juce::Colour (0xffdeebe3));
    theme::text (g, "LIVE FFT  /  dBFS  /  LOG Hz", { bounds.getWidth() - 254, 18, 230, 20 }, 10, juce::Colour (0xff8ca8a2), juce::Justification::centredRight);
    const auto plot = bounds.withTrimmedTop (57).withTrimmedBottom (60).withTrimmedLeft (49).withTrimmedRight (25);
    const auto xAt = [&] (float hz) { return plot.getX() + std::log10 (hz / 20.0f) / 3.0f * plot.getWidth(); };
    const auto yAt = [&] (float db) { return plot.getBottom() - (juce::jlimit (-90.0f, 0.0f, db) + 90.0f) / 90.0f * plot.getHeight(); };
    for (const auto db : { -20, -40, -60, -80 })
    {
        g.setColour (juce::Colour (0xff30484d));
        g.drawHorizontalLine (static_cast<int> (yAt (static_cast<float> (db))), plot.getX(), plot.getRight());
        theme::text (g, juce::String (db), { 8, yAt (static_cast<float> (db)) - 7, 31, 14 }, 9, juce::Colour (0xff8ca8a2), juce::Justification::centredRight);
    }
    for (const auto hz : { 20, 100, 1000, 10000, 20000 })
    {
        g.setColour (juce::Colour (0xff30484d));
        g.drawVerticalLine (static_cast<int> (xAt (static_cast<float> (hz))), plot.getY(), plot.getBottom());
        theme::text (g, hz >= 1000 ? juce::String (hz / 1000) + "k" : juce::String (hz), { xAt (static_cast<float> (hz)) - 15, plot.getBottom() + 6, 30, 15 }, 9, juce::Colour (0xff8ca8a2), juce::Justification::centred);
    }
    const auto alpha = processorRef.parameters.getRawParameterValue ("color")->load();
    const auto contour = processorRef.parameters.getRawParameterValue ("sourceMode")->load() > 0.5f ? processorRef.parameters.getRawParameterValue ("focus")->load() : 0.0f;
    const auto profile = static_cast<int> (processorRef.parameters.getRawParameterValue ("profile")->load());
    const auto referencePower = noise::Spectrum::powerAt (1000, alpha, contour, profile);
    juce::Path reference;
    for (int pixel = 0; pixel <= static_cast<int> (plot.getWidth()); ++pixel)
    {
        const auto hz = 20.0f * std::pow (1000.0f, static_cast<float> (pixel) / plot.getWidth());
        const auto power = noise::Spectrum::powerAt (hz, alpha, contour, profile);
        const auto point = juce::Point<float> { plot.getX() + static_cast<float> (pixel), yAt (-40.0f + 10.0f * std::log10 (juce::jmax (1.0e-20f, power / referencePower))) };
        if (pixel == 0)
            reference.startNewSubPath (point);
        else
            reference.lineTo (point);
    }
    g.setColour (theme::colorAt (alpha).brighter (0.3f));
    const float dash[] { 4.0f, 4.0f };
    juce::Path dashed;
    juce::PathStrokeType (1.5f).createDashedStroke (dashed, reference, dash, 2);
    g.fillPath (dashed);
    juce::Path trace;
    const auto sampleRate = juce::jmax (1.0, processorRef.getSampleRate());
    for (int pixel = 0; pixel <= static_cast<int> (plot.getWidth()); ++pixel)
    {
        const auto hz = 20.0f * std::pow (1000.0f, static_cast<float> (pixel) / plot.getWidth());
        const auto bin = juce::jlimit (1.0f, 2046.0f, hz * 4096.0f / static_cast<float> (sampleRate));
        const auto lower = static_cast<size_t> (bin);
        const auto db = juce::jmap (bin - static_cast<float> (lower), magnitudes[lower], magnitudes[lower + 1]);
        const auto point = juce::Point<float> { plot.getX() + static_cast<float> (pixel), yAt (db) };
        if (pixel == 0)
            trace.startNewSubPath (point);
        else
            trace.lineTo (point);
    }
    auto fill = trace;
    fill.lineTo (plot.getBottomRight());
    fill.lineTo (plot.getBottomLeft());
    fill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff78cfb6).withAlpha (0.28f), plot.getTopLeft(), juce::Colour (0xff78cfb6).withAlpha (0.01f), plot.getBottomLeft(), false));
    g.fillPath (fill);
    g.setColour (juce::Colour (0xff91debf));
    g.strokePath (trace, juce::PathStrokeType (1.6f));
    theme::text (g, "DASHED: SOURCE SHAPE (RELATIVE)  /  SOLID: OUTPUT FFT", { 25, bounds.getHeight() - 27, bounds.getWidth() - 50, 16 }, 10, juce::Colour (0xffb3cbc1));
}

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), spectrum (p), spectrumPad (p), color (p, "color", "COLOR / EXPONENT", theme::pink), width (p, "width", "STEREO WIDTH", theme::teal, true), drive (p, "drive", "SATURATION", theme::teal, true), output (p, "volume", "OUTPUT", theme::teal), keys (p.keyboard, juce::MidiKeyboardComponent::horizontalKeyboard)
{
    setLookAndFeel (&lookAndFeel);
    setOpaque (true);
    const std::array<juce::String, 5> stopNames { "VIOLET", "BLUE", "WHITE", "PINK", "BROWN" };
    for (size_t i = 0; i < colorStops.size(); ++i)
    {
        auto& button = colorStops[i];
        button.setButtonText (stopNames[i]);
        button.setColour (juce::TextButton::textColourOffId, theme::colorAt (static_cast<float> (i) - 2.0f));
        button.setTooltip ("Set the spectral exponent to " + juce::String (static_cast<int> (i) - 2) + ". RMS remains normalized.");
        button.onClick = [this, i] {
            auto* parameter = processorRef.parameters.getParameter ("color");
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (static_cast<float> (i) - 2.0f));
            parameter->endChangeGesture();
        };
        addAndMakeVisible (button);
    }
    sourceMode.addItemList ({ "Color dial", "Spectrum X/Y" }, 1);
    profile.addItemList ({ "Power law / full spectrum", "Green / midrange band", "Gray / perceptual contour" }, 1);
    sourceModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.parameters, "sourceMode", sourceMode);
    profileAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.parameters, "profile", profile);
    color.slider.setTooltip ("Continuous exponent from -2 (violet) to +2 (brown/red). White = 0, pink = 1. Automatic source RMS normalization.");
    profile.setTooltip ("Green: broad midrange band centered near 600 Hz. Gray: bounded inverse-A weighting approximation. Color tilt applies to every profile.");
    sourceMode.onChange = [this] { color.setVisible (sourceMode.getSelectedId() == 1); spectrumPad.setVisible (sourceMode.getSelectedId() == 2); repaint(); };
    const std::array<juce::String, 4> stages { "Attack", "Decay", "Sustain", "Release" };
    for (size_t i = 0; i < ampControls.size(); ++i)
    {
        ampControls[i] = std::make_unique<ParameterControl> (p, "amp" + stages[i], stages[i].toUpperCase(), theme::teal);
        addAndMakeVisible (*ampControls[i]);
        filters[i] = std::make_unique<FilterCard> (p, static_cast<int> (i));
        addAndMakeVisible (*filters[i]);
    }
    for (auto* child : std::initializer_list<juce::Component*> { &spectrum, &spectrumPad, &color, &sourceMode, &profile, &width, &drive, &output, &mode, &presets, &play, &panic, &keys })
        addAndMakeVisible (child);
    mode.addItemList ({ "Continuous", "MIDI instrument" }, 1);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (p.parameters, "mode", mode);
    presets.addItemList ({ "01  Pink atmosphere", "02  White laboratory", "03  Brown earth", "04  Glass keys", "05  Velvet organ", "06  Distant ocean", "07  Blue air", "08  Violet mist", "09  Green stream", "10  Gray contour", "11  Rumble + air X/Y" }, 1);
    presets.setTextWhenNothingSelected ("Explore a starting point...");
    presets.onChange = [this] { processorRef.applyPreset (presets.getSelectedId() - 1); };
    play.setColour (juce::TextButton::buttonColourId, theme::teal);
    play.setColour (juce::TextButton::textColourOffId, theme::panel);
    play.onClick = [this] { processorRef.setPlaying (!processorRef.playing.load()); };
    play.setTooltip ("Start or release continuous noise. In MIDI instrument mode, play notes on the keyboard instead.");
    panic.onClick = [this] { processorRef.keyboard.allNotesOff (0); processorRef.panic(); };
    panic.setTooltip ("Immediately silence every voice and stop continuous playback.");
    keys.setAvailableRange (36, 96);
    keys.setLowestVisibleKey (48);
    keys.setOctaveForMiddleC (4);
    keys.setScrollButtonsVisible (true);
    keys.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, theme::panel);
    keys.setColour (juce::MidiKeyboardComponent::blackNoteColourId, theme::ink);
    keys.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, theme::teal.withAlpha (0.65f));
    keys.setColour (juce::MidiKeyboardComponent::mouseOverKeyOverlayColourId, theme::teal.withAlpha (0.18f));
    keys.setColour (juce::MidiKeyboardComponent::textLabelColourId, theme::muted);
    keys.setColour (juce::MidiKeyboardComponent::keySeparatorLineColourId, theme::line);
    keys.setWantsKeyboardFocus (true);
    keys.setTitle ("MIDI keyboard: click keys or focus here and play A W S E D F T G Y H U J K");
    spectrumPad.setVisible (sourceMode.getSelectedId() == 2);
    color.setVisible (sourceMode.getSelectedId() == 1);
    setResizable (true, true);
    setResizeLimits (1280, 900, 1680, 1100);
    setSize (1280, 900);
    startTimerHz (30);
}
PluginEditor::~PluginEditor()
{
    stopTimer();
    processorRef.keyboard.allNotesOff (0);
    setLookAndFeel (nullptr);
}
void PluginEditor::timerCallback()
{
    spectrum.tick();
    spectrumPad.repaint();
    color.slider.setColour (juce::Slider::thumbColourId, theme::colorAt (processorRef.parameters.getRawParameterValue ("color")->load()));
    for (auto& filter : filters)
        filter->refresh();
    const auto instrument = mode.getSelectedId() == 2;
    play.setEnabled (!instrument);
    play.setButtonText (instrument ? "PLAY MIDI KEYS" : (processorRef.playing.load() ? "STOP NOISE" : "PLAY NOISE"));
    meter = juce::jmax (processorRef.outputPeak.load(), meter * 0.88f);
    repaint();
}
void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (theme::background);
    theme::text (g, "ofsound", { 24, 13, 130, 18 }, 13, theme::teal);
    theme::text (g, "Noise Explorer", { 24, 33, 310, 36 }, 29, theme::ink);
    theme::text (g, "A STUDY IN CONTROLLED CHAOS", { 262, 46, 290, 18 }, 10, theme::muted);
    theme::card (g, sourceBounds.toFloat());
    theme::text (g, "01  /  NOISE SOURCE", sourceBounds.toFloat().reduced (20).withHeight (20), 12, theme::ink);
    const auto sx = static_cast<float> (sourceBounds.getX());
    const auto alpha = processorRef.parameters.getRawParameterValue ("color")->load();
    if (sourceMode.getSelectedId() == 1)
    {
        const auto info = juce::Rectangle<float> (sx + 158, 171, static_cast<float> (sourceBounds.getWidth() - 176), 24);
        theme::text (g, theme::colorName (alpha), info, 19, theme::colorAt (alpha));
        theme::text (g, "S(f) ~ 1 / f^a", info.translated (0, 29), 12, theme::muted);
        theme::text (g, juce::String (-3.0103f * alpha, 2) + " dB / octave tilt", info.translated (0, 50), 12, theme::ink);
        theme::text (g, "AUTO RMS / LEVEL HELD", info.translated (0, 71), 10, theme::teal);
    }
    const auto filterHeadingY = sourceBounds.getBottom() + 16;
    theme::text (g, "02  /  FILTER BANK", { 24, static_cast<float> (filterHeadingY), 210, 24 }, 12, theme::ink);
    theme::text (g, "4 PARALLEL BRANCHES   /   MIDI-TUNED RESONATORS   /   A4 = 440 Hz", { 258, static_cast<float> (filterHeadingY), static_cast<float> (getWidth() - 282), 24 }, 10, theme::muted, juce::Justification::centredRight);
    theme::card (g, ampBounds.toFloat());
    theme::text (g, "03  /  AMPLITUDE", { static_cast<float> (ampBounds.getX() + 20), static_cast<float> (ampBounds.getY() + 14), 200, 20 }, 12, theme::ink);
    theme::text (g, "Shape the source", { static_cast<float> (ampBounds.getX() + 20), static_cast<float> (ampBounds.getY() + 38), 190, 20 }, 12, theme::muted);
    theme::text (g, "Resonators use their own ADSR", { static_cast<float> (ampBounds.getX() + 20), static_cast<float> (ampBounds.getY() + 59), 200, 20 }, 10, theme::muted);
    const auto envArea = juce::Rectangle<float> (static_cast<float> (ampBounds.getX() + 22), static_cast<float> (ampBounds.getY() + 83), 176, 23);
    const auto a = static_cast<float> (ampControls[0]->slider.getValue());
    const auto d = static_cast<float> (ampControls[1]->slider.getValue());
    const auto s = static_cast<float> (ampControls[2]->slider.getValue());
    const auto r = static_cast<float> (ampControls[3]->slider.getValue());
    const auto total = a + d + r + 0.5f;
    juce::Path envelope;
    envelope.startNewSubPath (envArea.getBottomLeft());
    envelope.lineTo (envArea.getX() + a / total * envArea.getWidth(), envArea.getY());
    envelope.lineTo (envArea.getX() + (a + d) / total * envArea.getWidth(), envArea.getBottom() - s * envArea.getHeight());
    envelope.lineTo (envArea.getRight() - r / total * envArea.getWidth(), envArea.getBottom() - s * envArea.getHeight());
    envelope.lineTo (envArea.getBottomRight());
    g.setColour (theme::teal);
    g.strokePath (envelope, juce::PathStrokeType (1.5f));
    theme::card (g, outputBounds.toFloat());
    const auto ox = static_cast<float> (outputBounds.getX() + 150);
    const auto oy = static_cast<float> (outputBounds.getY());
    theme::text (g, "MASTER LEVEL", { ox, oy + 17, 155, 20 }, 11, theme::ink);
    const auto db = juce::Decibels::gainToDecibels (meter, -90.0f);
    theme::text (g, db < -80 ? "SILENT" : juce::String (db, 1) + " dBFS", { ox, oy + 44, 150, 25 }, 20, theme::teal);
    g.setColour (theme::line);
    g.fillRoundedRectangle (ox, oy + 80, static_cast<float> (outputBounds.getWidth() - 175), 6, 3);
    g.setColour (theme::teal);
    g.fillRoundedRectangle (ox, oy + 80, juce::jmax (0.0f, (db + 60.0f) / 60.0f) * static_cast<float> (outputBounds.getWidth() - 175), 6, 3);
    theme::text (g, "8 voices  /  soft-limited output", { ox, oy + 91, 205, 18 }, 10, theme::muted);
    const auto footerY = static_cast<float> (getHeight() - 24);
    theme::text (g, "NOISE EXPLORER  /  0.1.0", { 24, footerY, 250, 17 }, 9, theme::muted);
    const auto state = processorRef.soundingVoices.load() > 0 ? juce::String (processorRef.soundingVoices.load()) + " VOICE(S) ACTIVE" : "READY TO EXPLORE";
    theme::text (g, state + "   /   " + (mode.getSelectedId() == 2 ? "MIDI IN" : "CONTINUOUS"), { static_cast<float> (getWidth() - 400), footerY, 376, 17 }, 9, theme::teal, juce::Justification::centredRight);
}
void PluginEditor::resized()
{
    const auto w = getWidth();
    const auto h = getHeight();
    presets.setBounds (w - 700, 31, 220, 32);
    mode.setBounds (w - 466, 31, 166, 32);
    play.setBounds (w - 286, 27, 174, 40);
    panic.setBounds (w - 98, 31, 74, 32);
    const auto sourceWidth = (w - 60) * 32 / 100;
    sourceBounds = { 24, 82, sourceWidth, 260 };
    spectrum.setBounds (sourceBounds.getRight() + 12, 82, w - sourceWidth - 60, 260);
    sourceMode.setBounds (sourceBounds.getRight() - 172, 98, 152, 26);
    profile.setBounds (sourceBounds.getX() + 20, 133, sourceWidth - 40, 26);
    color.setBounds (sourceBounds.getX() + 16, 163, 136, 104);
    spectrumPad.setBounds (sourceBounds.getX() + 20, 169, sourceWidth - 40, 94);
    for (size_t i = 0; i < colorStops.size(); ++i)
        colorStops[i].setBounds (sourceBounds.getX() + 20 + static_cast<int> (i) * (sourceWidth - 40) / 5, 268, (sourceWidth - 40) / 5 - 3, 19);
    width.setBounds (sourceBounds.getX() + 25, 288, (sourceWidth - 70) / 2, 49);
    drive.setBounds (sourceBounds.getCentreX() + 10, 288, (sourceWidth - 70) / 2, 49);
    const auto cardWidth = (w - 84) / 4;
    for (size_t i = 0; i < filters.size(); ++i)
        filters[i]->setBounds (24 + static_cast<int> (i) * (cardWidth + 12), 384, cardWidth, 300);
    ampBounds = { 24, 700, (w - 60) * 68 / 100, h - 788 };
    outputBounds = { ampBounds.getRight() + 12, 700, w - ampBounds.getRight() - 36, h - 788 };
    const auto controlWidth = (ampBounds.getWidth() - 225) / 4;
    for (size_t i = 0; i < ampControls.size(); ++i)
        ampControls[i]->setBounds (ampBounds.getX() + 216 + static_cast<int> (i) * controlWidth, ampBounds.getY() + 12, controlWidth, ampBounds.getHeight() - 21);
    output.setBounds (outputBounds.getX() + 12, outputBounds.getY() + 12, 120, outputBounds.getHeight() - 21);
    keys.setKeyWidth (static_cast<float> (w - 48) / 36.0f);
    keys.setBounds (24, h - 82, w - 48, 50);
}
