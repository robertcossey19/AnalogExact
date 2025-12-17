cmake_minimum_required(VERSION 3.15)

project(ANALOGEXACT VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE
add_subdirectory(JUCE)

# Plugin formats to build
set(FORMATS VST3 AU Standalone)

# Create the plugin target
juce_add_plugin(AnalogExact
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Yoco
    PLUGIN_CODE Anex
    FORMATS ${FORMATS}
    PRODUCT_NAME "AnalogExact"
    
    # Bundle ID for macOS
    BUNDLE_ID com.yourcompany.analogexact
    
    # Plugin characteristics
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    
    # Copy plugin after build
    COPY_PLUGIN_AFTER_BUILD TRUE
    
    # VST3 settings
    VST3_CATEGORIES "Fx"
    
    # AU settings  
    AU_MAIN_TYPE kAudioUnitType_Effect
    
    # Plugin description
    DESCRIPTION "Analog Tape Machine Emulation"
    PLUGIN_CODE Anex
)

# Source files
target_sources(AnalogExact
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# JUCE modules
target_compile_definitions(AnalogExact
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(AnalogExact
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Include directories
target_include_directories(AnalogExact
    PRIVATE
        Source
)
