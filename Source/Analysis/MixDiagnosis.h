#pragma once

#include <JuceHeader.h>
#include <vector>

/**
    MixDiagnosis
    -------------
    Representa UN hallazgo técnico del análisis, con la justificación
    obligatoria que exige la especificación (botón "¿POR QUÉ?"):

        "Reducí 2.1 dB en 270 Hz porque existía acumulación de energía
         que reducía la claridad de la voz."

    Cada Finding se genera exclusivamente a partir de mediciones reales
    (LUFS, True Peak, energía por banda). Nunca se inventan valores:
    measuredValueDb / frequencyHz siempre provienen de DSP real.
*/

enum class FindingSeverity
{
    info,       // observación, no requiere acción
    suggestion, // amarillo — recomendable corregir
    critical    // rojo — no se recomienda publicar así
};

enum class FindingCategory
{
    loudnessIntegrated,
    loudnessRange,
    truePeak,
    spectralBalance,
    stereoImage,
    dynamicRange,
    headroom
};

struct MixFinding
{
    FindingCategory category;
    FindingSeverity severity;

    // Ubicación del problema, si aplica (p. ej. banda de frecuencia).
    // NaN cuando no corresponde (p. ej. hallazgos de loudness integrado).
    float frequencyHz = std::numeric_limits<float>::quiet_NaN();

    // Magnitud medida del problema (dB de exceso, dB por encima del
    // objetivo, LU de diferencia, etc.) — siempre un valor real medido,
    // nunca estimado o inventado.
    float measuredValueDb = 0.0f;

    // Texto generado de forma determinista a partir de los campos
    // anteriores (ver MixAnalysisEngine::buildExplanation). Es lo que
    // se muestra al presionar "¿POR QUÉ?".
    juce::String explanation;
};

enum class SemaphoreState { verde, amarillo, rojo };

struct MixAnalysisSnapshot
{
    // --- Mediciones crudas (siempre reales, provenientes de DSP) ----------
    float momentaryLUFS   = -70.0f;
    float shortTermLUFS   = -70.0f;
    float integratedLUFS  = -70.0f;
    float loudnessRangeLU = 0.0f;
    float truePeakDb      = -100.0f;
    float headroomDb      = 0.0f; // = objetivoTruePeak - truePeakDb actual

    // --- Estado interpretado -------------------------------------------
    SemaphoreState semaphore = SemaphoreState::amarillo;
    std::vector<MixFinding> findings;

    // --- Contexto del destino elegido -----------------------------------
    juce::String destinoActivo;
    float destinoObjetivoLUFS = -14.0f;
    float destinoLimiteTruePeakDb = -1.0f;

    bool hasEnoughDataToJudge = false; // false durante los primeros ~3s de audio

    // Niveles de energía por banda (dBFS suavizado), mismas 6 bandas que
    // SpectralAnalyzer (Sub, Bass, LowMid, Mid, HighMid, Air). Se exponen
    // aquí para que la GUI (Fase 3) pueda dibujar el balance espectral sin
    // depender directamente de SpectralAnalyzer.
    static constexpr int numSpectralBands = 6;
    std::array<float, numSpectralBands> spectralBandLevelsDb { -100.0f, -100.0f, -100.0f, -100.0f, -100.0f, -100.0f };
};
