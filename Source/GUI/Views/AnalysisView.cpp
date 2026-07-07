#include "AnalysisView.h"

AnalysisView::AnalysisView()
{
    headerLabel.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    headerLabel.setColour (juce::Label::textColourId, TrackArchitectLookAndFeel::blanco);
    addAndMakeVisible (headerLabel);

    findingsList.setMultiLine (true);
    findingsList.setReadOnly (true);
    findingsList.setScrollbarsShown (true);
    findingsList.setCaretVisible (false);
    findingsList.setColour (juce::TextEditor::backgroundColourId, TrackArchitectLookAndFeel::grisGrafito);
    findingsList.setColour (juce::TextEditor::textColourId, TrackArchitectLookAndFeel::blanco);
    findingsList.setColour (juce::TextEditor::outlineColourId, TrackArchitectLookAndFeel::acero);
    findingsList.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 13.0f, juce::Font::plain)));
    addAndMakeVisible (findingsList);
}

juce::String AnalysisView::severityLabel (FindingSeverity s)
{
    switch (s)
    {
        case FindingSeverity::info:       return "INFO";
        case FindingSeverity::suggestion: return "AMARILLO";
        case FindingSeverity::critical:   return "ROJO";
    }
    return "?";
}

juce::String AnalysisView::categoryLabel (FindingCategory c)
{
    switch (c)
    {
        case FindingCategory::loudnessIntegrated: return "Loudness Integrado";
        case FindingCategory::loudnessRange:       return "Rango de Loudness (LRA)";
        case FindingCategory::truePeak:            return "True Peak";
        case FindingCategory::spectralBalance:     return "Balance Espectral";
        case FindingCategory::stereoImage:         return "Imagen Estereo";
        case FindingCategory::dynamicRange:        return "Rango Dinamico";
        case FindingCategory::headroom:            return "Headroom";
    }
    return "?";
}

void AnalysisView::refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain&)
{
    if (! snapshot.hasEnoughDataToJudge)
    {
        findingsList.setText ("Analizando... se necesitan al menos 3 segundos de audio "
                               "para emitir un diagnostico confiable.", juce::dontSendNotification);
        return;
    }

    if (snapshot.findings.empty())
    {
        findingsList.setText ("Sin hallazgos relevantes. La mezcla esta dentro de los "
                               "parametros esperados para el destino \"" + snapshot.destinoActivo + "\".",
                               juce::dontSendNotification);
        return;
    }

    juce::String text;
    int index = 1;
    for (const auto& f : snapshot.findings)
    {
        text << "[" << index << "] " << severityLabel (f.severity) << " - " << categoryLabel (f.category) << "\n";
        if (! std::isnan (f.frequencyHz))
            text << "    Frecuencia: " << juce::String (f.frequencyHz, 0) << " Hz\n";
        text << "    Valor medido: " << juce::String (f.measuredValueDb, 2) << " dB/LU\n";
        text << "    " << f.explanation << "\n\n";
        ++index;
    }
    findingsList.setText (text, juce::dontSendNotification);
}

void AnalysisView::paint (juce::Graphics& g)
{
    g.fillAll (TrackArchitectLookAndFeel::negroCarbon);
}

void AnalysisView::resized()
{
    auto area = getLocalBounds().reduced (16);
    headerLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (8);
    findingsList.setBounds (area);
}
