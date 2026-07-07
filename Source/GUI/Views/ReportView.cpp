#include "ReportView.h"
#include "../../PluginProcessor.h"

ReportView::ReportView (EstacionFinalAudioProcessor& processorToUse)
    : processor (processorToUse)
{
    headerLabel.setFont (juce::Font (juce::FontOptions (18.0f, juce::Font::bold)));
    headerLabel.setColour (juce::Label::textColourId, TrackArchitectLookAndFeel::blanco);
    addAndMakeVisible (headerLabel);

    reportText.setMultiLine (true);
    reportText.setReadOnly (true);
    reportText.setScrollbarsShown (true);
    reportText.setCaretVisible (false);
    reportText.setColour (juce::TextEditor::backgroundColourId, TrackArchitectLookAndFeel::grisGrafito);
    reportText.setColour (juce::TextEditor::textColourId, TrackArchitectLookAndFeel::blanco);
    reportText.setFont (juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), 12.5f, juce::Font::plain)));
    addAndMakeVisible (reportText);

    exportButton.onClick = [this]
    {
        auto file = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                        .getNonexistentChildFile ("EstacionFinal_Reporte", ".txt");
        file.replaceWithText (reportText.getText());

        juce::AlertWindow::showMessageBoxAsync (
            juce::MessageBoxIconType::InfoIcon, "Estacion Final",
            "Reporte exportado a:\n" + file.getFullPathName());
    };
    addAndMakeVisible (exportButton);
}

juce::String ReportView::buildReportText (const MixAnalysisSnapshot& snapshot, const MasteringChain& chain) const
{
    juce::String text;
    text << "ESTACION FINAL - Reporte de masterizacion\n";
    text << juce::Time::getCurrentTime().toString (true, true) << "\n";
    text << "==================================================\n\n";

    text << "Destino activo: " << snapshot.destinoActivo << "\n";
    text << "Objetivo LUFS: " << juce::String (snapshot.destinoObjetivoLUFS, 1) << " LUFS\n";
    text << "Limite True Peak: " << juce::String (snapshot.destinoLimiteTruePeakDb, 1) << " dBTP\n\n";

    text << "--- Mediciones ---\n";
    text << "LUFS Integrado: " << juce::String (snapshot.integratedLUFS, 2) << " LUFS\n";
    text << "Loudness Range: " << juce::String (snapshot.loudnessRangeLU, 2) << " LU\n";
    text << "True Peak: " << juce::String (snapshot.truePeakDb, 2) << " dBTP\n";
    text << "Headroom: " << juce::String (snapshot.headroomDb, 2) << " dB\n\n";

    text << "--- Cadena de mastering ---\n";
    text << "Estado: " << (chain.isEnabled() ? "ACTIVA" : "EN ESPERA") << "\n";
    text << "Ratio de compresion: " << juce::String (chain.getCompressorRatio(), 2) << ":1\n";
    text << "Reduccion de ganancia (compresor): " << juce::String (chain.getCompressorGainReductionDb(), 2) << " dB\n";
    text << "Reduccion de ganancia (limitador): " << juce::String (chain.getLimiterGainReductionDb(), 2) << " dB\n\n";

    text << "--- Hallazgos (" << snapshot.findings.size() << ") ---\n";
    if (snapshot.findings.empty())
    {
        text << "Sin hallazgos relevantes.\n";
    }
    else
    {
        int i = 1;
        for (const auto& f : snapshot.findings)
        {
            text << "[" << i << "] " << f.explanation << "\n";
            ++i;
        }
    }

    const auto& profile = processor.getProducerProfile();
    text << "\n--- Perfil de aprendizaje (acumulado entre sesiones) ---\n";
    text << "Sesiones registradas: " << profile.getTotalSessions() << "\n";
    text << "Tiempo total de trabajo: " << juce::String (profile.getTotalWorkSeconds() / 60.0, 1) << " minutos\n";

    const auto recomendacion = profile.getMostRepeatedErrorDescription();
    text << "Recomendacion personalizada: "
         << (recomendacion.isEmpty()
                ? "Aun no hay suficientes sesiones para una recomendacion confiable (se requieren al menos 3)."
                : recomendacion)
         << "\n";

    return text;
}

void ReportView::refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain& chain)
{
    reportText.setText (buildReportText (snapshot, chain), juce::dontSendNotification);
}

void ReportView::paint (juce::Graphics& g)
{
    g.fillAll (TrackArchitectLookAndFeel::negroCarbon);
}

void ReportView::resized()
{
    auto area = getLocalBounds().reduced (16);
    headerLabel.setBounds (area.removeFromTop (28));
    area.removeFromTop (8);
    exportButton.setBounds (area.removeFromBottom (36));
    area.removeFromBottom (8);
    reportText.setBounds (area);
}
