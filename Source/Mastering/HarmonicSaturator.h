#pragma once

#include <JuceHeader.h>

/**
    HarmonicSaturator
    -------------------
    Saturación armónica sutil de bus (tipo "glue" de mastering), NO
    distorsión agresiva. Usa waveshaping con función tanh (saturación
    suave, predominantemente armónicos impares de baja orden) sobre 2x
    oversampling para minimizar aliasing generado por las no-linealidades.

    La cantidad de saturación (drive) es proporcional a la intervención
    autorizada por el usuario y se limita a un rango conservador —
    consistente con la filosofía de "mentor, no caja negra": el efecto
    debe ser perceptible como "calidez" sutil, no como coloración obvia.
*/
class HarmonicSaturator
{
public:
    void prepare (double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    /** intervencion: 0..1, mapeado a drive interno conservador. */
    void setIntervencion (float intervencion);

    void process (juce::AudioBuffer<float>& buffer);

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    juce::LinearSmoothedValue<float> driveSmoothed { 0.0f };

    // Rango de drive conservador: 0.0 (bypass efectivo) a 0.35
    // (saturación perceptible pero no agresiva). Valores mayores
    // corresponderían a un plugin de "color" dedicado, fuera del
    // alcance de una etapa de masterización transparente.
    static constexpr float maxDrive = 0.35f;

    static float softClipSample (float x, float drive);
};
