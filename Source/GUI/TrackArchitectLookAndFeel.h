#pragma once

#include <JuceHeader.h>

/**
    TrackArchitectLookAndFeel
    ---------------------------
    Implementa la paleta e identidad visual definida en la especificación:
    negro carbon, gris grafito, acero, ambar, verde, rojo, azul tenue para
    informacion. Estetica de panel de control ferroviario / laboratorio
    profesional (no gamer, no caricaturesco).
*/
class TrackArchitectLookAndFeel : public juce::LookAndFeel_V4
{
public:
    TrackArchitectLookAndFeel();

    // Paleta central (ver especificacion, seccion PALETA)
    static const juce::Colour negroCarbon;
    static const juce::Colour grisGrafito;
    static const juce::Colour acero;
    static const juce::Colour ambar;
    static const juce::Colour verde;
    static const juce::Colour rojo;
    static const juce::Colour blanco;
    static const juce::Colour azulInformacion;

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                                bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;
};
