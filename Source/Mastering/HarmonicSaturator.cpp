#include "HarmonicSaturator.h"

void HarmonicSaturator::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        static_cast<size_t> (numChannels),
        1, // 2x oversampling: suficiente para saturación suave, sin coste excesivo de CPU
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true);

    oversampler->initProcessing (static_cast<size_t> (samplesPerBlock));

    driveSmoothed.reset (sampleRate, 0.05); // 50 ms de rampa: evita zipper noise al cambiar intervención
    reset();
}

void HarmonicSaturator::reset()
{
    if (oversampler != nullptr)
        oversampler->reset();
    driveSmoothed.setCurrentAndTargetValue (0.0f);
}

void HarmonicSaturator::setIntervencion (float intervencion)
{
    intervencion = juce::jlimit (0.0f, 1.0f, intervencion);
    driveSmoothed.setTargetValue (intervencion * maxDrive);
}

float HarmonicSaturator::softClipSample (float x, float drive)
{
    if (drive <= 0.0001f)
        return x;

    // tanh waveshaping con compensación de ganancia de salida para
    // mantener el nivel percibido aproximadamente constante (evita que
    // "más saturación" se perciba simplemente como "más volumen").
    const float driven = x * (1.0f + drive * 4.0f);
    const float shaped = std::tanh (driven);
    const float makeupGain = 1.0f / std::tanh (1.0f + drive * 4.0f);
    return shaped * makeupGain;
}

void HarmonicSaturator::process (juce::AudioBuffer<float>& buffer)
{
    const float drive = driveSmoothed.getNextValue();
    if (drive <= 0.0001f)
        return; // bypass real: sin coste de oversampling si no hay drive activo

    juce::dsp::AudioBlock<float> block (buffer);
    auto oversampledBlock = oversampler->processSamplesUp (block);

    for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
    {
        auto* data = oversampledBlock.getChannelPointer (ch);
        for (size_t i = 0; i < oversampledBlock.getNumSamples(); ++i)
            data[i] = softClipSample (data[i], drive);
    }

    oversampler->processSamplesDown (block);
}
