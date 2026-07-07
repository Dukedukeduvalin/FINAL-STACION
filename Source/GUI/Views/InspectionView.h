#pragma once

#include "SectionView.h"

/**
    InspectionView
    ---------------
    Sección "Inspección": vista técnica detallada de las mediciones crudas
    (momentary/short-term/integrated LUFS, LRA, True Peak, headroom),
    destinada al usuario que quiere ver los números exactos detrás del
    semáforo de la Pantalla Principal — coherente con la filosofía de
    "no ser una caja negra".
*/
class InspectionView final : public SectionView
{
public:
    InspectionView() = default;

    void refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain& chain) override;
    juce::String getSectionTitle() const override { return "INSPECCION"; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    MixAnalysisSnapshot lastSnapshot;
};
