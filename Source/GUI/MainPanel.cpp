#include "MainPanel.h"
#include "../PluginProcessor.h"

MainPanel::MainPanel (EstacionFinalAudioProcessor& processorToUse)
    : reportView (processorToUse), configView (processorToUse)
{
    sections = { &analysisView, &routeView, &destinoView, &inspectionView,
                 &masterView, &reportView, &configView, &aboutView };

    addAndMakeVisible (navigation);
    for (auto* s : sections)
        addChildComponent (s); // oculto por defecto; showSection lo activa

    navigation.onSectionSelected = [this] (int index) { showSection (index); };

    showSection (0);
}

void MainPanel::showSection (int index)
{
    currentSection = juce::jlimit (0, static_cast<int> (sections.size()) - 1, index);
    for (int i = 0; i < static_cast<int> (sections.size()); ++i)
        sections[static_cast<size_t> (i)]->setVisible (i == currentSection);
    resized();
}

void MainPanel::refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain& chain)
{
    // Se actualizan todas las vistas (no solo la visible) para que al
    // cambiar de sección los datos ya estén listos sin esperar el
    // siguiente tick del Timer.
    for (auto* s : sections)
        s->refresh (snapshot, chain);
}

void MainPanel::paint (juce::Graphics& g)
{
    g.fillAll (TrackArchitectLookAndFeel::negroCarbon);
}

void MainPanel::resized()
{
    auto area = getLocalBounds();
    navigation.setBounds (area.removeFromLeft (170));

    for (auto* s : sections)
        s->setBounds (area);
}
