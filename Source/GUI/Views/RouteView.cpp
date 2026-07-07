#include "RouteView.h"

void RouteView::refresh (const MixAnalysisSnapshot&, const MasteringChain& chain)
{
    const bool active = chain.isEnabled();

    int activeBands = 0;
    for (const auto& b : chain.getEqBands())
        if (b.active) ++activeBands;

    wagons[0] = { "EQ",              active && activeBands > 0 ? juce::String (activeBands) + " banda(s) activas" : "sin correccion", active && activeBands > 0 };
    wagons[1] = { "COMPRESION",      active ? "ratio " + juce::String (chain.getCompressorRatio(), 2) + ":1" : "en espera", active };
    wagons[2] = { "SATURACION",      active ? "activa" : "en espera", active };
    wagons[3] = { "IMAGEN ESTEREO",  active ? "corr. " + juce::String (chain.getStereoCorrelation(), 2) : "en espera", active };
    wagons[4] = { "LIMITADOR",       active ? juce::String (chain.getLimiterGainReductionDb(), 2) + " dB GR" : "en espera", active };
    wagons[5] = { "CONTROL FINAL",   active ? "autorizado" : "sin autorizar", active };
}

void RouteView::paint (juce::Graphics& g)
{
    g.fillAll (TrackArchitectLookAndFeel::negroCarbon);

    auto area = getLocalBounds().reduced (24);
    area.removeFromTop (36); // espacio del titulo dibujado aparte si se desea

    const int n = static_cast<int> (wagons.size());
    const int wagonWidth = area.getWidth() / n;

    for (int i = 0; i < n; ++i)
    {
        auto cell = area.withX (area.getX() + i * wagonWidth).withWidth (wagonWidth).reduced (8);
        const auto& w = wagons[static_cast<size_t> (i)];

        g.setColour (TrackArchitectLookAndFeel::grisGrafito);
        g.fillRect (cell);
        g.setColour (w.active ? TrackArchitectLookAndFeel::verde : TrackArchitectLookAndFeel::acero);
        g.drawRect (cell, 2.0f);

        g.setColour (TrackArchitectLookAndFeel::blanco);
        g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
        g.drawFittedText (w.name, cell.removeFromTop (cell.getHeight() / 2).reduced (4),
                            juce::Justification::centred, 2);

        g.setColour (w.active ? TrackArchitectLookAndFeel::verde : TrackArchitectLookAndFeel::acero);
        g.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::plain)));
        g.drawFittedText (w.status, cell.reduced (4), juce::Justification::centred, 2);

        // Conector visual entre vagones (representa el "riel").
        if (i < n - 1)
        {
            g.setColour (TrackArchitectLookAndFeel::acero.withAlpha (0.5f));
            const int connectorY = cell.getCentreY();
            g.drawLine (static_cast<float> (cell.getRight()), static_cast<float> (connectorY),
                        static_cast<float> (cell.getRight() + 8), static_cast<float> (connectorY), 2.0f);
        }
    }
}

void RouteView::resized()
{
    // El dibujo se hace enteramente en paint(); no hay sub-componentes.
}
