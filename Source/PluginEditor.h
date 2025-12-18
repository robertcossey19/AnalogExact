#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class AnalogExactAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer
{
public:
    AnalogExactAudioProcessorEditor (AnalogExactAudioProcessor&, 
                                     juce::AudioProcessorValueTreeState&);
    ~AnalogExactAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    AnalogExactAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& valueTreeState;
    
    // WebView component with custom handler
    class CustomWebView : public juce::WebBrowserComponent
    {
    public:
        CustomWebView (AnalogExactAudioProcessorEditor& ed) : editor (ed) {}
        
        bool pageAboutToLoad (const juce::String& newURL) override
        {
            // Intercept custom protocol for parameter changes
            if (newURL.startsWith ("juceplugin://"))
            {
                // Use message thread to ensure thread safety
                juce::MessageManager::callAsync ([this, newURL]() {
                    editor.handleWebViewMessage (newURL);
                });
                return false; // Don't actually navigate
            }
            return true;
        }
        
        void pageFinishedLoading (const juce::String& url) override
        {
            juce::ignoreUnused (url);
            // Sync parameters after page loads
            juce::MessageManager::callAsync ([this]() {
                editor.updateWebViewParameters();
            });
        }
        
    private:
        AnalogExactAudioProcessorEditor& editor;
    };
    
    std::unique_ptr<CustomWebView> webView;
    bool webViewReady = false;
    bool webViewAvailable = false;
    
    // Fallback UI for when WebView isn't available
    juce::Label fallbackLabel;
    juce::TextButton reloadButton {"Reload UI"};
    
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
    
    // Send parameter updates to WebView
    void updateWebViewParameters();
    
    // JavaScript bridge functions
    void executeJS (const juce::String& script);
    void setParameterInJS (const juce::String& paramName, float value);
    void setParameterInJS (const juce::String& paramName, int value);
    void setParameterInJS (const juce::String& paramName, bool value);
    
    // Handle messages from WebView
    void handleWebViewMessage (const juce::String& url);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogExactAudioProcessorEditor)
};
