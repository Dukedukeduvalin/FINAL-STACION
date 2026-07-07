#include "TruePeakLimiter.h"

void TruePeakLimiter::prepare (double newSampleRate, int samplesPerBlock, int newNumChannels)
{
    sampleRate = newSampleRate;
    numChannels = juce::jmax (1, newNumChannels);

    oversampler = std::make_unique<juce::dsp::Oversampling<float>> (
        static_cast<size_t> (numChannels),
        2, // 4x oversampling
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true);
    oversampler->initProcessing (static_cast<size_t> (samplesPerBlock));

    lookAheadSamples = static_cast<int> (std::round (sampleRate * lookAheadMs / 1000.0));

    // Margen generoso: el host puede, en teoria, entregar bloques mas
    // pequenos que samplesPerBlock (nunca mayores, por contrato de JUCE),
    // asi que dimensionar con samplesPerBlock es seguro. Se anade un
    // margen x2 como salvaguarda documentada, no una garantia formal.
    delayBufferLength = lookAheadSamples + juce::jmax (samplesPerBlock * 2, 4096);

    delayLine.setSize (numChannels, delayBufferLength);
    gainHistory.assign (static_cast<size_t> (delayBufferLength), 1.0f);

    const double releaseSeconds = 0.050;
    releaseCoeff = static_cast<float> (std::exp (-1.0 / (releaseSeconds * sampleRate)));

    reset();
}

void TruePeakLimiter::reset()
{
    if (oversampler != nullptr)
        oversampler->reset();

    delayLine.clear();
    std::fill (gainHistory.begin(), gainHistory.end(), 1.0f);
    minWindowIndices.clear();
    writeIndex = 0;
    currentGainLinear = 1.0f;
    currentGainReductionDb.store (0.0f);
}

void TruePeakLimiter::setCeilingDb (float ceilingDb)
{
    ceilingDbValue = ceilingDb;
}

void TruePeakLimiter::process (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int channelsToProcess = juce::jmin (numChannels, buffer.getNumChannels());
    const float ceilingLinear = juce::Decibels::decibelsToGain (ceilingDbValue);

    // 1) Sobremuestrear el bloque de ENTRADA (aun no retrasado) para medir
    //    el True Peak real por muestra al sample rate nativo. Cada muestra
    //    original i corresponde a un tramo de `oversamplingFactor` muestras
    //    sobremuestreadas contiguas.
    juce::AudioBuffer<float> scratch;
    scratch.makeCopyOf (buffer, true);
    juce::dsp::AudioBlock<float> scratchBlock (scratch);
    auto oversampledBlock = oversampler->processSamplesUp (scratchBlock);

    const size_t totalOversampled = oversampledBlock.getNumSamples();
    const int oversamplingFactor = numSamples > 0
        ? static_cast<int> (totalOversampled / static_cast<size_t> (numSamples))
        : 1;

    for (int i = 0; i < numSamples; ++i)
    {
        // --- Paso A: True Peak de esta muestra original (todos los canales) ---
        float peakLinear = 0.0f;
        const size_t startOs = static_cast<size_t> (i) * static_cast<size_t> (oversamplingFactor);
        const size_t endOs = juce::jmin (totalOversampled, startOs + static_cast<size_t> (oversamplingFactor));

        for (size_t ch = 0; ch < oversampledBlock.getNumChannels(); ++ch)
        {
            const auto* data = oversampledBlock.getChannelPointer (ch);
            for (size_t os = startOs; os < endOs; ++os)
                peakLinear = juce::jmax (peakLinear, std::abs (data[os]));
        }

        const float targetGain = peakLinear > 1.0e-9f ? juce::jmin (1.0f, ceilingLinear / peakLinear) : 1.0f;

        // --- Paso B: escribir en el historial circular + linea de delay ---
        const int histIdx = bufferIndex (writeIndex);
        gainHistory[static_cast<size_t> (histIdx)] = targetGain;

        for (int ch = 0; ch < channelsToProcess; ++ch)
            delayLine.setSample (ch, histIdx, buffer.getSample (ch, i));

        // --- Paso C: mantener la ventana deslizante de minimo (deque monotono) ---
        while (! minWindowIndices.empty() && gainHistory[static_cast<size_t> (bufferIndex (minWindowIndices.back()))] >= targetGain)
            minWindowIndices.pop_back();
        minWindowIndices.push_back (writeIndex);

        while (! minWindowIndices.empty() && (writeIndex - minWindowIndices.front()) >= lookAheadSamples)
            minWindowIndices.pop_front();

        // --- Paso D: leer la muestra retrasada + su ganancia look-ahead ---
        const juce::int64 readAbsoluteIndex = writeIndex - lookAheadSamples + 1;

        float outputGainTarget = 1.0f;
        if (readAbsoluteIndex >= 0 && ! minWindowIndices.empty())
            outputGainTarget = gainHistory[static_cast<size_t> (bufferIndex (minWindowIndices.front()))];
        else if (readAbsoluteIndex < 0)
            outputGainTarget = 1.0f; // aun llenando el buffer de look-ahead: pasa en seco (silencio inicial tipico)

        // Suavizado adicional: ataque instantaneo hacia una ganancia menor
        // (nunca dejar pasar un pico), release exponencial hacia una mayor.
        if (outputGainTarget < currentGainLinear)
            currentGainLinear = outputGainTarget;
        else
            currentGainLinear = outputGainTarget + (currentGainLinear - outputGainTarget) * releaseCoeff;

        if (readAbsoluteIndex >= 0)
        {
            const int readIdx = bufferIndex (readAbsoluteIndex);
            for (int ch = 0; ch < channelsToProcess; ++ch)
            {
                const float delayedSample = delayLine.getSample (ch, readIdx);
                buffer.setSample (ch, i, delayedSample * currentGainLinear);
            }
        }
        else
        {
            // Todavia dentro de la latencia inicial de look-ahead: no hay
            // muestra retrasada valida que emitir. Se emite silencio en
            // vez de la muestra sin retrasar, para no romper el contrato
            // de latencia reportado al host (que ya compensa este
            // desfase en su linea de tiempo).
            for (int ch = 0; ch < channelsToProcess; ++ch)
                buffer.setSample (ch, i, 0.0f);
        }

        ++writeIndex;
    }

    oversampler->reset(); // evita arrastrar estado de filtro entre bloques discontinuos de medicion

    const float reductionDb = -juce::Decibels::gainToDecibels (juce::jmax (1.0e-6f, currentGainLinear));
    currentGainReductionDb.store (juce::jmax (0.0f, reductionDb));
}
