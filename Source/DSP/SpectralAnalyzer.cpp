#include "SpectralAnalyzer.h"

void SpectralAnalyzer::prepare (double newSampleRate, int /*samplesPerBlock*/)
{
    sampleRate = newSampleRate;
    reset();
}

void SpectralAnalyzer::reset()
{
    circularBuffer.fill (0.0f);
    fftBuffer.fill (0.0f);
    lastMagnitudesLinear.fill (0.0f);
    writePos = 0;
    samplesSinceLastFFT = 0;

    for (auto& v : bandLevelsDb) v.store (-100.0f);
    for (auto& v : peakFrequencyPerBand) v.store (0.0f);
}

void SpectralAnalyzer::processBlock (const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const int hop = fftSize / 4; // 75% de solape, consistente con estándares de análisis espectral en tiempo real

    for (int i = 0; i < numSamples; ++i)
    {
        float mono = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            mono += buffer.getSample (ch, i);
        mono /= static_cast<float> (juce::jmax (1, numChannels));

        circularBuffer[static_cast<size_t> (writePos)] = mono;
        writePos = (writePos + 1) % fftSize;

        if (++samplesSinceLastFFT >= hop)
        {
            samplesSinceLastFFT = 0;
            performFFTAndUpdateBands();
        }
    }
}

void SpectralAnalyzer::performFFTAndUpdateBands()
{
    // Reordenar el buffer circular a orden lineal (más antiguo -> más reciente).
    for (int i = 0; i < fftSize; ++i)
    {
        const int idx = (writePos + i) % fftSize;
        fftBuffer[static_cast<size_t> (i)] = circularBuffer[static_cast<size_t> (idx)];
    }
    std::fill (fftBuffer.begin() + fftSize, fftBuffer.end(), 0.0f);

    window.multiplyWithWindowingTable (fftBuffer.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform (fftBuffer.data());

    {
        const juce::SpinLock::ScopedLockType lock (magnitudesLock);
        for (int bin = 0; bin < fftSize / 2; ++bin)
            lastMagnitudesLinear[static_cast<size_t> (bin)] = fftBuffer[static_cast<size_t> (bin)];
    }

    const double binHz = sampleRate / static_cast<double> (fftSize);

    for (int b = 0; b < NumBands; ++b)
    {
        const int binLow  = juce::jmax (1, static_cast<int> (std::floor (bandLowHz[b]  / binHz)));
        const int binHigh = juce::jmin (fftSize / 2 - 1, static_cast<int> (std::ceil  (bandHighHz[b] / binHz)));

        double sumSquares = 0.0;
        int count = 0;
        float peakMag = 0.0f;
        int peakBin = binLow;

        for (int bin = binLow; bin <= binHigh; ++bin)
        {
            const float mag = fftBuffer[static_cast<size_t> (bin)];
            sumSquares += static_cast<double> (mag) * static_cast<double> (mag);
            ++count;
            if (mag > peakMag)
            {
                peakMag = mag;
                peakBin = bin;
            }
        }

        const float rms = count > 0 ? static_cast<float> (std::sqrt (sumSquares / count)) : 0.0f;
        const float rmsDb = juce::Decibels::gainToDecibels (rms, -100.0f);

        const float previous = bandLevelsDb[b].load();
        const float smoothed = (previous <= -99.0f) ? rmsDb
                                                       : (smoothingCoeff * previous + (1.0f - smoothingCoeff) * rmsDb);
        bandLevelsDb[b].store (smoothed);

        peakFrequencyPerBand[b].store (static_cast<float> (peakBin * binHz));
    }
}

float SpectralAnalyzer::getPeakFrequencyInBand (Band band) const
{
    return peakFrequencyPerBand[band].load();
}
