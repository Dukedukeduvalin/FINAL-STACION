#pragma once

#include <JuceHeader.h>
#include "TrackArchitectLookAndFeel.h"

/**
    NavigationPanel
    ----------------
    Menú lateral con las 8 secciones del "Panel Central" de la
    especificación: Análisis, Ruta, Destino, Inspección, Master, Reporte,
    Configuración, Acerca de. Notifica a MainPanel el índice seleccionado.
*/
class NavigationPanel final : public juce::Component
{
public:
    NavigationPanel();

    std::function<void (int sectionIndex)> onSectionSelected;

    void paint (juce::Graphics&) override;
    void resized() override;

    static constexpr int numSections = 8;

private:
    juce::OwnedArray<juce::TextButton> buttons;
    int selectedIndex = 0;

    void selectSection (int index);
};
