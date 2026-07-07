#pragma once

#include "SectionView.h"

/**
    AnalysisView
    -------------
    Sección "Análisis" del Panel Central: lista completa de MixFinding
    actuales con severidad, categoría, valor medido y explicación —
    la versión expandida y permanente del botón "¿POR QUÉ?" de la
    Pantalla Principal (que solo muestra un popup puntual).
*/
class AnalysisView final : public SectionView
{
public:
    AnalysisView();

    void refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain& chain) override;
    juce::String getSectionTitle() const override { return "ANALISIS"; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Label headerLabel { {}, "Hallazgos detectados" };
    juce::TextEditor findingsList; // solo lectura: listado desplazable de hallazgos

    static juce::String severityLabel (FindingSeverity s);
    static juce::String categoryLabel (FindingCategory c);
};
