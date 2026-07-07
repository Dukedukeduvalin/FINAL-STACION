#include "TrackArchitectLookAndFeel.h"

const juce::Colour TrackArchitectLookAndFeel::negroCarbon     { 0xff141414 };
const juce::Colour TrackArchitectLookAndFeel::grisGrafito     { 0xff2a2a2c };
const juce::Colour TrackArchitectLookAndFeel::acero           { 0xff8a8f96 };
const juce::Colour TrackArchitectLookAndFeel::ambar           { 0xffffb020 };
const juce::Colour TrackArchitectLookAndFeel::verde           { 0xff3ddc72 };
const juce::Colour TrackArchitectLookAndFeel::rojo            { 0xffe4483b };
const juce::Colour TrackArchitectLookAndFeel::blanco          { 0xfff2f2f2 };
const juce::Colour TrackArchitectLookAndFeel::azulInformacion { 0xff5aa9e6 };

TrackArchitectLookAndFeel::TrackArchitectLookAndFeel()
{
    setColour (juce::ResizableWindow::backgroundColourId, negroCarbon);
    setColour (juce::TextButton::buttonColourId, grisGrafito);
    setColour (juce::TextButton::textColourOffId, blanco);
    setColour (juce::Label::textColourId, blanco);
}

void TrackArchitectLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                        const juce::Colour& backgroundColour,
                                                        bool shouldDrawButtonAsHighlighted,
                                                        bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);

    auto baseColour = backgroundColour;
    if (shouldDrawButtonAsDown)
        baseColour = baseColour.darker (0.3f);
    else if (shouldDrawButtonAsHighlighted)
        baseColour = baseColour.brighter (0.15f);

    // Panel tipo "boton industrial": bordes rectos, sutil bisel, sin
    // gradientes llamativos ni estetica gamer.
    g.setColour (baseColour);
    g.fillRect (bounds);

    g.setColour (acero.withAlpha (0.6f));
    g.drawRect (bounds, 1.0f);
}

juce::Font TrackArchitectLookAndFeel::getLabelFont (juce::Label&)
{
    return juce::Font (juce::FontOptions (15.0f, juce::Font::plain));
}

juce::Font TrackArchitectLookAndFeel::getTextButtonFont (juce::TextButton&, int buttonHeight)
{
    return juce::Font (juce::FontOptions (juce::jmin (16.0f, static_cast<float> (buttonHeight) * 0.5f),
                                            juce::Font::bold));
}
