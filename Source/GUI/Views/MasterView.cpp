#include "MasterView.h"

void MasterView::refresh (const MixAnalysisSnapshot&, const MasteringChain& chain)
{
    chainActive = chain.isEnabled();
    compressorRatio = chain.getCompressorRatio();
    compressorGR = chain.getCompressorGainReductionDb();
    limiterGR = chain.getLimiterGainReductionDb();
    stereoCorrelation = chain.getStereoCorrelation();
    eqBands = chain.getEqBands();
    repaint();
}

void MasterView::drawMeter (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& label,
                              float value, float maxValue, juce::Colour colour) const
{
    g.setColour (TrackArchitectLookAndFeel::acero);
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::plain)));
    g.drawText (label, bounds.removeFromTop (20), juce::Justification::left);

    auto meterArea = bounds.reduced (0, 4);
    g.setColour (TrackArchitectLookAndFeel::grisGrafito);
    g.fillRect (meterArea);

    const float ratio = maxValue > 0.0001f ? juce::jlimit (0.0f, 1.0f, value / maxValue) : 0.0f;
    auto filled = meterArea.withWidth (static_cast<int> (meterArea.getWidth() * ratio));
    g.setColour (colour);
    g.fillRect (filled);

    g.setColour (TrackArchitectLookAndFeel::acero);
    g.drawRect (meterArea, 1.0f);
}

void MasterView::paint (juce::Graphics& g)
{
    g.fillAll (TrackArchitectLookAndFeel::negroCarbon);

    auto area = getLocalBounds().reduced (20);

    g.setColour (TrackArchitectLookAndFeel::blanco);
    g.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    g.drawText ("Estado de la cadena de mastering", area.removeFromTop (30), juce::Justification::left);

    g.setColour (chainActive ? TrackArchitectLookAndFeel::verde : TrackArchitectLookAndFeel::acero);
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.drawText (chainActive ? "CADENA ACTIVA" : "CADENA EN ESPERA (RESTAURADO)",
                area.removeFromTop (24), juce::Justification::left);

    area.removeFromTop (10);

    drawMeter (g, area.removeFromTop (48), "Compresor - Reduccion de ganancia (dB)", compressorGR, 12.0f, TrackArchitectLookAndFeel::ambar);
    area.removeFromTop (6);
    drawMeter (g, area.removeFromTop (48), "Limitador - Reduccion de ganancia (dB)", limiterGR, 12.0f, TrackArchitectLookAndFeel::rojo);
    area.removeFromTop (6);
    drawMeter (g, area.removeFromTop (48), "Ratio de compresion actual", compressorRatio, 3.0f, TrackArchitectLookAndFeel::azulInformacion);
    area.removeFromTop (6);
    drawMeter (g, area.removeFromTop (48), "Correlacion estereo (-1 a +1, normalizada)",
               stereoCorrelation + 1.0f, 2.0f, TrackArchitectLookAndFeel::verde);

    area.removeFromTop (16);
    g.setColour (TrackArchitectLookAndFeel::blanco);
    g.setFont (juce::Font (juce::FontOptions (14.0f, juce::Font::bold)));
    g.drawText ("Bandas de EQ activas", area.removeFromTop (24), juce::Justification::left);

    bool anyActive = false;
    for (const auto& b : eqBands)
    {
        if (! b.active) continue;
        anyActive = true;
        auto row = area.removeFromTop (22);
        g.setColour (TrackArchitectLookAndFeel::azulInformacion);
        g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::plain)));
        g.drawText (juce::String (b.frequencyHz, 0) + " Hz  |  " + juce::String (b.gainDb, 1)
                        + " dB  |  Q " + juce::String (b.q, 2),
                    row, juce::Justification::left);
    }
    if (! anyActive)
    {
        g.setColour (TrackArchitectLookAndFeel::acero);
        g.drawText ("Sin bandas activas actualmente.", area.removeFromTop (22), juce::Justification::left);
    }
}

void MasterView::resized()
{
}
