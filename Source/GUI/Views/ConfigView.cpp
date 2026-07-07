#include "ConfigView.h"
#include "../../PluginProcessor.h"

ConfigView::ConfigView (EstacionFinalAudioProcessor& processorToUse)
{
    headerLabel.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    headerLabel.setColour (juce::Label::textColourId, TrackArchitectLookAndFeel::blanco);
    addAndMakeVisible (headerLabel);

    intervencionLabel.setColour (juce::Label::textColourId, TrackArchitectLookAndFeel::acero);
    addAndMakeVisible (intervencionLabel);

    intervencionSlider.setRange (0.0, 100.0, 1.0);
    intervencionSlider.setColour (juce::Slider::trackColourId, TrackArchitectLookAndFeel::ambar);
    intervencionSlider.setColour (juce::Slider::backgroundColourId, TrackArchitectLookAndFeel::grisGrafito);
    addAndMakeVisible (intervencionSlider);

    bypassToggle.setColour (juce::ToggleButton::textColourId, TrackArchitectLookAndFeel::blanco);
    addAndMakeVisible (bypassToggle);

    intervencionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        processorToUse.apvts, "intervencion", intervencionSlider);

    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        processorToUse.apvts, "bypassCadena", bypassToggle);
}

void ConfigView::refresh (const MixAnalysisSnapshot&, const MasteringChain&)
{
    // Los valores se mantienen sincronizados automaticamente por las
    // attachments de JUCE; no se requiere logica adicional aqui.
}

void ConfigView::paint (juce::Graphics& g)
{
    g.fillAll (TrackArchitectLookAndFeel::negroCarbon);
}

void ConfigView::resized()
{
    auto area = getLocalBounds().reduced (20);
    headerLabel.setBounds (area.removeFromTop (30));
    area.removeFromTop (16);

    intervencionLabel.setBounds (area.removeFromTop (22));
    intervencionSlider.setBounds (area.removeFromTop (32));

    area.removeFromTop (16);
    bypassToggle.setBounds (area.removeFromTop (28));
}
