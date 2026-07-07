#pragma once

#include <JuceHeader.h>
#include "DynamicEQ.h"
#include "MasteringCompressor.h"
#include "HarmonicSaturator.h"
#include "StereoImager.h"
#include "TruePeakLimiter.h"
#include "../Analysis/MixDiagnosis.h"

/**
    MasteringChain
    ----------------
    Orquesta los "vagones" definidos en la especificación, en el orden
    documentado: EQ -> Compresión -> Saturación -> Imagen Estéreo ->
    Limitador -> Control Final.

    Este es el módulo que el botón "HACERLO MEJOR" activa. Su
    responsabilidad es doble:

      1. Traducir un MixAnalysisSnapshot (diagnóstico) en un plan de
         corrección concreto para cada vagón (updatePlanFromSnapshot).
      2. Procesar el audio del Master Bus aplicando ese plan
         (processBlock), de forma segura para el hilo de audio (sin
         asignaciones dinámicas dentro de processBlock).

    El plan solo se recalcula cuando la GUI llama a updatePlanFromSnapshot
    (al presionar "HACERLO MEJOR" o periódicamente si el usuario deja el
    modo automático activo) — nunca dentro de processBlock.
*/
class MasteringChain
{
public:
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    /** Construye el plan de corrección de todos los vagones a partir del
        diagnóstico actual. Debe llamarse fuera del hilo de audio (desde
        el hilo de mensajes/GUI, o desde un hilo de baja prioridad). */
    void updatePlanFromSnapshot (const MixAnalysisSnapshot& snapshot, float intervencion01);

    /** Procesa el bloque de audio aplicando la cadena completa. Se llama
        desde processBlock del AudioProcessor. Thread-safe respecto a
        updatePlanFromSnapshot gracias a que cada sub-módulo actualiza sus
        coeficientes de forma atómica/lock-free internamente. */
    void process (juce::AudioBuffer<float>& buffer);

    void setEnabled (bool shouldBeEnabled);
    bool isEnabled() const { return targetEnabled; }

    int getLatencySamples() const { return limiter.getLatencySamples(); }

    // --- Estado expuesto a la GUI (para el panel "Master" de la Fase 3) ---
    float getCompressorRatio() const { return compressor.getCurrentRatio(); }
    float getCompressorGainReductionDb() const { return compressor.getCurrentGainReductionDb(); }
    float getLimiterGainReductionDb() const { return limiter.getCurrentGainReductionDb(); }
    float getStereoCorrelation() const { return imager.getCurrentCorrelationEstimate(); }
    const std::array<DynamicEQ::BandCorrection, DynamicEQ::maxBands>& getEqBands() const { return eq.getActiveBands(); }

private:
    DynamicEQ eq;
    MasteringCompressor compressor;
    HarmonicSaturator saturator;
    StereoImager imager;
    TruePeakLimiter limiter;

    bool targetEnabled = false;
    juce::LinearSmoothedValue<float> mixSmoothed { 0.0f }; // 0 = 100% dry, 1 = 100% wet
    juce::AudioBuffer<float> dryBuffer;
    static constexpr double mixRampSeconds = 0.05; // 50 ms: crossfade audible-free entre dry/wet
};
