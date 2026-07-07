#pragma once

#include <JuceHeader.h>
#include "NavigationPanel.h"
#include "Views/AnalysisView.h"
#include "Views/RouteView.h"
#include "Views/DestinoView.h"
#include "Views/InspectionView.h"
#include "Views/MasterView.h"
#include "Views/ReportView.h"
#include "Views/ConfigView.h"
#include "Views/AboutView.h"

class EstacionFinalAudioProcessor;

/**
    MainPanel
    ----------
    Implementa el "Panel Central" completo de la especificación: aloja
    el NavigationPanel a la izquierda y, a la derecha, la vista de la
    sección activa (una de las 8 SectionView). Se actualiza a 30 Hz desde
    el mismo Timer que ya usaba la Pantalla Principal (ver PluginEditor).
*/
class MainPanel final : public juce::Component
{
public:
    explicit MainPanel (EstacionFinalAudioProcessor& processorToUse);

    void refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain& chain);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    NavigationPanel navigation;

    AnalysisView   analysisView;
    RouteView      routeView;
    DestinoView    destinoView;
    InspectionView inspectionView;
    MasterView     masterView;
    ReportView     reportView;
    ConfigView     configView;
    AboutView      aboutView;

    std::array<SectionView*, 8> sections;
    int currentSection = 0;

    void showSection (int index);
};
