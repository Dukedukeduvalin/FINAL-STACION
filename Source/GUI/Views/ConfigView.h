#pragma once

#include "SectionView.h"

class EstacionFinalAudioProcessor; // fwd decl (ver PluginProcessor.h)

/**
    ConfigView
    -----------
    Sección "Configuración": expone los parámetros automatizables del
    plugin (intervención permitida al motor "HACERLO MEJOR", bypass
    global de la cadena de corrección) mediante controles enlazados
    directamente al APVTS del procesador, siguiendo el patrón estándar
    de JUCE (SliderAttachment / ButtonAttachment) para que el host pueda
    automatizarlos sin lógica adicional.
*/
class ConfigView final : public SectionView
{
public:
    explicit ConfigView (EstacionFinalAudioProcessor& processorToUse);

    void refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain& chain) override;
    juce::String getSectionTitle() const override { return "CONFIGURACION"; }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    juce::Label headerLabel { {}, "Configuracion del motor de mastering" };

    juce::Label intervencionLabel { {}, "Intervencion permitida (%)" };
    juce::Slider intervencionSlider { juce::Slider::LinearHorizontal, juce::Slider::TextBoxRight };

    juce::ToggleButton bypassToggle { "Bypass global de la cadena de correccion" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> intervencionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
};
