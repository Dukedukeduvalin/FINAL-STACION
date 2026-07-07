#pragma once

#include "SectionView.h"

/**
    RouteView
    ----------
    Sección "Ruta": representa visualmente el concepto central de
    identidad de la especificación — la mezcla es un tren, cada vagón es
    un proceso (EQ, Compresión, Saturación, Imagen Estéreo, Limitador,
    Control Final). Muestra el estado de cada vagón (activo/inactivo,
    parámetro principal actual) en orden de señal.
*/
class RouteView final : public SectionView
{
public:
    RouteView() = default;

    void refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain& chain) override;
    juce::String getSectionTitle() const override { return "RUTA"; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct WagonInfo { juce::String name; juce::String status; bool active = false; };
    std::array<WagonInfo, 6> wagons;
};
