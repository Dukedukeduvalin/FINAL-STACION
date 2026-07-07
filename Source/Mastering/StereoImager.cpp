#include "StereoImager.h"

void StereoImager::prepare (double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = newSampleRate;
    juce::ignoreUnused (numChannels);

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock), 1 };

    for (auto* f : { &lowPassL, &lowPassR })
    {
        f->setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        f->setCutoffFrequency (crossoverHz);
        f->prepare (spec);
    }
    for (auto* f : { &highPassL, &highPassR })
    {
        f->setType (juce::dsp::LinkwitzRileyFilterType::highpass);
        f->setCutoffFrequency (crossoverHz);
        f->prepare (spec);
    }

    widthSmoothed.reset (sampleRate, 0.08); // 80 ms: transición de anchura inaudible como salto
    reset();
}

void StereoImager::reset()
{
    for (auto* f : { &lowPassL, &lowPassR, &highPassL, &highPassR })
        f->reset();

    widthSmoothed.setCurrentAndTargetValue (1.0f);
    correlationEstimate.store (1.0f);
}

void StereoImager::setWidth (float width)
{
    widthSmoothed.setTargetValue (juce::jlimit (0.5f, 1.5f, width));
}

void StereoImager::updateCorrelationEstimate (const juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() < 2)
    {
        correlationEstimate.store (1.0f);
        return;
    }

    const auto* left = buffer.getReadPointer (0);
    const auto* right = buffer.getReadPointer (1);
    const int n = buffer.getNumSamples();

    double sumLR = 0.0, sumLL = 0.0, sumRR = 0.0;
    for (int i = 0; i < n; ++i)
    {
        sumLR += static_cast<double> (left[i]) * right[i];
        sumLL += static_cast<double> (left[i]) * left[i];
        sumRR += static_cast<double> (right[i]) * right[i];
    }

    const double denom = std::sqrt (sumLL * sumRR);
    const float correlation = denom > 1e-9 ? static_cast<float> (sumLR / denom) : 1.0f;
    correlationEstimate.store (juce::jlimit (-1.0f, 1.0f, correlation));
}

void StereoImager::process (juce::AudioBuffer<float>& buffer)
{
    updateCorrelationEstimate (buffer);

    if (buffer.getNumChannels() < 2)
        return; // el ensanchamiento M/S no aplica a señales mono

    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (1);
    const int n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        const float width = widthSmoothed.getNextValue();

        // Separar banda de graves (mantenida ~mono) de la banda superior
        // (donde se aplica el ensanchamiento M/S).
        const float lowL = lowPassL.processSample (0, left[i]);
        const float lowR = lowPassR.processSample (0, right[i]);
        const float highL = highPassL.processSample (0, left[i]);
        const float highR = highPassR.processSample (0, right[i]);

        // Graves: promedio L/R (mono real), evita cancelaciones de fase
        // en sistemas de reproducción mono y en corte de vinilo.
        const float lowMono = 0.5f * (lowL + lowR);

        // Agudos/medios: codificación M/S con anchura variable.
        const float mid = 0.5f * (highL + highR);
        const float side = 0.5f * (highL - highR);
        const float widenedSide = side * width;

        const float highOutL = mid + widenedSide;
        const float highOutR = mid - widenedSide;

        left[i]  = lowMono + highOutL;
        right[i] = lowMono + highOutR;
    }
}
