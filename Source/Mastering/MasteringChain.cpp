#include "MasteringChain.h"

void MasteringChain::prepare (double sampleRate, int samplesPerBlock, int numChannels)
{
    eq.prepare (sampleRate, samplesPerBlock, numChannels);
    compressor.prepare (sampleRate, samplesPerBlock, numChannels);
    saturator.prepare (sampleRate, samplesPerBlock, numChannels);
    imager.prepare (sampleRate, samplesPerBlock, numChannels);
    limiter.prepare (sampleRate, samplesPerBlock, numChannels);

    mixSmoothed.reset (sampleRate, mixRampSeconds);
    mixSmoothed.setCurrentAndTargetValue (targetEnabled ? 1.0f : 0.0f);
    dryBuffer.setSize (numChannels, samplesPerBlock);
}

void MasteringChain::reset()
{
    eq.reset();
    compressor.reset();
    saturator.reset();
    imager.reset();
    limiter.reset();
    mixSmoothed.setCurrentAndTargetValue (targetEnabled ? 1.0f : 0.0f);
}

void MasteringChain::setEnabled (bool shouldBeEnabled)
{
    targetEnabled = shouldBeEnabled;
    mixSmoothed.setTargetValue (shouldBeEnabled ? 1.0f : 0.0f);
}

void MasteringChain::updatePlanFromSnapshot (const MixAnalysisSnapshot& snapshot, float intervencion01)
{
    intervencion01 = juce::jlimit (0.0f, 1.0f, intervencion01);

    // 1) EQ: corrige exactamente los excesos espectrales medidos.
    eq.setCorrectionPlan (snapshot.findings, intervencion01);

    // 2) Compresión: ratio adaptado al LRA medido.
    compressor.updateFromAnalysis (snapshot.loudnessRangeLU, intervencion01);

    // 3) Saturación: cantidad fija conservadora, escalada por intervención.
    //    No depende de un finding específico — es "glue" general de bus,
    //    práctica estándar documentada de masterización, no una corrección
    //    de un problema medido.
    saturator.setIntervencion (intervencion01 * 0.6f); // limitado a 60% del rango máximo por defecto

    // 4) Imagen estéreo: solo se ensancha si hay evidencia de un problema
    //    medido (finding de categoría stereoImage). Sin ese finding, se
    //    mantiene neutro (width = 1.0), conforme a "nunca aplicar procesos
    //    sin justificación".
    float targetWidth = 1.0f;
    for (const auto& f : snapshot.findings)
    {
        if (f.category == FindingCategory::stereoImage)
        {
            // measuredValueDb aquí representa un desbalance detectado;
            // se traduce a una reducción de anchura conservadora (nunca
            // ensanchar para "corregir" un desbalance — se estrecha
            // hacia mono para mayor compatibilidad).
            targetWidth = juce::jlimit (0.5f, 1.0f, 1.0f - (f.measuredValueDb / 20.0f) * intervencion01);
        }
    }
    imager.setWidth (targetWidth);

    // 5) Limitador: techo = límite dBTP del destino activo, con 0.1 dB de
    //    margen de seguridad adicional para absorber redondeo del propio
    //    proceso de oversampling.
    limiter.setCeilingDb (snapshot.destinoLimiteTruePeakDb - 0.1f);
}

void MasteringChain::process (juce::AudioBuffer<float>& buffer)
{
    // Optimizacion: si la cadena esta completamente en 0 (dry) y no hay
    // rampa en curso, se evita todo el procesamiento — equivalente al
    // "return" temprano de la version anterior, pero ahora expresado en
    // terminos del estado de la rampa en vez de un booleano instantaneo.
    if (! targetEnabled && ! mixSmoothed.isSmoothing() && mixSmoothed.getCurrentValue() <= 0.0001f)
        return;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    dryBuffer.setSize (numChannels, numSamples, false, false, true);
    dryBuffer.makeCopyOf (buffer, true);

    eq.process (buffer);
    compressor.process (buffer);
    saturator.process (buffer);
    imager.process (buffer);
    limiter.process (buffer); // Control Final: siempre el ultimo vagon

    // Crossfade muestra a muestra entre dry (pre-cadena) y wet (post-cadena),
    // evita cualquier click al activar/desactivar "HACERLO MEJOR" o al
    // alternar COMPARAR/RESTAURAR.
    for (int i = 0; i < numSamples; ++i)
    {
        const float mix = mixSmoothed.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* wet = buffer.getWritePointer (ch);
            const auto* dry = dryBuffer.getReadPointer (ch);
            wet[i] = dry[i] * (1.0f - mix) + wet[i] * mix;
        }
    }
}
