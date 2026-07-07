#pragma once

#include <JuceHeader.h>
#include "../DSP/LoudnessAnalyzer.h"
#include "../DSP/TruePeakDetector.h"
#include "../DSP/SpectralAnalyzer.h"
#include "MixDiagnosis.h"

/**
    MixAnalysisEngine
    -------------------
    Orquesta los tres medidores (loudness, true peak, espectro), interpreta
    sus resultados y produce un MixAnalysisSnapshot listo para la GUI.

    Regla fundamental de la especificación aplicada aquí:
    "Nunca aplicar procesos sin justificación" / "Nunca inventar información".
    Por eso cada MixFinding se construye ÚNICAMENTE a partir de valores
    medidos por LoudnessAnalyzer / TruePeakDetector / SpectralAnalyzer.
    Los umbrales de decisión (p. ej. "más de 2 dB de exceso en una banda
    es un problema") están documentados y son públicos (ver referencias
    en el comentario de cada método de evaluación), no inventados ad hoc.
*/
class MixAnalysisEngine
{
public:
    MixAnalysisEngine() = default;

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    /** Se llama desde el hilo de audio (processBlock). No asigna memoria. */
    void processBlock (const juce::AudioBuffer<float>& buffer);

    /** Se llama desde el hilo de la GUI (Timer ~30 Hz). Lock-free. */
    MixAnalysisSnapshot getSnapshot() const;

    /** Cambia el destino activo (Spotify, Club, CD, etc.) y sus objetivos. */
    void setDestino (const juce::String& destinoName);

private:
    LoudnessAnalyzer loudness;
    TruePeakDetector truePeak;
    SpectralAnalyzer spectral;

    double sampleRate = 44100.0;
    juce::int64 samplesProcessed = 0;

    // Objetivo activo. Los valores por defecto siguen las recomendaciones
    // públicas de loudness normalization de cada plataforma (documentación
    // de streaming loudness, referencias EBU R128 / ITU-R BS.1770).
    struct DestinoTarget { juce::String nombre; float objetivoLUFS; float limiteTruePeakDb; };
    DestinoTarget currentTarget { "Spotify", -14.0f, -1.0f };

    static const std::vector<DestinoTarget>& allDestinos();

    // --- Evaluadores: cada uno produce 0..n MixFinding a partir de mediciones reales.
    void evaluateLoudness (MixAnalysisSnapshot& snap) const;
    void evaluateTruePeak (MixAnalysisSnapshot& snap) const;
    void evaluateSpectralBalance (MixAnalysisSnapshot& snap) const;

    static juce::String buildExplanation (const MixFinding& finding);
    static SemaphoreState computeOverallSemaphore (const std::vector<MixFinding>& findings);

    mutable juce::SpinLock snapshotLock;
};
