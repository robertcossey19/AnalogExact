#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class AnalogExactAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    AnalogExactAudioProcessorEditor(AnalogExactAudioProcessor&, juce::AudioProcessorValueTreeState&);
    ~AnalogExactAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    AnalogExactAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& valueTreeState;

    // Sliders - using unique_ptr to ensure proper initialization
    std::unique_ptr<juce::Slider> warmthSlider;
    std::unique_ptr<juce::Slider> bitDepthSlider;
    std::unique_ptr<juce::Slider> saturationSlider;
    std::unique_ptr<juce::Slider> noiseSlider;
    std::unique_ptr<juce::Slider> dryWetSlider;

    // Labels
    std::unique_ptr<juce::Label> warmthLabel;
    std::unique_ptr<juce::Label> bitDepthLabel;
    std::unique_ptr<juce::Label> saturationLabel;
    std::unique_ptr<juce::Label> noiseLabel;
    std::unique_ptr<juce::Label> dryWetLabel;

    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> warmthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bitDepthAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> saturationAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> noiseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;

    void setupSlider(std::unique_ptr<juce::Slider>& slider,
                     std::unique_ptr<juce::Label>& label,
                     const juce::String& labelText,
                     const juce::String& parameterID);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AnalogExactAudioProcessorEditor)
};
