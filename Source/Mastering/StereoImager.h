#pragma once

#include <JuceHeader.h>

/**
    StereoImager
    -------------
    Control de anchura estéreo mediante codificación Mid/Side, con dos
    bandas (graves/resto) separadas por un cruce a 200 Hz. Los graves se
    mantienen predominantemente mono (práctica estándar de mastering para
    compatibilidad de reproducción y evitar problemas de fase en sistemas
    mono/vinilo), mientras que el resto del espectro puede ensancharse
    con moderación.

    Este módulo no decide autónomamente la cantidad de ensanchamiento:
    MasteringChain lo fija a partir de hallazgos de categoría stereoImage
    (cuando existan) o lo deja neutro (anchura 1.0 = sin cambio) si no hay
    evidencia de un problema medido. Ensanchar sin justificación violaría
    la regla fundamental "nunca aplicar procesos sin justificación".
*/
class StereoImager
{
public:
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    /** width: 1.0 = sin cambio, <1.0 = más mono, >1.0 = más ancho (rango 0.5–1.5). */
    void setWidth (float width);

    void process (juce::AudioBuffer<float>& buffer);

    float getCurrentCorrelationEstimate() const { return correlationEstimate.load(); }

private:
    double sampleRate = 44100.0;
    juce::LinearSmoothedValue<float> widthSmoothed { 1.0f };

    juce::dsp::LinkwitzRileyFilter<float> lowPassL, lowPassR, highPassL, highPassR;
    static constexpr float crossoverHz = 200.0f;

    std::atomic<float> correlationEstimate { 1.0f };

    void updateCorrelationEstimate (const juce::AudioBuffer<float>& buffer);
};
