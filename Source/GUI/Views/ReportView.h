#pragma once

#include "SectionView.h"

class EstacionFinalAudioProcessor;

/**
    ReportView
    -----------
    Sección "Reporte": genera un resumen textual exportable con el
    diagnóstico completo, pensado para el botón "EXPORTAR REPORTE".
    Incluye ademas el resumen del perfil de aprendizaje del productor
    (ProducerProfile) acumulado entre sesiones (Fase 4).
*/
class ReportView final : public SectionView
{
public:
    explicit ReportView (EstacionFinalAudioProcessor& processorToUse);

    void refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain& chain) override;
    juce::String getSectionTitle() const override { return "REPORTE"; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    EstacionFinalAudioProcessor& processor;

    juce::Label headerLabel { {}, "Reporte de masterizacion" };
    juce::TextEditor reportText;
    juce::TextButton exportButton { "EXPORTAR REPORTE (.txt)" };

    juce::String buildReportText (const MixAnalysisSnapshot& snapshot, const MasteringChain& chain) const;
};
