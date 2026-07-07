#include "PluginProcessor.h"
#include "PluginEditor.h"

EstacionFinalAudioProcessor::EstacionFinalAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

EstacionFinalAudioProcessor::~EstacionFinalAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout EstacionFinalAudioProcessor::createParameterLayout()
{
    using Param = juce::AudioParameterFloat;
    using Range = juce::NormalisableRange<float>;

    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Destino de loudness objetivo (ver DESTINOS en la especificación).
    // Se representa como choice para que la GUI pueda listar:
    // Spotify, Apple Music, YouTube, TikTok, Club, Radio, CD, Vinilo, Archivo Maestro.
    juce::StringArray destinos {
        "Spotify", "Apple Music", "YouTube", "TikTok",
        "Club", "Radio", "CD", "Vinilo", "Archivo Maestro"
    };
    params.push_back (std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "destino", 1 }, "Destino", destinos, 0));

    // Cantidad de intervención permitida al motor "HACERLO MEJOR" (0 = solo
    // diagnóstico, 100 = corrección completa autorizada).
    params.push_back (std::make_unique<Param>(
        juce::ParameterID { "intervencion", 1 }, "Intervencion",
        Range { 0.0f, 100.0f, 1.0f }, 75.0f));

    // Bypass global de la cadena de corrección (el análisis sigue activo).
    params.push_back (std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "bypassCadena", 1 }, "Bypass Cadena", false));

    return { params.begin(), params.end() };
}

void EstacionFinalAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    analysisEngine.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    masteringChain.prepare (sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    setLatencySamples (masteringChain.getLatencySamples());

    sessionDetectedCategories.clear();
    producerProfile.recordSessionStart();

    // Guardado periodico: mitiga la perdida de datos si el host cierra
    // de forma abrupta (releaseResources() no esta garantizado en ese
    // caso). Se ejecuta en el hilo de mensajes (juce::Timer), nunca en
    // el hilo de audio.
    startTimer (profileSaveIntervalMs);
}

void EstacionFinalAudioProcessor::releaseResources()
{
    stopTimer();
    analysisEngine.reset();
    masteringChain.reset();
    producerProfile.recordSessionEnd(); // persiste a disco (ver ProducerProfile::recordSessionEnd)
}

void EstacionFinalAudioProcessor::timerCallback()
{
    producerProfile.saveToDisk();
}

bool EstacionFinalAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto mainOut = layouts.getMainOutputChannelSet();
    if (mainOut != juce::AudioChannelSet::stereo() && mainOut != juce::AudioChannelSet::mono())
        return false;

    return layouts.getMainInputChannelSet() == mainOut;
}

void EstacionFinalAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto ch = totalNumInputChannels; ch < totalNumOutputChannels; ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    // 1) Alimentar el motor de análisis con el bloque tal cual llega
    //    (pre-corrección). Este paso es obligatorio para el semáforo,
    //    LUFS, True Peak y detección de problemas espectrales.
    analysisEngine.processBlock (buffer);

    // 2) Cadena de corrección: se aplica solo si el usuario confirmó
    //    "HACERLO MEJOR" (masteringChain.isEnabled()) y el bypass global
    //    del parámetro no está activo. El plan de corrección en sí NO se
    //    recalcula aquí (eso ocurre en requestAutoMaster, fuera del hilo
    //    de audio); aquí solo se ejecuta el procesamiento con los
    //    coeficientes ya calculados.
    const bool bypassed = apvts.getRawParameterValue ("bypassCadena")->load() > 0.5f;
    if (! bypassed)
        masteringChain.process (buffer);
}

void EstacionFinalAudioProcessor::requestAutoMaster()
{
    // Flujo completo exigido por la especificación para "HACERLO MEJOR":
    // Analizar -> Pensar -> Tomar decisiones -> Aplicar -> (Explicar y
    // Comparar quedan cubiertos por getCurrentSnapshot() + "¿POR QUÉ?" y
    // por setCompareBypassed, respectivamente, ya existentes en la GUI).
    const auto snapshot = analysisEngine.getSnapshot();

    if (! snapshot.hasEnoughDataToJudge)
    {
        // No hay suficiente audio analizado todavía: no se debe construir
        // un plan de corrección sobre datos insuficientes (violaría
        // "nunca inventar información").
        return;
    }

    const float intervencion01 = apvts.getRawParameterValue ("intervencion")->load() / 100.0f;

    masteringChain.updatePlanFromSnapshot (snapshot, intervencion01);
    masteringChain.setEnabled (true);

    // Registro de aprendizaje: se considera "corregida" una categoria
    // cuando existe un vagon que actua directamente sobre ella (ver
    // MasteringChain::updatePlanFromSnapshot). loudnessIntegrated y
    // headroom no tienen un vagon dedicado (son consecuencia indirecta
    // de compresor+limitador), por lo que no se registran como
    // "corregidas" para no inflar artificialmente la estadistica.
    for (const auto& f : snapshot.findings)
    {
        switch (f.category)
        {
            case FindingCategory::spectralBalance:
            case FindingCategory::loudnessRange:
            case FindingCategory::truePeak:
            case FindingCategory::stereoImage:
                producerProfile.recordFindingCorrected (f.category);
                break;
            default:
                break;
        }
    }

    if (snapshot.destinoActivo != lastDestinoRecorded)
    {
        producerProfile.recordDestinoUsed (snapshot.destinoActivo);
        lastDestinoRecorded = snapshot.destinoActivo;
    }
}

void EstacionFinalAudioProcessor::pollAndRecordLearning()
{
    const auto snapshot = analysisEngine.getSnapshot();
    if (! snapshot.hasEnoughDataToJudge)
        return;

    for (const auto& f : snapshot.findings)
    {
        if (sessionDetectedCategories.insert (f.category).second)
            producerProfile.recordFindingDetected (f.category); // solo la primera vez en esta sesion
    }

    if (! snapshot.destinoActivo.isEmpty() && snapshot.destinoActivo != lastDestinoRecorded)
    {
        producerProfile.recordDestinoUsed (snapshot.destinoActivo);
        lastDestinoRecorded = snapshot.destinoActivo;
    }
}

void EstacionFinalAudioProcessor::restoreOriginal()
{
    masteringChain.setEnabled (false);
}

void EstacionFinalAudioProcessor::setCompareBypassed (bool bypassCorrectionChain)
{
    masteringChain.setEnabled (! bypassCorrectionChain);
}

juce::AudioProcessorEditor* EstacionFinalAudioProcessor::createEditor()
{
    return new EstacionFinalAudioProcessorEditor (*this);
}

void EstacionFinalAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        if (auto xml = state.createXml())
            copyXmlToBinary (*xml, destData);
    }
}

void EstacionFinalAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// Esta función es requerida por JUCE para instanciar el plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EstacionFinalAudioProcessor();
}
