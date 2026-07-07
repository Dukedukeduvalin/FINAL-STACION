#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "GUI/TrackArchitectLookAndFeel.h"
#include "GUI/MainPanel.h"

/**
    EstacionFinalAudioProcessorEditor
    ------------------------------------
    Implementa la "PANTALLA PRINCIPAL" de la especificacion:

        Logo + slogan
        Estado del viaje (semaforo)
        Destino / LUFS / True Peak / Balance / Headroom
        Boton principal "HACERLO MEJOR"

    Este archivo cubre unicamente la pantalla principal. El panel central
    con las secciones (Analisis, Ruta, Destino, Inspeccion, Master, Reporte,
    Configuracion, Acerca de) se implementa como modulo separado en la
    siguiente fase (ver ROADMAP.md, Fase 3: GUI completa).
*/
class EstacionFinalAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                                  private juce::Timer
{
public:
    explicit EstacionFinalAudioProcessorEditor (EstacionFinalAudioProcessor&);
    ~EstacionFinalAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void updateFromSnapshot (const MixAnalysisSnapshot& snap);

    juce::Colour semaphoreColour (SemaphoreState state) const;

    EstacionFinalAudioProcessor& processor;
    TrackArchitectLookAndFeel lookAndFeel;

    juce::Label titleLabel        { {}, "ESTACION FINAL" };
    juce::Label sloganLabel       { {}, "The Last Stop Before Release" };

    juce::Label destinoLabel      { {}, "Destino: --" };
    juce::Label lufsLabel         { {}, "LUFS Integrado: --" };
    juce::Label truePeakLabel     { {}, "True Peak: --" };
    juce::Label headroomLabel     { {}, "Headroom: --" };
    juce::Label lraLabel          { {}, "LRA: --" };

    juce::TextButton analizarButton     { "ANALIZAR" };
    juce::TextButton hacerloMejorButton { "HACERLO MEJOR" };
    juce::TextButton porQueButton       { "¿POR QUE?" };
    juce::TextButton compararButton     { "COMPARAR" };
    juce::TextButton restaurarButton    { "RESTAURAR" };

    juce::Label cadenaEstadoLabel { {}, "Cadena: en espera" };

    juce::ComboBox destinoSelector;

    MainPanel mainPanel;

    // Panel de semaforo dibujado directamente (no es un componente aparte
    // para mantener el paint centralizado y fiel a la estetica de indicador
    // analogico industrial).
    SemaphoreState currentSemaphore = SemaphoreState::amarillo;

    // Ultimo diagnostico recibido, usado por el boton "¿POR QUE?" para
    // mostrar las explicaciones reales generadas por MixAnalysisEngine.
    std::vector<MixFinding> lastFindings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EstacionFinalAudioProcessorEditor)
};
