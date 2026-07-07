#pragma once

#include <JuceHeader.h>
#include "../../Analysis/MixDiagnosis.h"
#include "../../Mastering/MasteringChain.h"
#include "../TrackArchitectLookAndFeel.h"

/**
    SectionView
    ------------
    Interfaz común de las 8 secciones del "Panel Central" definidas en la
    especificación: Análisis, Ruta, Destino, Inspección, Master, Reporte,
    Configuración, Acerca de.

    Cada vista recibe el snapshot de análisis actualizado periódicamente
    (mismo Timer de 30 Hz que la Pantalla Principal) y una referencia
    de solo lectura a la cadena de mastering para leer su estado interno
    (reducción de ganancia, ratio, correlación estéreo, bandas de EQ
    activas). Ninguna vista escribe directamente sobre el DSP: toda acción
    del usuario pasa por el AudioProcessor (mismo patrón que la Pantalla
    Principal con requestAutoMaster/restoreOriginal).
*/
class SectionView : public juce::Component
{
public:
    ~SectionView() override = default;

    /** Se llama ~30 veces por segundo desde MainPanel::timerCallback. */
    virtual void refresh (const MixAnalysisSnapshot& snapshot, const MasteringChain& chain) = 0;

    virtual juce::String getSectionTitle() const = 0;
};
