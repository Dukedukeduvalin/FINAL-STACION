cmake_minimum_required(VERSION 3.22)

project(EstacionFinal VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# JUCE
# ---------------------------------------------------------------------------
# Se espera que JUCE esté disponible como submódulo en libs/JUCE, o instalado
# globalmente y localizable vía find_package(JUCE CONFIG REQUIRED).
#
#   git submodule add https://github.com/juce-framework/JUCE.git libs/JUCE
#
if(EXISTS "${CMAKE_SOURCE_DIR}/libs/JUCE/CMakeLists.txt")
    add_subdirectory(libs/JUCE)
else()
    find_package(JUCE CONFIG REQUIRED)
endif()

# ---------------------------------------------------------------------------
# Plugin target
# ---------------------------------------------------------------------------
juce_add_plugin(EstacionFinal
    COMPANY_NAME                "EduBeatsMusic"
    BUNDLE_ID                   "com.edubeatsmusic.estacionfinal"
    PLUGIN_MANUFACTURER_CODE    EBNP
    PLUGIN_CODE                 EFin
    FORMATS                     VST3
    PRODUCT_NAME                "Estacion Final"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     TRUE
    VST3_CATEGORIES             Fx Mastering
)

juce_generate_juce_header(EstacionFinal)

# ---------------------------------------------------------------------------
# Fuentes
# ---------------------------------------------------------------------------
target_sources(EstacionFinal
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h

        Source/DSP/LoudnessAnalyzer.cpp
        Source/DSP/LoudnessAnalyzer.h
        Source/DSP/TruePeakDetector.cpp
        Source/DSP/TruePeakDetector.h
        Source/DSP/SpectralAnalyzer.cpp
        Source/DSP/SpectralAnalyzer.h

        Source/Analysis/MixAnalysisEngine.cpp
        Source/Analysis/MixAnalysisEngine.h
        Source/Analysis/MixDiagnosis.h

        Source/Mastering/MasteringChain.cpp
        Source/Mastering/MasteringChain.h
        Source/Mastering/DynamicEQ.cpp
        Source/Mastering/DynamicEQ.h
        Source/Mastering/MasteringCompressor.cpp
        Source/Mastering/MasteringCompressor.h
        Source/Mastering/HarmonicSaturator.cpp
        Source/Mastering/HarmonicSaturator.h
        Source/Mastering/StereoImager.cpp
        Source/Mastering/StereoImager.h
        Source/Mastering/TruePeakLimiter.cpp
        Source/Mastering/TruePeakLimiter.h

        Source/Learning/ProducerProfile.cpp
        Source/Learning/ProducerProfile.h

        Source/GUI/TrackArchitectLookAndFeel.cpp
        Source/GUI/TrackArchitectLookAndFeel.h
        Source/GUI/NavigationPanel.cpp
        Source/GUI/NavigationPanel.h
        Source/GUI/MainPanel.cpp
        Source/GUI/MainPanel.h

        Source/GUI/Views/SectionView.h
        Source/GUI/Views/AnalysisView.cpp
        Source/GUI/Views/AnalysisView.h
        Source/GUI/Views/RouteView.cpp
        Source/GUI/Views/RouteView.h
        Source/GUI/Views/DestinoView.cpp
        Source/GUI/Views/DestinoView.h
        Source/GUI/Views/InspectionView.cpp
        Source/GUI/Views/InspectionView.h
        Source/GUI/Views/MasterView.cpp
        Source/GUI/Views/MasterView.h
        Source/GUI/Views/ReportView.cpp
        Source/GUI/Views/ReportView.h
        Source/GUI/Views/ConfigView.cpp
        Source/GUI/Views/ConfigView.h
        Source/GUI/Views/AboutView.cpp
        Source/GUI/Views/AboutView.h
)

target_compile_definitions(EstacionFinal
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(EstacionFinal
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# NOTA (solo Linux, no aplica al target Windows de este proyecto): JUCE
# encuentra freetype/fontconfig via pkg-config pero no enlaza harfbuzz de
# forma automatica en algunas distribuciones, causando un error de enlazado
# (undefined reference a hb_buffer_destroy). Se enlaza aqui explicitamente
# solo para permitir compilar y validar el proyecto en este entorno Linux
# de desarrollo/CI; en Windows (target real del proyecto) esto no es
# necesario porque JUCE usa DirectWrite/GDI para fuentes, no harfbuzz.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(HARFBUZZ REQUIRED harfbuzz)
    target_link_libraries(EstacionFinal PRIVATE ${HARFBUZZ_LIBRARIES})
    target_include_directories(EstacionFinal PRIVATE ${HARFBUZZ_INCLUDE_DIRS})
endif()
