#include "ProducerProfile.h"

juce::File ProducerProfile::getProfileFile()
{
    auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                   .getChildFile ("EstacionFinal");
    if (! dir.exists())
        dir.createDirectory();
    return dir.getChildFile ("producer_profile.json");
}

juce::String ProducerProfile::categoryToKey (FindingCategory category)
{
    switch (category)
    {
        case FindingCategory::loudnessIntegrated: return "loudnessIntegrated";
        case FindingCategory::loudnessRange:       return "loudnessRange";
        case FindingCategory::truePeak:            return "truePeak";
        case FindingCategory::spectralBalance:     return "spectralBalance";
        case FindingCategory::stereoImage:         return "stereoImage";
        case FindingCategory::dynamicRange:        return "dynamicRange";
        case FindingCategory::headroom:            return "headroom";
    }
    return "unknown";
}

bool ProducerProfile::categoryFromKey (const juce::String& key, FindingCategory& outCategory)
{
    static const std::map<juce::String, FindingCategory> lookup {
        { "loudnessIntegrated", FindingCategory::loudnessIntegrated },
        { "loudnessRange",       FindingCategory::loudnessRange },
        { "truePeak",            FindingCategory::truePeak },
        { "spectralBalance",     FindingCategory::spectralBalance },
        { "stereoImage",         FindingCategory::stereoImage },
        { "dynamicRange",        FindingCategory::dynamicRange },
        { "headroom",            FindingCategory::headroom },
    };
    auto it = lookup.find (key);
    if (it == lookup.end())
        return false;
    outCategory = it->second;
    return true;
}

ProducerProfile::ProducerProfile()
{
    loadFromDisk();
}

void ProducerProfile::loadFromDisk()
{
    const juce::ScopedLock lock (dataLock);

    auto file = getProfileFile();
    if (! file.existsAsFile())
        return; // perfil nuevo: se queda con los valores por defecto (todo en cero)

    auto parsed = juce::JSON::parse (file);
    if (! parsed.isObject())
        return; // archivo corrupto o vacio: no se inventa un perfil falso, se empieza de cero

    totalSessions = static_cast<int> (parsed.getProperty ("totalSessions", 0));
    totalWorkSeconds = static_cast<double> (parsed.getProperty ("totalWorkSeconds", 0.0));

    if (auto* statsObj = parsed.getProperty ("stats", {}).getDynamicObject())
    {
        for (const auto& prop : statsObj->getProperties())
        {
            FindingCategory category;
            if (! categoryFromKey (prop.name.toString(), category))
                continue;

            if (auto* entry = prop.value.getDynamicObject())
            {
                CategoryStats cs;
                cs.timesFound = static_cast<int> (entry->getProperty ("timesFound"));
                cs.timesCorrected = static_cast<int> (entry->getProperty ("timesCorrected"));
                stats[category] = cs;
            }
        }
    }

    if (auto* destinosObj = parsed.getProperty ("destinos", {}).getDynamicObject())
    {
        for (const auto& prop : destinosObj->getProperties())
            destinoUsageCount[prop.name.toString()] = static_cast<int> (prop.value);
    }
}

void ProducerProfile::saveToDisk()
{
    const juce::ScopedLock lock (dataLock);

    // Lock inter-proceso: evita corrupcion si dos instancias del plugin
    // (dos pistas del mismo proyecto) intentan guardar al mismo tiempo.
    juce::InterProcessLock interProcessLock ("EstacionFinal_ProducerProfile");
    const juce::InterProcessLock::ScopedLockType scopedLock (interProcessLock);

    auto* root = new juce::DynamicObject();
    root->setProperty ("totalSessions", totalSessions);
    root->setProperty ("totalWorkSeconds", totalWorkSeconds);

    auto* statsObj = new juce::DynamicObject();
    for (const auto& [category, cs] : stats)
    {
        auto* entry = new juce::DynamicObject();
        entry->setProperty ("timesFound", cs.timesFound);
        entry->setProperty ("timesCorrected", cs.timesCorrected);
        statsObj->setProperty (categoryToKey (category), juce::var (entry));
    }
    root->setProperty ("stats", juce::var (statsObj));

    auto* destinosObj = new juce::DynamicObject();
    for (const auto& [nombre, count] : destinoUsageCount)
        destinosObj->setProperty (nombre, count);
    root->setProperty ("destinos", juce::var (destinosObj));

    const juce::String json = juce::JSON::toString (juce::var (root));

    // Escritura atomica: a archivo temporal y luego renombrar, para evitar
    // dejar el perfil corrupto si el proceso se interrumpe a mitad de la
    // escritura (p. ej. el host se cierra de forma abrupta).
    auto targetFile = getProfileFile();
    auto tempFile = targetFile.getSiblingFile (targetFile.getFileNameWithoutExtension() + "_tmp.json");

    if (tempFile.replaceWithText (json))
        tempFile.moveFileTo (targetFile);
}

void ProducerProfile::recordSessionStart()
{
    const juce::ScopedLock lock (dataLock);
    currentSessionStartMs = juce::Time::currentTimeMillis();
    ++totalSessions;
}

void ProducerProfile::recordSessionEnd()
{
    const juce::ScopedLock lock (dataLock);
    if (currentSessionStartMs == 0)
        return;

    const auto elapsedMs = juce::Time::currentTimeMillis() - currentSessionStartMs;
    totalWorkSeconds += static_cast<double> (elapsedMs) / 1000.0;
    currentSessionStartMs = 0;

    saveToDisk();
}

void ProducerProfile::recordFindingDetected (FindingCategory category)
{
    const juce::ScopedLock lock (dataLock);
    stats[category].timesFound += 1;
}

void ProducerProfile::recordFindingCorrected (FindingCategory category)
{
    const juce::ScopedLock lock (dataLock);
    stats[category].timesCorrected += 1;
}

void ProducerProfile::recordDestinoUsed (const juce::String& destinoNombre)
{
    const juce::ScopedLock lock (dataLock);
    destinoUsageCount[destinoNombre] += 1;
}

ProducerProfile::CategoryStats ProducerProfile::getStats (FindingCategory category) const
{
    const juce::ScopedLock lock (dataLock);
    auto it = stats.find (category);
    return it != stats.end() ? it->second : CategoryStats {};
}

juce::String ProducerProfile::getMostFrequentDestino() const
{
    const juce::ScopedLock lock (dataLock);
    if (destinoUsageCount.empty())
        return {};

    auto best = std::max_element (destinoUsageCount.begin(), destinoUsageCount.end(),
        [] (const auto& a, const auto& b) { return a.second < b.second; });
    return best->first;
}

juce::String ProducerProfile::getMostRepeatedErrorDescription() const
{
    const juce::ScopedLock lock (dataLock);

    // Regla explicita para evitar conclusiones prematuras: se requieren al
    // menos 3 sesiones registradas antes de emitir una recomendacion
    // basada en patrones (evita que un solo proyecto atipico se interprete
    // como un "error repetitivo" del productor).
    if (totalSessions < 3 || stats.empty())
        return {};

    auto best = std::max_element (stats.begin(), stats.end(),
        [] (const auto& a, const auto& b) { return a.second.timesFound < b.second.timesFound; });

    if (best->second.timesFound < 2)
        return {}; // ni siquiera el mas frecuente se repite lo suficiente

    juce::String categoryName;
    switch (best->first)
    {
        case FindingCategory::loudnessIntegrated: categoryName = "loudness integrado fuera del objetivo del destino"; break;
        case FindingCategory::loudnessRange:       categoryName = "rango de loudness (LRA) atipico"; break;
        case FindingCategory::truePeak:            categoryName = "True Peak por encima del limite"; break;
        case FindingCategory::spectralBalance:     categoryName = "acumulaciones de energia espectral"; break;
        case FindingCategory::stereoImage:         categoryName = "incoherencias de imagen estereo"; break;
        case FindingCategory::dynamicRange:        categoryName = "rango dinamico atipico"; break;
        case FindingCategory::headroom:            categoryName = "headroom insuficiente"; break;
    }

    return "En " + juce::String (best->second.timesFound) + " de " + juce::String (totalSessions)
         + " sesiones registradas se detecto: " + categoryName + ".";
}
