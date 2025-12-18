cmake_minimum_required(VERSION 3.22)

project(ANALOGEXACT VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Suppress deprecation warnings on macOS
if(APPLE)
    add_compile_options(-Wno-deprecated-declarations)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

# Fetch JUCE 8.0.4 for macOS 15 compatibility
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# On Windows, download WebView2 SDK
if(WIN32)
    FetchContent_Declare(
        WebView2
        URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/1.0.2210.55
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(WebView2)
    
    # Tell JUCE where to find WebView2
    set(JUCE_WEBVIEW2_SDK_LOCATION "${webview2_SOURCE_DIR}/build/native" CACHE PATH "WebView2 SDK location")
endif()

# Plugin formats
if(APPLE)
    set(FORMATS VST3 AU Standalone)
else()
    set(FORMATS VST3 Standalone)
endif()

# Create the plugin
juce_add_plugin(AnalogExact
    COMPANY_NAME "AnalogExact"
    PLUGIN_MANUFACTURER_CODE Anex
    PLUGIN_CODE Aexc
    FORMATS ${FORMATS}
    PRODUCT_NAME "AnalogExact"
    
    BUNDLE_ID com.analogexact.tapemachine
    
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    
    COPY_PLUGIN_AFTER_BUILD TRUE
    
    VST3_CATEGORIES "Fx"
    AU_MAIN_TYPE kAudioUnitType_Effect
    
    NEEDS_WEBVIEW2 TRUE
)

# Generate the JuceHeader.h file
juce_generate_juce_header(AnalogExact)

# Source files
target_sources(AnalogExact
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Compile definitions
target_compile_definitions(AnalogExact
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_REPORT_APP_USAGE=0
        JUCE_USE_CURL=0
)

# Link JUCE modules
target_link_libraries(AnalogExact
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_include_directories(AnalogExact
    PRIVATE
        Source
)
