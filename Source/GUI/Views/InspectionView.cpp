#include "InspectionView.h"

void InspectionView::refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain&)
{
    lastSnapshot = snapshot;
    repaint();
}

void InspectionView::paint (juce::Graphics& g)
{
    g.fillAll (TrackArchitectLookAndFeel::negroCarbon);

    auto area = getLocalBounds().reduced (20);
    g.setColour (TrackArchitectLookAndFeel::blanco);
    g.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    g.drawText ("Mediciones crudas", area.removeFromTop (30), juce::Justification::left);
    area.removeFromTop (10);

    struct Row { juce::String label; juce::String value; };
    const std::vector<Row> rows {
        { "LUFS Momentaneo",  juce::String (lastSnapshot.momentaryLUFS, 2) + " LUFS" },
        { "LUFS Short-Term",  juce::String (lastSnapshot.shortTermLUFS, 2) + " LUFS" },
        { "LUFS Integrado",   juce::String (lastSnapshot.integratedLUFS, 2) + " LUFS" },
        { "Loudness Range",   juce::String (lastSnapshot.loudnessRangeLU, 2) + " LU" },
        { "True Peak",        juce::String (lastSnapshot.truePeakDb, 2) + " dBTP" },
        { "Headroom",         juce::String (lastSnapshot.headroomDb, 2) + " dB" },
        { "Destino activo",   lastSnapshot.destinoActivo },
        { "Datos suficientes", lastSnapshot.hasEnoughDataToJudge ? "Si" : "No (analizando...)" },
    };

    const int rowHeight = 28;
    g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::plain)));
    for (const auto& r : rows)
    {
        auto row = area.removeFromTop (rowHeight);
        g.setColour (TrackArchitectLookAndFeel::acero);
        g.drawText (r.label, row.removeFromLeft (220), juce::Justification::left);
        g.setColour (TrackArchitectLookAndFeel::azulInformacion);
        g.drawText (r.value, row, juce::Justification::left);
    }
}

void InspectionView::resized()
{
}
