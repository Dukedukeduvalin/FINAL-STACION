#include "MasteringCompressor.h"

void MasteringCompressor::prepare (double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock),
                                   static_cast<juce::uint32> (numChannels) };
    compressor.prepare (spec);

    // Attack/release conservadores de mastering: lo suficientemente lentos
    // para no distorsionar transitorios de bombo/voz, lo suficientemente
    // rápidos para reaccionar a picos de energía sostenidos.
    compressor.setAttack (12.0f);
    compressor.setRelease (140.0f);
    compressor.setThreshold (-18.0f);
    compressor.setRatio (1.5f);

    reset();
}

void MasteringCompressor::reset()
{
    compressor.reset();
    currentRatio.store (1.0f);
    currentThresholdDb.store (-18.0f);
    currentGainReductionDb.store (0.0f);
}

void MasteringCompressor::updateFromAnalysis (float measuredLraLu, float intervencion)
{
    intervencion = juce::jlimit (0.0f, 1.0f, intervencion);

    // Mapeo documentado: LRA alto (mezcla poco compactada, > 10 LU) permite
    // un ratio algo mayor sin sonar artificial; LRA bajo (mezcla ya
    // comprimida, < 5 LU) exige ratio mínimo para no sobre-comprimir.
    // Rango de salida: 1.3:1 (LRA bajo) a 3.0:1 (LRA alto), escalado por
    // la intervención autorizada por el usuario.
    const float lraNormalised = juce::jlimit (0.0f, 1.0f, (measuredLraLu - 4.0f) / 12.0f);
    const float targetRatio = 1.3f + lraNormalised * 1.7f * intervencion;

    // Threshold: relativo al RMS de entrada REAL medido (media movil
    // exponencial de ~1s, actualizada en process()), no un valor absoluto
    // fijo. Se fija 3 dB por debajo del RMS medido, un margen conservador
    // de "engagement" suave tipico de compresion de mastering (evita
    // comprimir el grueso de la señal, solo los picks de energia por
    // encima del promedio del programa).
    const float measuredInputRms = smoothedInputRmsDb.load();
    const float targetThreshold = juce::jlimit (-40.0f, -6.0f, measuredInputRms - 3.0f);

    compressor.setRatio (targetRatio);
    compressor.setThreshold (targetThreshold);

    currentRatio.store (targetRatio);
    currentThresholdDb.store (targetThreshold);
}

float MasteringCompressor::computeBlockRmsDb (const juce::AudioBuffer<float>& buffer) const
{
    double sumSquares = 0.0;
    int totalSamples = 0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer (ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            sumSquares += static_cast<double> (data[i]) * static_cast<double> (data[i]);
            ++totalSamples;
        }
    }

    if (totalSamples == 0)
        return -100.0f;

    const float rms = static_cast<float> (std::sqrt (sumSquares / totalSamples));
    return juce::Decibels::gainToDecibels (rms, -100.0f);
}

void MasteringCompressor::process (juce::AudioBuffer<float>& buffer)
{
    const float rmsBefore = computeBlockRmsDb (buffer);

    // Actualizar el RMS de entrada suavizado ANTES de comprimir (media
    // movil exponencial lenta), usado por updateFromAnalysis() para fijar
    // un threshold relativo a la señal real en vez de un valor absoluto fijo.
    const float previousSmoothed = smoothedInputRmsDb.load();
    const float updatedSmoothed = (previousSmoothed <= -99.0f)
                                      ? rmsBefore
                                      : (rmsSmoothingCoeff * previousSmoothed + (1.0f - rmsSmoothingCoeff) * rmsBefore);
    smoothedInputRmsDb.store (updatedSmoothed);

    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);
    compressor.process (context);

    const float rmsAfter = computeBlockRmsDb (buffer);

    currentGainReductionDb.store (juce::jmax (0.0f, rmsBefore - rmsAfter));
}
