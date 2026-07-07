#include "NavigationPanel.h"

NavigationPanel::NavigationPanel()
{
    static const juce::StringArray names {
        "ANALISIS", "RUTA", "DESTINO", "INSPECCION",
        "MASTER", "REPORTE", "CONFIGURACION", "ACERCA DE"
    };

    for (int i = 0; i < numSections; ++i)
    {
        auto* button = buttons.add (new juce::TextButton (names[i]));
        button->setClickingTogglesState (false);
        button->onClick = [this, i] { selectSection (i); };
        addAndMakeVisible (button);
    }

    selectSection (0);
}

void NavigationPanel::selectSection (int index)
{
    selectedIndex = index;

    for (int i = 0; i < buttons.size(); ++i)
    {
        auto* b = buttons[i];
        b->setColour (juce::TextButton::buttonColourId,
                       i == selectedIndex ? TrackArchitectLookAndFeel::ambar.withAlpha (0.25f)
                                            : TrackArchitectLookAndFeel::grisGrafito);
        b->setColour (juce::TextButton::textColourOffId,
                       i == selectedIndex ? TrackArchitectLookAndFeel::ambar : TrackArchitectLookAndFeel::blanco);
    }

    if (onSectionSelected != nullptr)
        onSectionSelected (selectedIndex);

    repaint();
}

void NavigationPanel::paint (juce::Graphics& g)
{
    g.fillAll (TrackArchitectLookAndFeel::grisGrafito.darker (0.3f));

    g.setColour (TrackArchitectLookAndFeel::acero.withAlpha (0.4f));
    g.drawLine (static_cast<float> (getWidth()), 0.0f, static_cast<float> (getWidth()),
                static_cast<float> (getHeight()), 1.0f);
}

void NavigationPanel::resized()
{
    auto area = getLocalBounds().reduced (6);
    const int buttonHeight = 40;

    for (auto* b : buttons)
    {
        b->setBounds (area.removeFromTop (buttonHeight));
        area.removeFromTop (4);
    }
}
