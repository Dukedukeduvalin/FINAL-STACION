#pragma once

#include "SectionView.h"

/**
    MasterView
    -----------
    Sección "Master": medidores en vivo de cada vagón de MasteringChain —
    reducción de ganancia del compresor y del limitador, ratio actual,
    correlación estéreo, y bandas de EQ activas con su frecuencia/ganancia.
    Es el panel de control técnico del motor de mastering.
*/
class MasterView final : public SectionView
{
public:
    MasterView() = default;

    void refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain& chain) override;
    juce::String getSectionTitle() const override { return "MASTER"; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    bool chainActive = false;
    float compressorRatio = 1.0f;
    float compressorGR = 0.0f;
    float limiterGR = 0.0f;
    float stereoCorrelation = 1.0f;
    std::array<DynamicEQ::BandCorrection, DynamicEQ::maxBands> eqBands;

    void drawMeter (juce::Graphics& g, juce::Rectangle<int> bounds, const juce::String& label,
                     float value, float maxValue, juce::Colour colour) const;
};
