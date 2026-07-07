#include "DestinoView.h"

DestinoView::DestinoView()
{
    // Mismos valores de referencia que MixAnalysisEngine::allDestinos(),
    // duplicados aquí deliberadamente en forma de tabla de solo lectura
    // para la GUI (evita acoplar la vista al motor de análisis interno;
    // ver ROADMAP.md para unificar esta fuente en Fase 5).
    destinos = {
        { "Spotify",        -14.0f, -1.0f },
        { "Apple Music",    -16.0f, -1.0f },
        { "YouTube",        -14.0f, -1.0f },
        { "TikTok",         -14.0f, -1.0f },
        { "Club",            -8.0f, -0.3f },
        { "Radio",          -16.0f, -1.0f },
        { "CD",              -9.0f, -0.3f },
        { "Vinilo",          -12.0f, -3.0f },
        { "Archivo Maestro", -10.0f, -1.0f },
    };
}

void DestinoView::refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain&)
{
    activo = snapshot.destinoActivo;
    currentIntegratedLUFS = snapshot.integratedLUFS;
    repaint();
}

void DestinoView::paint (juce::Graphics& g)
{
    g.fillAll (TrackArchitectLookAndFeel::negroCarbon);

    auto area = getLocalBounds().reduced (20);

    g.setColour (TrackArchitectLookAndFeel::blanco);
    g.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    g.drawText ("Objetivos de loudness por destino", area.removeFromTop (30), juce::Justification::left);

    area.removeFromTop (10);

    const int rowHeight = 30;
    // Cabecera
    {
        auto row = area.removeFromTop (rowHeight);
        g.setColour (TrackArchitectLookAndFeel::acero);
        g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
        g.drawText ("Destino",         row.removeFromLeft (220), juce::Justification::left);
        g.drawText ("Objetivo LUFS",   row.removeFromLeft (140), juce::Justification::left);
        g.drawText ("Limite True Peak",row.removeFromLeft (160), juce::Justification::left);
        g.drawText ("Diferencia",      row, juce::Justification::left);
    }

    for (const auto& d : destinos)
    {
        auto row = area.removeFromTop (rowHeight);
        const bool isActive = d.nombre == activo;

        if (isActive)
        {
            g.setColour (TrackArchitectLookAndFeel::grisGrafito);
            g.fillRect (row);
        }

        g.setColour (isActive ? TrackArchitectLookAndFeel::ambar : TrackArchitectLookAndFeel::blanco);
        g.setFont (juce::Font (juce::FontOptions (13.0f, isActive ? juce::Font::bold : juce::Font::plain)));
        g.drawText (d.nombre, row.removeFromLeft (220), juce::Justification::left);
        g.drawText (juce::String (d.objetivoLUFS, 1) + " LUFS", row.removeFromLeft (140), juce::Justification::left);
        g.drawText (juce::String (d.limiteTruePeakDb, 1) + " dBTP", row.removeFromLeft (160), juce::Justification::left);

        if (isActive)
        {
            const float diff = currentIntegratedLUFS - d.objetivoLUFS;
            g.setColour (std::abs (diff) >= 3.0f ? TrackArchitectLookAndFeel::rojo
                                                    : (std::abs (diff) >= 1.0f ? TrackArchitectLookAndFeel::ambar
                                                                                  : TrackArchitectLookAndFeel::verde));
            g.drawText ((diff >= 0 ? "+" : "") + juce::String (diff, 1) + " LU", row, juce::Justification::left);
        }
    }
}

void DestinoView::resized()
{
}
