#include "TruePeakDetector.h"

void TruePeakDetector::prepare (double sampleRate, int samplesPerBlock, int newNumChannels)
{
    numChannels = juce::jmax (1, newNumChannels);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        static_cast<size_t> (numChannels),
        oversamplingFactorPow2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true /* maximum quality */);

    oversampler->initProcessing (static_cast<size_t> (samplesPerBlock));
    juce::ignoreUnused (sampleRate);

    reset();
}

void TruePeakDetector::reset()
{
    if (oversampler != nullptr)
        oversampler->reset();

    truePeakDb.store (-100.0f);
    lastBlockTruePeakDb.store (-100.0f);
}

void TruePeakDetector::processBlock (const juce::AudioBuffer<float>& buffer)
{
    if (oversampler == nullptr)
        return;

    // El oversampler de JUCE requiere su propio buffer de trabajo; se copia
    // la señal de entrada (no se modifica el buffer original del processBlock
    // del plugin — este detector es puramente de medición).
    juce::AudioBuffer<float> scratch;
    scratch.makeCopyOf (buffer, true);

    juce::dsp::AudioBlock<float> block (scratch);
    auto oversampledBlock = oversampler->processSamplesUp (block);

    float peakLinear = 0.0f;
    for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
    {
        const auto* data = oversampledBlock.getChannelPointer (ch);
        for (size_t i = 0; i < oversampledBlock.getNumSamples(); ++i)
            peakLinear = juce::jmax (peakLinear, std::abs (data[i]));
    }

    oversampler->reset(); // evita arrastrar estado de latencia entre bloques de medición

    const float peakDb = juce::Decibels::gainToDecibels (peakLinear, -100.0f);

    lastBlockTruePeakDb.store (peakDb);

    float heldPeak = truePeakDb.load();
    if (peakDb > heldPeak)
        truePeakDb.store (peakDb);
}
