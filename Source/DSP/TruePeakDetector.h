#pragma once

#include <JuceHeader.h>

/**
    TruePeakDetector
    -----------------
    Estima el nivel de True Peak (dBTP) según ITU-R BS.1770-4 Annex 2,
    usando sobremuestreo 4x con un filtro polifásico (juce::dsp::Oversampling)
    para detectar picos inter-muestra que un medidor de peak digital
    convencional no puede ver.

    Uso típico: comparar el resultado contra el límite del destino elegido
    (p. ej. -1.0 dBTP para streaming, -0.3 dBTP para CD) y alimentar el
    semáforo de MixAnalysisEngine.
*/
class TruePeakDetector
{
public:
    TruePeakDetector() = default;

    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    /** Analiza el bloque (solo lectura) y actualiza el peak retenido. */
    void processBlock (const juce::AudioBuffer<float>& buffer);

    /** Verdadero pico más alto (dBTP) desde el último reset()/prepare(). */
    float getTruePeakDb() const { return truePeakDb.load(); }

    /** Pico "instantáneo" del último bloque procesado, útil para medidores en vivo. */
    float getLastBlockTruePeakDb() const { return lastBlockTruePeakDb.load(); }

    void resetHeldPeak() { truePeakDb.store (-100.0f); }

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    int numChannels = 2;

    std::atomic<float> truePeakDb { -100.0f };
    std::atomic<float> lastBlockTruePeakDb { -100.0f };

    static constexpr int oversamplingFactorPow2 = 2; // 2^2 = 4x, conforme al mínimo de la norma
};
