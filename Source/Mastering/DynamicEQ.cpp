#include "DynamicEQ.h"

void DynamicEQ::prepare (double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = newSampleRate;

    juce::dsp::ProcessSpec spec { sampleRate, static_cast<juce::uint32> (samplesPerBlock),
                                   static_cast<juce::uint32> (numChannels) };

    for (auto& f : filters)
    {
        f.state = juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 1000.0, 1.4, 1.0f);
        f.prepare (spec);
    }

    reset();
}

void DynamicEQ::reset()
{
    for (auto& f : filters)
        f.reset();

    const juce::SpinLock::ScopedLockType lock (planLock);
    for (auto& b : bands)
        b = BandCorrection {};
}

void DynamicEQ::setCorrectionPlan (const std::vector<MixFinding>& findings, float factorAplicado)
{
    factorAplicado = juce::jlimit (0.0f, 1.0f, factorAplicado);

    std::array<BandCorrection, maxBands> newBands;
    int idx = 0;

    for (const auto& f : findings)
    {
        if (idx >= maxBands)
            break;

        if (f.category != FindingCategory::spectralBalance)
            continue;

        BandCorrection bc;
        bc.active = true;
        bc.frequencyHz = juce::jlimit (20.0f, 20000.0f, f.frequencyHz);

        // La corrección aplicada es el exceso medido, atenuado por un
        // factor conservador (0.7) para evitar sobre-corrección audible,
        // y escalado por la intervención autorizada por el usuario.
        // Nunca se corrige más de lo que se midió: esto es la garantía
        // de "cada acción justificada técnicamente".
        const float rawCorrection = -f.measuredValueDb * 0.7f * factorAplicado;
        bc.gainDb = juce::jlimit (-9.0f, 0.0f, rawCorrection);

        // Q más ajustado (mayor Q) cuanto más puntual es el exceso medido,
        // para evitar afectar zonas del espectro que no presentaban problema.
        bc.q = f.measuredValueDb >= 6.0f ? 2.2f : 1.4f;

        newBands[static_cast<size_t> (idx)] = bc;
        ++idx;
    }

    {
        const juce::SpinLock::ScopedLockType lock (planLock);
        bands = newBands;
    }

    for (int i = 0; i < maxBands; ++i)
        updateFilterCoefficients (i);
}

void DynamicEQ::updateFilterCoefficients (int bandIndex)
{
    BandCorrection bc;
    {
        const juce::SpinLock::ScopedLockType lock (planLock);
        bc = bands[static_cast<size_t> (bandIndex)];
    }

    const float linearGain = bc.active ? juce::Decibels::decibelsToGain (bc.gainDb) : 1.0f;
    const float freq = bc.active ? bc.frequencyHz : 1000.0f;
    const float q = bc.active ? bc.q : 1.4f;

    // makePeakFilter con ganancia 1.0 (0 dB) cuando la banda está inactiva
    // equivale a un filtro neutro (sin coloración), evitando ramas
    // condicionales de bypass por banda en el hilo de audio.
    *filters[static_cast<size_t> (bandIndex)].state =
        *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, freq, q, juce::jmax (0.01f, linearGain));
}

void DynamicEQ::process (juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block (buffer);
    juce::dsp::ProcessContextReplacing<float> context (block);

    for (auto& f : filters)
        f.process (context);
}
