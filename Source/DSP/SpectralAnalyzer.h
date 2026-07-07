#pragma once

#include <JuceHeader.h>
#include <array>

/**
    SpectralAnalyzer
    -----------------
    FFT de tamaño fijo con ventana Hann, acumulando energía promedio (RMS
    en dB, con suavizado exponencial) en bandas de tercio de octava
    aproximadas, relevantes para el diagnóstico de balance tonal:

        Sub        20   -  60 Hz
        Bass       60   - 250 Hz
        LowMid     250  - 500 Hz
        Mid        500  - 2000 Hz
        HighMid    2000 - 6000 Hz
        Air        6000 - 20000 Hz

    Estas bandas son una simplificación estándar usada en herramientas de
    análisis espectral de mastering; no representan opiniones estéticas,
    solo regiones del espectro con relevancia perceptual conocida
    (p. ej. 250-500 Hz es la zona típica de "barro" en una mezcla).
*/
class SpectralAnalyzer
{
public:
    enum Band { Sub = 0, Bass, LowMid, Mid, HighMid, Air, NumBands };

    SpectralAnalyzer() = default;

    void prepare (double sampleRate, int samplesPerBlock);
    void reset();

    void processBlock (const juce::AudioBuffer<float>& buffer);

    /** Energía suavizada de la banda, en dBFS. */
    float getBandLevelDb (Band band) const { return bandLevelsDb[band].load(); }

    /** Frecuencia central aproximada de la banda con mayor energía relativa
        respecto a un espectro plano de referencia (usada para localizar
        acumulaciones, p. ej. "270 Hz" del ejemplo de la especificación). */
    float getPeakFrequencyInBand (Band band) const;

    static constexpr float bandLowHz[NumBands]  = { 20.0f,  60.0f,  250.0f, 500.0f,  2000.0f, 6000.0f };
    static constexpr float bandHighHz[NumBands] = { 60.0f,  250.0f, 500.0f, 2000.0f, 6000.0f, 20000.0f };

private:
    static constexpr int fftOrder = 12; // 4096 puntos
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT fft { fftOrder };
    juce::dsp::WindowingFunction<float> window { static_cast<size_t> (fftSize),
                                                  juce::dsp::WindowingFunction<float>::hann };

    std::array<float, fftSize * 2> fftBuffer {};
    std::array<float, fftSize> circularBuffer {};
    int writePos = 0;
    int samplesSinceLastFFT = 0;

    double sampleRate = 44100.0;

    std::array<std::atomic<float>, NumBands> bandLevelsDb;
    std::array<std::atomic<float>, NumBands> peakFrequencyPerBand;

    // Historial de magnitud por bin (último análisis), usado para localizar
    // el pico dentro de cada banda sin recalcular la FFT en el hilo de GUI.
    std::array<float, fftSize / 2> lastMagnitudesLinear {};
    juce::SpinLock magnitudesLock;

    void performFFTAndUpdateBands();
    static constexpr float smoothingCoeff = 0.85f; // suavizado exponencial (release lento, ~decibelímetro VU)
};
