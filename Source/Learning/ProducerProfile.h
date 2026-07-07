#pragma once

#include <JuceHeader.h>
#include "../Analysis/MixDiagnosis.h"

/**
    ProducerProfile
    -----------------
    Perfil de aprendizaje del productor, persistente entre sesiones
    (especificación, sección MOTOR DE APRENDIZAJE):

        - Errores encontrados / corregidos
        - Plugins/procesos utilizados (vagones activados en cada sesión)
        - Tiempo de trabajo
        - Estadísticas acumuladas

    Almacenamiento: un único archivo JSON en el directorio de datos de
    aplicación del usuario (no requiere motor de base de datos: el volumen
    de datos de un perfil de un productor es trivial y JSON ya viene
    integrado en JUCE sin dependencias adicionales).

    Concurrencia: si el usuario abre el plugin en varias pistas del mismo
    proyecto, cada instancia mantiene su propia copia en memoria y
    fusiona (merge aditivo) sus cambios al guardar, usando un
    juce::InterProcessLock para serializar la escritura del archivo.
*/
class ProducerProfile
{
public:
    struct CategoryStats
    {
        int timesFound = 0;
        int timesCorrected = 0;
    };

    ProducerProfile();

    /** Carga el perfil desde disco. Si no existe, se crea uno nuevo vacío. */
    void loadFromDisk();

    /** Persiste el perfil a disco de forma atómica (escribe a archivo
        temporal y renombra), protegido por un lock inter-proceso. */
    void saveToDisk();

    // --- Registro de eventos de la sesión actual ---------------------------
    void recordSessionStart();
    void recordSessionEnd();

    /** Registra que se detectó un hallazgo (llamar una vez por finding único
        detectado en la sesión, no en cada tick del análisis). */
    void recordFindingDetected (FindingCategory category);

    /** Registra que la cadena de mastering corrigió activamente una categoría
        (llamar cuando MasteringChain aplica una corrección real de esa
        categoría, no solo cuando se diagnostica). */
    void recordFindingCorrected (FindingCategory category);

    void recordDestinoUsed (const juce::String& destinoNombre);

    // --- Consultas para la GUI / motor de recomendaciones -------------------
    int getTotalSessions() const { return totalSessions; }
    double getTotalWorkSeconds() const { return totalWorkSeconds; }
    CategoryStats getStats (FindingCategory category) const;
    juce::String getMostFrequentDestino() const;

    /** Categoría de error más repetida a lo largo de todas las sesiones
        (para el motor de recomendaciones personalizadas). Vacío si no hay
        datos suficientes (menos de 3 sesiones registradas, para evitar
        conclusiones prematuras). */
    juce::String getMostRepeatedErrorDescription() const;

    static juce::File getProfileFile();

private:
    static juce::String categoryToKey (FindingCategory category);
    static bool categoryFromKey (const juce::String& key, FindingCategory& outCategory);

    int totalSessions = 0;
    double totalWorkSeconds = 0.0;
    juce::int64 currentSessionStartMs = 0;

    std::map<FindingCategory, CategoryStats> stats;
    std::map<juce::String, int> destinoUsageCount;

    juce::CriticalSection dataLock;
};
