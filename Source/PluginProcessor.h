#pragma once

#include <JuceHeader.h>
#include <set>
#include "DSP/LoudnessAnalyzer.h"
#include "DSP/TruePeakDetector.h"
#include "Analysis/MixAnalysisEngine.h"
#include "Mastering/MasteringChain.h"
#include "Learning/ProducerProfile.h"

/**
    EstacionFinalAudioProcessor
    ----------------------------
    Núcleo DSP del plugin. Responsabilidades:

      1. Recibir el bloque de audio del Master Bus.
      2. Alimentar el motor de análisis (loudness, true peak, espectro).
      3. Exponer al editor (GUI) el estado del análisis en tiempo real
         mediante estructuras thread-safe (no bloqueantes en el hilo de audio).
      4. Aplicar la cadena de procesamiento de mastering cuando el usuario
         confirma las correcciones sugeridas ("HACERLO MEJOR").

    Principio de diseño: el hilo de audio NUNCA debe bloquear ni asignar
    memoria. Todo el análisis pesado (espectral, generación de explicaciones)
    ocurre en un hilo de baja prioridad y se comunica a la GUI vía
    juce::AbstractFifo / std::atomic snapshots.
*/
class EstacionFinalAudioProcessor final : public juce::AudioProcessor,
                                            private juce::Timer
{
public:
    EstacionFinalAudioProcessor();
    ~EstacionFinalAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // API pública para la GUI: snapshot inmutable y thread-safe del estado
    // actual del análisis. La GUI lo consulta con un Timer (~30 Hz), nunca
    // desde el hilo de audio.
    MixAnalysisSnapshot getCurrentSnapshot() const { return analysisEngine.getSnapshot(); }

    // Dispara el flujo "HACERLO MEJOR": toma el snapshot actual del
    // análisis, construye el plan de corrección para los 6 vagones de
    // MasteringChain y activa la cadena. La transición de bypass a activo
    // se suaviza internamente en cada sub-módulo (SmoothedValue), evitando
    // clics, tal como exige la especificación.
    void requestAutoMaster();

    // "RESTAURAR": desactiva la cadena de corrección y vuelve a la señal
    // original (pre-mastering), sin perder el diagnóstico ya calculado.
    void restoreOriginal();

    // "COMPARAR": alterna A/B entre la señal corregida y la original.
    void setCompareBypassed (bool bypassCorrectionChain);
    bool isCompareBypassed() const { return masteringChain.isEnabled() == false; }

    const MasteringChain& getMasteringChain() const { return masteringChain; }

    ProducerProfile& getProducerProfile() { return producerProfile; }
    const ProducerProfile& getProducerProfile() const { return producerProfile; }

    /** Sondea el snapshot actual y registra en el perfil los hallazgos
        NUEVOS de esta sesión (una vez por categoría por sesión, no en
        cada llamada). Pensado para invocarse desde el Timer de la GUI
        (~30 Hz) o desde un hilo de baja prioridad; nunca desde el hilo
        de audio. */
    void pollAndRecordLearning();

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void timerCallback() override; // guardado periodico de ProducerProfile (Fase 5)
    static constexpr int profileSaveIntervalMs = 120000; // 2 minutos

    MixAnalysisEngine analysisEngine;
    MasteringChain masteringChain;
    ProducerProfile producerProfile;

    std::set<FindingCategory> sessionDetectedCategories;
    juce::String lastDestinoRecorded;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EstacionFinalAudioProcessor)
};
