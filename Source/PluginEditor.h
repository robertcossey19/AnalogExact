#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

//==============================================================================
class AnalogExactAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer
{
public:
    AnalogExactAudioProcessorEditor (AnalogExactAudioProcessor&);
    ~AnalogExactAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    AnalogExactAudioProcessor& audioProcessor;
    
    // WebView component
    std::unique_ptr<juce::WebBrowserComponent> webView;
    
    // Flag to track if webview is ready
    std::atomic<bool> webViewReady { false };
    
    // Track last sent values to avoid redundant updates
    float lastInputGain = 0.0f;
    float lastOutputGain = 0.0f;
    float lastBias = 0.0f;
    int lastMonitor = -1;
    int lastSpeed = -1;
    int lastFlux = -1;
    int lastEQ = -1;
    int lastTapeType = -1;
    bool lastTransformer = true;
    int lastHeadblock = -1;
    bool lastAutoCal = true;
    
    // Generate the HTML UI
    juce::String generateHTML();
    
    // Send parameter updates to WebView (thread-safe)
    void updateWebViewParameters();
    
    // JavaScript bridge functions (thread-safe)
    void safeEvaluateJS (const juce::String& script);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogExactAudioProcessorEditor)
};
