#include "AboutView.h"

AboutView::AboutView()
{
    titleLabel.setFont (juce::Font (juce::FontOptions (26.0f, juce::Font::bold)));
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setColour (juce::Label::textColourId, TrackArchitectLookAndFeel::blanco);
    addAndMakeVisible (titleLabel);

    sloganLabel.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::italic)));
    sloganLabel.setJustificationType (juce::Justification::centred);
    sloganLabel.setColour (juce::Label::textColourId, TrackArchitectLookAndFeel::acero);
    addAndMakeVisible (sloganLabel);

    bodyText.setMultiLine (true);
    bodyText.setReadOnly (true);
    bodyText.setScrollbarsShown (true);
    bodyText.setCaretVisible (false);
    bodyText.setColour (juce::TextEditor::backgroundColourId, TrackArchitectLookAndFeel::grisGrafito);
    bodyText.setColour (juce::TextEditor::textColourId, TrackArchitectLookAndFeel::blanco);
    bodyText.setFont (juce::Font (juce::FontOptions (13.5f, juce::Font::plain)));
    bodyText.setText (
        "Diseñado y desarrollado por EduBeatsMusic, NewOrderPro y Parde Asez.\n\n"
        "ESTACION FINAL es un ingeniero de mastering virtual: analiza la mezcla, "
        "explica cada hallazgo y aplica correcciones justificadas tecnicamente, "
        "nunca presets ni recetas arbitrarias.\n\n"
        "Filosofia: el usuario debe aprender mientras masteriza. Cada decision del "
        "plugin debe poder explicarse con el boton \"¿POR QUE?\".\n\n"
        "Version: Fase 3 (nucleo de analisis + motor de mastering + GUI de "
        "navegacion completa). Ver ROADMAP.md para el estado detallado del "
        "proyecto.",
        juce::dontSendNotification);
    addAndMakeVisible (bodyText);
}

void AboutView::paint (juce::Graphics& g)
{
    g.fillAll (TrackArchitectLookAndFeel::negroCarbon);
}

void AboutView::resized()
{
    auto area = getLocalBounds().reduced (20);
    titleLabel.setBounds (area.removeFromTop (40));
    sloganLabel.setBounds (area.removeFromTop (24));
    area.removeFromTop (12);
    bodyText.setBounds (area);
}
