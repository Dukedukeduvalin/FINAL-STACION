#include "LoudnessAnalyzer.h"

// ---------------------------------------------------------------------------
// K-weighting filter (ITU-R BS.1770-4, coeficientes de referencia a 48 kHz,
// recalculados por muestreo para otras sample rates mediante bilinear
// transform con prewarping — juce::dsp::IIR::Coefficients maneja esto).
// ---------------------------------------------------------------------------
void LoudnessAnalyzer::KWeightingFilter::prepare (const juce::dsp::ProcessSpec& spec)
{
    const auto sr = spec.sampleRate;

    // Etapa 1: high-shelf, +4 dB por encima de ~1681 Hz (pre-filter de la norma)
    auto preCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sr, 1681.0, 0.7071, juce::Decibels::decibelsToGain (4.0f));
    preFilter.coefficients = preCoeffs;

    // Etapa 2: paso alto de segundo orden ~38 Hz (RLB weighting)
    auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 38.0, 0.5);
    highPassFilter.coefficients = hpCoeffs;

    preFilter.prepare (spec);
    highPassFilter.prepare (spec);
}

void LoudnessAnalyzer::KWeightingFilter::reset()
{
    preFilter.reset();
    highPassFilter.reset();
}

float LoudnessAnalyzer::KWeightingFilter::processSample (float x)
{
    x = preFilter.processSample (x);
    x = highPassFilter.processSample (x);
    return x;
}

// ---------------------------------------------------------------------------
void LoudnessAnalyzer::prepare (double newSampleRate, int newNumChannels)
{
    sampleRate = newSampleRate;
    numChannels = juce::jmax (1, newNumChannels);

    channelFilters.clear();
    channelFilters.resize (static_cast<size_t> (numChannels));
    juce::dsp::ProcessSpec spec { sampleRate, 512, static_cast<juce::uint32> (numChannels) };
    for (auto& f : channelFilters)
        f.prepare (spec);

    // Bloques de 400 ms con hop de 100 ms (75% solape) -> gating momentáneo.
    samplesPerGatingBlock = static_cast<int> (std::round (0.400 * sampleRate));
    hopSize               = static_cast<int> (std::round (0.100 * sampleRate));

    gatingRingBuffers.assign (static_cast<size_t> (numChannels),
                               std::vector<float> (static_cast<size_t> (samplesPerGatingBlock), 0.0f));
    ringWritePos = 0;
    samplesSinceLastHop = 0;

    reset();
}

void LoudnessAnalyzer::reset()
{
    for (auto& f : channelFilters)
        f.reset();

    for (auto& ring : gatingRingBuffers)
        std::fill (ring.begin(), ring.end(), 0.0f);

    blockLoudnessHistory.clear();
    ringWritePos = 0;
    samplesSinceLastHop = 0;

    momentaryLUFS.store (-70.0f);
    shortTermLUFS.store (-70.0f);
    integratedLUFS.store (-70.0f);
    loudnessRangeLU.store (0.0f);
}

float LoudnessAnalyzer::meanSquareOfRingBuffer (int channel) const
{
    const auto& ring = gatingRingBuffers[static_cast<size_t> (channel)];
    double sum = 0.0;
    for (auto v : ring)
        sum += static_cast<double> (v) * static_cast<double> (v);
    return static_cast<float> (sum / static_cast<double> (ring.size()));
}

float LoudnessAnalyzer::meanSquareToLUFS (float meanSquareChannelSum)
{
    // L = -0.691 + 10*log10(sum_canales(G_i * mean_square_i))
    // Con G_i = 1.0 para L/R (canales frontales), conforme BS.1770 para
    // configuración estéreo estándar sin canales surround.
    if (meanSquareChannelSum <= 1.0e-12f)
        return -70.0f; // piso práctico (silencio digital)

    return -0.691f + 10.0f * std::log10 (meanSquareChannelSum);
}

void LoudnessAnalyzer::computeBlockLoudnessAndPush()
{
    float channelSum = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
        channelSum += meanSquareOfRingBuffer (ch);

    const float blockLUFS = meanSquareToLUFS (channelSum);

    momentaryLUFS.store (blockLUFS);

    blockLoudnessHistory.push_back (blockLUFS);
    if (blockLoudnessHistory.size() > maxHistoryBlocks)
        blockLoudnessHistory.pop_front();

    // --- Short-term (últimos 3 s => 30 bloques de hop 100ms) ---------------
    {
        const int windowBlocks = 30;
        const int n = static_cast<int> (blockLoudnessHistory.size());
        const int start = juce::jmax (0, n - windowBlocks);

        double sumEnergy = 0.0;
        int count = 0;
        for (int i = start; i < n; ++i)
        {
            const float lufs = blockLoudnessHistory[static_cast<size_t> (i)];
            sumEnergy += std::pow (10.0, (lufs + 0.691) / 10.0);
            ++count;
        }
        if (count > 0)
        {
            const float meanEnergy = static_cast<float> (sumEnergy / count);
            shortTermLUFS.store (meanSquareToLUFS (meanEnergy));
        }
    }

    // --- Integrado con gating relativo (norma BS.1770) ----------------------
    // Paso 1: gating absoluto a -70 LUFS.
    std::vector<float> passedAbsolute;
    passedAbsolute.reserve (blockLoudnessHistory.size());
    for (auto lufs : blockLoudnessHistory)
        if (lufs > -70.0f)
            passedAbsolute.push_back (lufs);

    if (! passedAbsolute.empty())
    {
        double sumEnergy = 0.0;
        for (auto lufs : passedAbsolute)
            sumEnergy += std::pow (10.0, (lufs + 0.691) / 10.0);
        const float ungatedMean = static_cast<float> (sumEnergy / passedAbsolute.size());
        const float relativeThreshold = meanSquareToLUFS (ungatedMean) - 10.0f;

        // Paso 2: gating relativo a (media - 10 LU).
        double sumEnergyGated = 0.0;
        int countGated = 0;
        for (auto lufs : passedAbsolute)
        {
            if (lufs > relativeThreshold)
            {
                sumEnergyGated += std::pow (10.0, (lufs + 0.691) / 10.0);
                ++countGated;
            }
        }

        if (countGated > 0)
        {
            const float gatedMean = static_cast<float> (sumEnergyGated / countGated);
            integratedLUFS.store (meanSquareToLUFS (gatedMean));

            // Loudness Range (LRA) simplificado: percentil 95 - percentil 10
            // de los bloques que superan el gating relativo.
            std::vector<float> gatedValues;
            gatedValues.reserve (passedAbsolute.size());
            for (auto lufs : passedAbsolute)
                if (lufs > relativeThreshold)
                    gatedValues.push_back (lufs);

            std::sort (gatedValues.begin(), gatedValues.end());
            if (gatedValues.size() >= 2)
            {
                const auto idxLow  = static_cast<size_t> (0.10 * static_cast<double> (gatedValues.size() - 1));
                const auto idxHigh = static_cast<size_t> (0.95 * static_cast<double> (gatedValues.size() - 1));
                loudnessRangeLU.store (gatedValues[idxHigh] - gatedValues[idxLow]);
            }
        }
    }
}

void LoudnessAnalyzer::processBlock (const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int channelsToProcess = juce::jmin (numChannels, buffer.getNumChannels());

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < channelsToProcess; ++ch)
        {
            const float raw = buffer.getSample (ch, i);
            const float weighted = channelFilters[static_cast<size_t> (ch)].processSample (raw);
            gatingRingBuffers[static_cast<size_t> (ch)][static_cast<size_t> (ringWritePos)] = weighted;
        }

        ringWritePos = (ringWritePos + 1) % samplesPerGatingBlock;
        ++samplesSinceLastHop;

        if (samplesSinceLastHop >= hopSize)
        {
            samplesSinceLastHop = 0;
            computeBlockLoudnessAndPush();
        }
    }
}
