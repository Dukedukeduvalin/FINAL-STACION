#pragma once

#include "SectionView.h"

/**
    AboutView
    ----------
    Sección "Acerca de": identidad del proyecto tal como está definida en
    la especificación (nombre comercial, slogan, creadores, filosofía).
    Contenido estático; no requiere datos de análisis.
*/
class AboutView final : public SectionView
{
public:
    AboutView();

    void refresh (const MixAnalysisSnapshot&, const MasteringChain&) override {}
    juce::String getSectionTitle() const override { return "ACERCA DE"; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Label titleLabel  { {}, "ESTACION FINAL" };
    juce::Label sloganLabel { {}, "La ultima parada antes del lanzamiento." };
    juce::TextEditor bodyText;
};
