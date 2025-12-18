#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class AnalogExactAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    AnalogExactAudioProcessorEditor (AnalogExactAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~AnalogExactAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    AnalogExactAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& valueTreeState;

    juce::Label loadingLabel;
    std::unique_ptr<juce::WebBrowserComponent> web;

    void createWebUI();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogExactAudioProcessorEditor)
};
