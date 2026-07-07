#pragma once

#include "SectionView.h"

/**
    DestinoView
    ------------
    Sección "Destino": tabla de referencia de los 9 destinos definidos en
    la especificación con sus objetivos de loudness y límite de True Peak,
    y resalta el destino actualmente activo junto con la diferencia entre
    la mezcla medida y el objetivo.
*/
class DestinoView final : public SectionView
{
public:
    DestinoView();

    void refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain& chain) override;
    juce::String getSectionTitle() const override { return "DESTINO"; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct DestinoRow { juce::String nombre; float objetivoLUFS; float limiteTruePeakDb; };
    std::vector<DestinoRow> destinos;

    juce::String activo;
    float currentIntegratedLUFS = -70.0f;
};
