cmake_minimum_required(VERSION 3.15)

project(ANALOGEXACT VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

set(FORMATS VST3 AU Standalone)

juce_add_plugin(AnalogExact
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Yoco
    PLUGIN_CODE Anex
    FORMATS ${FORMATS}
    PRODUCT_NAME "AnalogExact"
    BUNDLE_ID com.yourcompany.analogexact

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    # CI-safe
    COPY_PLUGIN_AFTER_BUILD FALSE

    VST3_CATEGORIES "Fx"
    AU_MAIN_TYPE kAudioUnitType_Effect
    DESCRIPTION "Analog Tape Machine Emulation"

    # JUCE 8 WebView support (Windows needs this; safe elsewhere)
    NEEDS_WEBVIEW2 TRUE
)

# Needed if you use <JuceHeader.h>
juce_generate_juce_header(AnalogExact)

# Embed your web UI file into BinaryData
juce_add_binary_data(AnalogExactWebUI
    SOURCES
        WebUI/index.html
)

target_sources(AnalogExact
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_compile_definitions(AnalogExact
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1

        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(AnalogExact
    PRIVATE
        AnalogExactWebUI
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_include_directories(AnalogExact PRIVATE Source)
