#pragma once

#include <JuceHeader.h>
#include <deque>

/**
    TruePeakLimiter
    -----------------
    Limitador look-ahead REAL con detección de True Peak (4x oversampling).

    Diseño (ver TruePeakLimiter.cpp para el detalle del algoritmo):
    la señal de audio se retrasa `lookAheadSamples` muestras mediante una
    línea de delay circular. En paralelo, se calcula la ganancia objetivo
    de cada muestra (a partir de su True Peak sobremuestreado) y se
    mantiene una ventana deslizante de mínimo (deque monótono, O(1)
    amortizado) de tamaño `lookAheadSamples` sobre ese historial de
    ganancias. Gracias al retraso, en el momento de emitir la muestra
    retrasada ya se conoce su ganancia "futura" real dentro de la ventana,
    permitiendo anticipar la reducción de ganancia ANTES de que el
    transitorio llegue — la definición correcta de look-ahead, a
    diferencia de la Fase 2 (que medía y aplicaba sobre el mismo bloque).
*/
class TruePeakLimiter
{
public:
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void setCeilingDb (float ceilingDb);

    /** Latencia introducida por el look-ahead, en muestras al sample rate
        nativo. Debe reportarse al host vía AudioProcessor::setLatencySamples. */
    int getLatencySamples() const { return lookAheadSamples; }

    void process (juce::AudioBuffer<float>& buffer);

    float getCurrentGainReductionDb() const { return currentGainReductionDb.load(); }

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;

    double sampleRate = 44100.0;
    int numChannels = 2;

    float ceilingDbValue = -1.0f;
    std::atomic<float> currentGainReductionDb { 0.0f };

    static constexpr int lookAheadMs = 5;
    int lookAheadSamples = 0;

    // Linea de delay circular (audio "seco", sin procesar) por canal.
    juce::AudioBuffer<float> delayLine;
    int delayBufferLength = 0;

    // Historial circular de ganancia objetivo (pre-look-ahead) por muestra,
    // indexado por posicion absoluta modulo delayBufferLength.
    std::vector<float> gainHistory;

    // Ventana deslizante de minimo: deque de posiciones absolutas
    // (juce::int64) cuyo valor de ganancia es monotonamente creciente
    // dentro de la ventana actual — el frente es siempre el minimo vigente.
    std::deque<juce::int64> minWindowIndices;

    juce::int64 writeIndex = 0; // contador absoluto de muestras escritas desde reset()

    // Suavizado adicional de release sobre la ganancia ya determinada por
    // la ventana de minimo, para evitar transiciones en escalon entre
    // muestras consecutivas de la ventana.
    float currentGainLinear = 1.0f;
    float releaseCoeff = 0.9995f;

    int bufferIndex (juce::int64 absoluteIndex) const
    {
        return static_cast<int> (((absoluteIndex % delayBufferLength) + delayBufferLength) % delayBufferLength);
    }
};

