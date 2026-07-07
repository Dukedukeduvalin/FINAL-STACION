#include "PluginEditor.h"

EstacionFinalAudioProcessorEditor::EstacionFinalAudioProcessorEditor (EstacionFinalAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p), mainPanel (p)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::Font (juce::FontOptions (28.0f, juce::Font::bold)));
    addAndMakeVisible (titleLabel);

    sloganLabel.setJustificationType (juce::Justification::centred);
    sloganLabel.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::italic)));
    sloganLabel.setColour (juce::Label::textColourId, TrackArchitectLookAndFeel::acero);
    addAndMakeVisible (sloganLabel);

    for (auto* label : { &destinoLabel, &lufsLabel, &truePeakLabel, &headroomLabel, &lraLabel })
    {
        label->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*label);
    }

    juce::StringArray destinos { "Spotify", "Apple Music", "YouTube", "TikTok",
                                  "Club", "Radio", "CD", "Vinilo", "Archivo Maestro" };
    destinoSelector.addItemList (destinos, 1);
    destinoSelector.setSelectedItemIndex (0, juce::dontSendNotification);
    destinoSelector.onChange = [this]
    {
        processor.apvts.getParameterAsValue ("destino") = destinoSelector.getSelectedItemIndex();
    };
    addAndMakeVisible (destinoSelector);

    analizarButton.onClick = [this]
    {
        // El analisis ya corre continuamente en processBlock; este boton
        // fuerza una actualizacion inmediata de la GUI en lugar de esperar
        // al proximo tick del Timer.
        updateFromSnapshot (processor.getCurrentSnapshot());
    };
    addAndMakeVisible (analizarButton);

    hacerloMejorButton.onClick = [this]
    {
        processor.requestAutoMaster();
        cadenaEstadoLabel.setText ("Cadena: activa (corrigiendo)", juce::dontSendNotification);
    };
    addAndMakeVisible (hacerloMejorButton);

    compararButton.onClick = [this]
    {
        const bool nowBypassed = ! processor.isCompareBypassed();
        processor.setCompareBypassed (nowBypassed);
        cadenaEstadoLabel.setText (nowBypassed ? "Cadena: comparando (original)"
                                                 : "Cadena: comparando (corregida)",
                                    juce::dontSendNotification);
    };
    addAndMakeVisible (compararButton);

    restaurarButton.onClick = [this]
    {
        processor.restoreOriginal();
        cadenaEstadoLabel.setText ("Cadena: en espera (original)", juce::dontSendNotification);
    };
    addAndMakeVisible (restaurarButton);

    cadenaEstadoLabel.setJustificationType (juce::Justification::centred);
    cadenaEstadoLabel.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::italic)));
    cadenaEstadoLabel.setColour (juce::Label::textColourId, TrackArchitectLookAndFeel::azulInformacion);
    addAndMakeVisible (cadenaEstadoLabel);

    porQueButton.onClick = [this]
    {
        if (lastFindings.empty())
        {
            juce::AlertWindow::showMessageBoxAsync (
                juce::MessageBoxIconType::InfoIcon, "Estacion Final",
                "No hay hallazgos que justificar todavia. Deja correr el audio unos segundos.");
            return;
        }

        juce::String message;
        for (const auto& f : lastFindings)
            message << "- " << f.explanation << "\n\n";

        juce::AlertWindow::showMessageBoxAsync (
            juce::MessageBoxIconType::InfoIcon, "¿Por que?", message);
    };
    addAndMakeVisible (porQueButton);

    addAndMakeVisible (mainPanel);

    setSize (900, 640);
    startTimerHz (30);
}

EstacionFinalAudioProcessorEditor::~EstacionFinalAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

juce::Colour EstacionFinalAudioProcessorEditor::semaphoreColour (SemaphoreState state) const
{
    switch (state)
    {
        case SemaphoreState::verde:    return TrackArchitectLookAndFeel::verde;
        case SemaphoreState::amarillo: return TrackArchitectLookAndFeel::ambar;
        case SemaphoreState::rojo:     return TrackArchitectLookAndFeel::rojo;
    }
    return TrackArchitectLookAndFeel::acero;
}

void EstacionFinalAudioProcessorEditor::timerCallback()
{
    const auto snapshot = processor.getCurrentSnapshot();
    updateFromSnapshot (snapshot);
    mainPanel.refresh (snapshot, processor.getMasteringChain());
    processor.pollAndRecordLearning();
}

void EstacionFinalAudioProcessorEditor::updateFromSnapshot (const MixAnalysisSnapshot& snap)
{
    currentSemaphore = snap.semaphore;
    lastFindings = snap.findings;

    destinoLabel.setText ("Destino: " + snap.destinoActivo, juce::dontSendNotification);
    lufsLabel.setText ("LUFS Integrado: " + juce::String (snap.integratedLUFS, 1)
                            + " (objetivo " + juce::String (snap.destinoObjetivoLUFS, 1) + ")",
                        juce::dontSendNotification);
    truePeakLabel.setText ("True Peak: " + juce::String (snap.truePeakDb, 2) + " dBTP", juce::dontSendNotification);
    headroomLabel.setText ("Headroom: " + juce::String (snap.headroomDb, 2) + " dB", juce::dontSendNotification);
    lraLabel.setText ("LRA: " + juce::String (snap.loudnessRangeLU, 1) + " LU", juce::dontSendNotification);

    repaint();
}

void EstacionFinalAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (TrackArchitectLookAndFeel::negroCarbon);

    // Semaforo ferroviario: circulo indicador en la esquina superior derecha.
    const int diameter = 28;
    const auto bounds = getLocalBounds();
    juce::Rectangle<int> lampArea (bounds.getRight() - diameter - 20, 20, diameter, diameter);

    g.setColour (juce::Colours::black);
    g.fillEllipse (lampArea.toFloat().expanded (3.0f));

    g.setColour (semaphoreColour (currentSemaphore));
    g.fillEllipse (lampArea.toFloat());

    g.setColour (TrackArchitectLookAndFeel::acero);
    g.drawEllipse (lampArea.toFloat().expanded (3.0f), 1.5f);
}

void EstacionFinalAudioProcessorEditor::resized()
{
    auto fullArea = getLocalBounds();

    // --- Pantalla Principal (compacta, banda superior) ---------------------
    auto area = fullArea.removeFromTop (230).reduced (20);

    titleLabel.setBounds (area.removeFromTop (36));
    sloganLabel.setBounds (area.removeFromTop (22));

    area.removeFromTop (8);
    destinoSelector.setBounds (area.removeFromTop (26).removeFromLeft (200));

    area.removeFromTop (8);
    auto infoArea = area.removeFromTop (100);
    const int rowHeight = infoArea.getHeight() / 4;
    destinoLabel.setBounds  (infoArea.removeFromTop (rowHeight));
    lufsLabel.setBounds     (infoArea.removeFromTop (rowHeight));
    truePeakLabel.setBounds (infoArea.removeFromTop (rowHeight));
    headroomLabel.setBounds (infoArea.removeFromTop (rowHeight));

    lraLabel.setBounds (area.removeFromTop (20));

    area.removeFromTop (8);
    auto buttonRow = area.removeFromTop (36);
    const int buttonWidth = buttonRow.getWidth() / 3;
    analizarButton.setBounds     (buttonRow.removeFromLeft (buttonWidth).reduced (4));
    hacerloMejorButton.setBounds (buttonRow.removeFromLeft (buttonWidth).reduced (4));
    porQueButton.setBounds       (buttonRow.removeFromLeft (buttonWidth).reduced (4));

    auto buttonRow2 = area.removeFromTop (30);
    const int buttonWidth2 = buttonRow2.getWidth() / 2;
    compararButton.setBounds  (buttonRow2.removeFromLeft (buttonWidth2).reduced (4));
    restaurarButton.setBounds (buttonRow2.removeFromLeft (buttonWidth2).reduced (4));

    cadenaEstadoLabel.setBounds (area.removeFromTop (18));

    // --- Panel Central (Fase 3): navegación + sección activa ---------------
    mainPanel.setBounds (fullArea);
}
