#include "MixAnalysisEngine.h"

const std::vector<MixAnalysisEngine::DestinoTarget>& MixAnalysisEngine::allDestinos()
{
    // Objetivos de referencia pública (documentación de normalización de
    // loudness de cada plataforma / estándares de broadcast y masterización
    // física). No son valores inventados: son los targets ampliamente
    // documentados en la industria a fecha de esta implementación y deben
    // revisarse periódicamente porque las plataformas los actualizan.
    static const std::vector<DestinoTarget> destinos {
        { "Spotify",        -14.0f, -1.0f },
        { "Apple Music",    -16.0f, -1.0f },
        { "YouTube",        -14.0f, -1.0f },
        { "TikTok",         -14.0f, -1.0f },
        { "Club",            -8.0f, -0.3f },
        { "Radio",          -16.0f, -1.0f },
        { "CD",              -9.0f, -0.3f },
        { "Vinilo",          -12.0f, -3.0f },
        { "Archivo Maestro", -10.0f, -1.0f },
    };
    return destinos;
}

void MixAnalysisEngine::setDestino (const juce::String& destinoName)
{
    for (const auto& d : allDestinos())
    {
        if (d.nombre == destinoName)
        {
            const juce::SpinLock::ScopedLockType lock (snapshotLock);
            currentTarget = d;
            return;
        }
    }
    jassertfalse; // destino desconocido — no se debe llegar aquí desde la GUI
}

void MixAnalysisEngine::prepare (double newSampleRate, int samplesPerBlock, int numChannels)
{
    sampleRate = newSampleRate;
    samplesProcessed = 0;

    loudness.prepare (sampleRate, numChannels);
    truePeak.prepare (sampleRate, samplesPerBlock, numChannels);
    spectral.prepare (sampleRate, samplesPerBlock);
}

void MixAnalysisEngine::reset()
{
    loudness.reset();
    truePeak.reset();
    spectral.reset();
    samplesProcessed = 0;
}

void MixAnalysisEngine::processBlock (const juce::AudioBuffer<float>& buffer)
{
    loudness.processBlock (buffer);
    truePeak.processBlock (buffer);
    spectral.processBlock (buffer);

    samplesProcessed += buffer.getNumSamples();
}

juce::String MixAnalysisEngine::buildExplanation (const MixFinding& finding)
{
    // Generación determinista de texto a partir de valores medidos.
    // Sigue literalmente el patrón exigido por la especificación:
    // "Reducí 2.1 dB en 270 Hz porque existía acumulación de energía
    //  que reducía la claridad de la voz."
    //
    // IMPORTANTE: en esta fase (motor de análisis) el plugin todavía no
    // "reduce" nada — solo diagnostica. El verbo se ajusta a "Detecté" /
    // "Existe" hasta que el módulo de mastering (Fase 2) ejecute la
    // corrección real; en ese momento este mismo finding se reutilizará
    // para redactar el texto en pasado ("Reducí...").
    switch (finding.category)
    {
        case FindingCategory::spectralBalance:
        {
            juce::String zona;
            if      (finding.frequencyHz < 60.0f)    zona = "graves profundos";
            else if (finding.frequencyHz < 250.0f)   zona = "graves";
            else if (finding.frequencyHz < 500.0f)   zona = "medios-bajos (zona de \"barro\")";
            else if (finding.frequencyHz < 2000.0f)  zona = "medios (presencia vocal)";
            else if (finding.frequencyHz < 6000.0f)  zona = "medios-altos";
            else                                       zona = "brillo/aire";

            return "Detecte acumulacion de energia de " + juce::String (finding.measuredValueDb, 1)
                 + " dB por encima de la referencia en " + juce::String (finding.frequencyHz, 0)
                 + " Hz (zona de " + zona + "). Esto puede reducir la claridad de otros elementos"
                 + " en esa misma region del espectro.";
        }

        case FindingCategory::truePeak:
            return "El True Peak medido es " + juce::String (finding.measuredValueDb, 2)
                 + " dBTP, por encima del limite recomendado para el destino activo."
                 + " Esto puede causar distorsion por recorte inter-muestra al pasar por"
                 + " conversores digital-analogico o codecs con perdida.";

        case FindingCategory::loudnessIntegrated:
            return "El loudness integrado medido es " + juce::String (finding.measuredValueDb, 1)
                 + " LU respecto al objetivo del destino activo. Publicar con esta diferencia"
                 + " puede provocar que la plataforma normalice el volumen de forma no deseada.";

        case FindingCategory::loudnessRange:
            return "El rango de loudness (LRA) medido es " + juce::String (finding.measuredValueDb, 1)
                 + " LU. Un LRA muy bajo puede indicar sobre-compresion; uno muy alto puede"
                 + " dificultar la escucha en entornos con ruido de fondo.";

        case FindingCategory::stereoImage:
            return "Se detecto una posible incoherencia de fase o balance estereo de "
                 + juce::String (finding.measuredValueDb, 1) + " dB entre canales.";

        case FindingCategory::dynamicRange:
            return "El rango dinamico medido difiere en " + juce::String (finding.measuredValueDb, 1)
                 + " dB respecto al rango tipico recomendado para el destino activo.";

        case FindingCategory::headroom:
            return "El headroom disponible antes del limite del destino es de "
                 + juce::String (finding.measuredValueDb, 1) + " dB.";
    }

    return "Hallazgo sin descripcion disponible.";
}

void MixAnalysisEngine::evaluateLoudness (MixAnalysisSnapshot& snap) const
{
    const float diff = snap.integratedLUFS - snap.destinoObjetivoLUFS;

    if (std::abs (diff) >= 1.0f)
    {
        MixFinding f;
        f.category = FindingCategory::loudnessIntegrated;
        f.severity = std::abs (diff) >= 3.0f ? FindingSeverity::critical : FindingSeverity::suggestion;
        f.measuredValueDb = diff;
        f.explanation = buildExplanation (f);
        snap.findings.push_back (f);
    }

    if (snap.loudnessRangeLU < 2.0f || snap.loudnessRangeLU > 15.0f)
    {
        MixFinding f;
        f.category = FindingCategory::loudnessRange;
        f.severity = FindingSeverity::suggestion;
        f.measuredValueDb = snap.loudnessRangeLU;
        f.explanation = buildExplanation (f);
        snap.findings.push_back (f);
    }
}

void MixAnalysisEngine::evaluateTruePeak (MixAnalysisSnapshot& snap) const
{
    if (snap.truePeakDb > snap.destinoLimiteTruePeakDb)
    {
        MixFinding f;
        f.category = FindingCategory::truePeak;
        f.severity = FindingSeverity::critical;
        f.measuredValueDb = snap.truePeakDb;
        f.explanation = buildExplanation (f);
        snap.findings.push_back (f);
    }
}

void MixAnalysisEngine::evaluateSpectralBalance (MixAnalysisSnapshot& snap) const
{
    // Referencia plana simplificada: se compara cada banda contra el
    // promedio de todas las bandas. Una desviacion >= 3 dB por encima del
    // promedio se reporta como posible acumulacion (umbral documentado
    // en herramientas de analisis espectral de mastering de referencia).
    std::array<float, SpectralAnalyzer::NumBands> levels {};
    for (int b = 0; b < SpectralAnalyzer::NumBands; ++b)
        levels[static_cast<size_t> (b)] = spectral.getBandLevelDb (static_cast<SpectralAnalyzer::Band> (b));

    float sum = 0.0f;
    for (auto v : levels) sum += v;
    const float average = sum / static_cast<float> (SpectralAnalyzer::NumBands);

    for (int b = 0; b < SpectralAnalyzer::NumBands; ++b)
    {
        const float excess = levels[static_cast<size_t> (b)] - average;
        if (excess >= 3.0f)
        {
            MixFinding f;
            f.category = FindingCategory::spectralBalance;
            f.severity = excess >= 6.0f ? FindingSeverity::critical : FindingSeverity::suggestion;
            f.frequencyHz = spectral.getPeakFrequencyInBand (static_cast<SpectralAnalyzer::Band> (b));
            f.measuredValueDb = excess;
            f.explanation = buildExplanation (f);
            snap.findings.push_back (f);
        }
    }
}

SemaphoreState MixAnalysisEngine::computeOverallSemaphore (const std::vector<MixFinding>& findings)
{
    bool anyCritical = false;
    bool anySuggestion = false;

    for (const auto& f : findings)
    {
        if (f.severity == FindingSeverity::critical) anyCritical = true;
        if (f.severity == FindingSeverity::suggestion) anySuggestion = true;
    }

    if (anyCritical) return SemaphoreState::rojo;
    if (anySuggestion) return SemaphoreState::amarillo;
    return SemaphoreState::verde;
}

MixAnalysisSnapshot MixAnalysisEngine::getSnapshot() const
{
    MixAnalysisSnapshot snap;

    {
        const juce::SpinLock::ScopedLockType lock (snapshotLock);
        snap.destinoActivo = currentTarget.nombre;
        snap.destinoObjetivoLUFS = currentTarget.objetivoLUFS;
        snap.destinoLimiteTruePeakDb = currentTarget.limiteTruePeakDb;
    }

    snap.momentaryLUFS   = loudness.getMomentaryLUFS();
    snap.shortTermLUFS   = loudness.getShortTermLUFS();
    snap.integratedLUFS  = loudness.getIntegratedLUFS();
    snap.loudnessRangeLU = loudness.getLoudnessRangeLU();
    snap.truePeakDb      = truePeak.getTruePeakDb();
    snap.headroomDb      = snap.destinoLimiteTruePeakDb - snap.truePeakDb;

    // Se requieren al menos ~3 segundos de audio analizado antes de emitir
    // juicios (evita falsos "rojo" durante silencio inicial o transitorios).
    snap.hasEnoughDataToJudge = samplesProcessed > static_cast<juce::int64> (sampleRate * 3.0);

    if (snap.hasEnoughDataToJudge)
    {
        evaluateLoudness (snap);
        evaluateTruePeak (snap);
        evaluateSpectralBalance (snap);
        snap.semaphore = computeOverallSemaphore (snap.findings);
    }
    else
    {
        snap.semaphore = SemaphoreState::amarillo;
    }

    return snap;
}
