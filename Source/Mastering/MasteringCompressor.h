#pragma once

#include <JuceHeader.h>

/**
    MasteringCompressor
    ---------------------
    Compresión de bus estéreo (link L/R) con detección RMS, diseñada para
    control de rango dinámico sutil en mastering (no para "aplastar" la
    señal). Envoltura sobre juce::dsp::Compressor con lógica de ratio
    adaptativo: cuanto mayor el LRA medido por LoudnessAnalyzer, mayor el
    ratio aplicado — dentro de límites conservadores típicos de mastering
    (1.5:1 a 3:1), nunca ratios de tipo "buss compression" agresivos que
    corresponderían a una etapa de mezcla, no de masterización.
*/
class MasteringCompressor
{
public:
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    /** measuredLraLu: LRA medido actual (LU). intervencion: 0..1. */
    void updateFromAnalysis (float measuredLraLu, float intervencion);

    void process (juce::AudioBuffer<float>& buffer);

    float getCurrentRatio() const { return currentRatio.load(); }
    float getCurrentThresholdDb() const { return currentThresholdDb.load(); }
    float getCurrentGainReductionDb() const { return currentGainReductionDb.load(); }

private:
    juce::dsp::Compressor<float> compressor;
    double sampleRate = 44100.0;

    std::atomic<float> currentRatio { 1.0f };
    std::atomic<float> currentThresholdDb { 0.0f };
    std::atomic<float> currentGainReductionDb { 0.0f };

    // RMS de entrada suavizado (media movil exponencial lenta, ~1s de
    // constante de tiempo), usado para derivar el threshold de forma
    // relativa a la señal real en vez de un valor absoluto fijo.
    std::atomic<float> smoothedInputRmsDb { -18.0f };
    static constexpr float rmsSmoothingCoeff = 0.98f;

    float computeBlockRmsDb (const juce::AudioBuffer<float>& buffer) const;
};
