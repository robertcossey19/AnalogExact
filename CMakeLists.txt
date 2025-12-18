cmake_minimum_required(VERSION 3.22)

project(AnalogExact VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE subdirectory
add_subdirectory(JUCE)

# macOS Universal Binary support (x86_64 and arm64 for Rosetta)
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for macOS" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version" FORCE)
endif()

# Windows WebView2 support
if(WIN32)
    # Set WebView2 package location hint
    set(JUCE_WEBVIEW2_PACKAGE_LOCATION "$ENV{USERPROFILE}/.nuget/packages" CACHE PATH "WebView2 package location")
endif()

juce_add_plugin(AnalogExact
    COMPANY_NAME "YourCompany"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Rcsy
    PLUGIN_CODE Anex
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "AnalogExact"
    VST3_CATEGORIES "Fx" "Dynamics"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
)

# Source files
target_sources(AnalogExact PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

# Compile definitions
target_compile_definitions(AnalogExact PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

# Link JUCE modules
target_link_libraries(AnalogExact PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
)

# Public compile definitions
target_link_libraries(AnalogExact PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
