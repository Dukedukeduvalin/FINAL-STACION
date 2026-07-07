#pragma once

#include <JuceHeader.h>
#include "../Analysis/MixDiagnosis.h"

/**
    DynamicEQ
    ----------
    Aplica correcciones de banda (campana/notch) generadas a partir de los
    MixFinding de categoría spectralBalance producidos por MixAnalysisEngine.

    Diseño: banco fijo de hasta 6 filtros paramétricos (uno por banda de
    SpectralAnalyzer), cada uno actualizable en caliente sin clics gracias
    al smoothing interno de juce::dsp::IIR::Coefficients + rampa de
    ganancia con SmoothedValue.

    Este módulo NO decide qué corregir — solo ejecuta la corrección que
    MasteringChain le indica a partir del diagnóstico. Mantiene la regla
    "cada acción deberá estar justificada técnicamente": la ganancia
    aplicada en cada banda es exactamente el exceso medido (con un factor
    de corrección conservador, ver applyCorrectionPlan), nunca un valor
    arbitrario.
*/
class DynamicEQ
{
public:
    static constexpr int maxBands = 6;

    struct BandCorrection
    {
        bool active = false;
        float frequencyHz = 1000.0f;
        float gainDb = 0.0f;   // negativo = atenuación (caso típico de corrección)
        float q = 1.4f;        // Q moderado: corrige sin sonar quirúrgico/artificial
    };

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    /** Recalcula el plan de corrección a partir de los hallazgos actuales.
        factorAplicado (0..1) escala la magnitud de la corrección — se
        alimenta desde el parámetro "intervencion" del usuario. */
    void setCorrectionPlan (const std::vector<MixFinding>& findings, float factorAplicado);

    void process (juce::AudioBuffer<float>& buffer);

    const std::array<BandCorrection, maxBands>& getActiveBands() const { return bands; }

private:
    std::array<BandCorrection, maxBands> bands;
    std::array<juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                                juce::dsp::IIR::Coefficients<float>>, maxBands> filters;

    double sampleRate = 44100.0;
    juce::SpinLock planLock;

    void updateFilterCoefficients (int bandIndex);
};
