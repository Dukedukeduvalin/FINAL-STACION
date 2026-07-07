#pragma once

#include <JuceHeader.h>
#include <array>
#include <deque>

/**
    LoudnessAnalyzer
    -----------------
    Implementación de medición de loudness según ITU-R BS.1770-4 /
    EBU R128:

      1. Filtro K-weighting (pre-filtro shelving + filtro de altas).
      2. Ventaneo en bloques de 400 ms con solape del 75% (gating momentáneo
         y de corto plazo) para el cálculo de loudness integrado con
         "gating" relativo (-10 LU bajo la media absoluta -70 LUFS).

    Este módulo NO decide nada por sí mismo. Solo mide. La interpretación
    (semáforo, recomendaciones) vive en MixAnalysisEngine, conforme a la
    regla fundamental de la especificación: "nunca inventar información",
    "basar recomendaciones en principios públicos de ingeniería de audio".
*/
class LoudnessAnalyzer
{
public:
    LoudnessAnalyzer() = default;

    void prepare (double sampleRate, int numChannels);
    void reset();

    /** Procesa un bloque estéreo/mono. No modifica el buffer (solo lectura). */
    void processBlock (const juce::AudioBuffer<float>& buffer);

    // --- Lecturas actuales (thread-safe, lock-free) -------------------------
    float getMomentaryLUFS() const  { return momentaryLUFS.load(); }
    float getShortTermLUFS() const  { return shortTermLUFS.load(); }
    float getIntegratedLUFS() const { return integratedLUFS.load(); }
    float getLoudnessRangeLU() const { return loudnessRangeLU.load(); }

private:
    struct KWeightingFilter
    {
        // Etapa 1: pre-filtro shelving de altas frecuencias (+4 dB approx a 
        // partir de ~1.5 kHz), etapa 2: filtro de paso alto a ~38 Hz.
        juce::dsp::IIR::Filter<float> preFilter;
        juce::dsp::IIR::Filter<float> highPassFilter;

        void prepare (const juce::dsp::ProcessSpec& spec);
        void reset();
        float processSample (float x);
    };

    double sampleRate = 44100.0;
    int numChannels = 2;

    std::vector<KWeightingFilter> channelFilters;

    // Acumuladores para ventanas de 400 ms (gating momentáneo) con solape 75%.
    int samplesPerGatingBlock = 0;
    int hopSize = 0;
    std::vector<std::vector<float>> gatingRingBuffers; // por canal
    int ringWritePos = 0;

    std::deque<float> blockLoudnessHistory; // valores en LUFS de cada bloque de 400ms (gating momentáneo)
    static constexpr size_t maxHistoryBlocks = 3000; // ~10 minutos a 200ms hop

    std::atomic<float> momentaryLUFS  { -70.0f };
    std::atomic<float> shortTermLUFS  { -70.0f };
    std::atomic<float> integratedLUFS { -70.0f };
    std::atomic<float> loudnessRangeLU { 0.0f };

    int samplesSinceLastHop = 0;

    void computeBlockLoudnessAndPush();
    float meanSquareOfRingBuffer (int channel) const;
    static float meanSquareToLUFS (float meanSquareChannelSum);
};
