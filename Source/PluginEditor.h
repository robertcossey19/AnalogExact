#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class VUMeter : public juce::Component, public juce::Timer
{
public:
    VUMeter();
    void paint (juce::Graphics& g) override;
    void setLevel (float level);
    void timerCallback() override;
    
private:
    float currentLevel = 0.0f;
    float displayLevel = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VUMeter)
};

//==============================================================================
class RotaryKnob : public juce::Component
{
public:
    RotaryKnob (const juce::String& labelText);
    void paint (juce::Graphics& g) override;
    void resized() override;
    
    void setValue (float value); // 0.0 to 1.0
    float getValue() const { return value; }
    
    std::function<void(float)> onValueChange;
    
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    
private:
    juce::String label;
    float value = 0.5f;
    juce::Point<int> dragStart;
    float dragStartValue = 0.5f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RotaryKnob)
};

//==============================================================================
class AnalogExactAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        public juce::Timer
{
public:
    AnalogExactAudioProcessorEditor (AnalogExactAudioProcessor&, 
                                     juce::AudioProcessorValueTreeState&);
    ~AnalogExactAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    AnalogExactAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& valueTreeState;
    
    // VU Meters
    VUMeter vuMeterL;
    VUMeter vuMeterR;
    
    // Knobs
    RotaryKnob inputKnob {"INPUT"};
    RotaryKnob outputKnob {"OUTPUT"};
    RotaryKnob biasKnob {"BIAS"};
    
    // Labels for knob values
    juce::Label inputLabel;
    juce::Label outputLabel;
    juce::Label biasLabel;
    
    // Monitor buttons
    juce::TextButton monitorInputBtn {"INPUT"};
    juce::TextButton monitorReproBtn {"REPRO"};
    
    // Headblock buttons
    juce::TextButton headblockStereoBtn {"STEREO 2-TRK"};
    juce::TextButton headblockMonoBtn {"MONO"};
    
    // Speed buttons
    juce::TextButton speed75Btn {"7.5"};
    juce::TextButton speed15Btn {"15"};
    juce::TextButton speed30Btn {"30"};
    
    // Flux buttons
    juce::TextButton flux185Btn {"185"};
    juce::TextButton flux250Btn {"250"};
    juce::TextButton flux370Btn {"370"};
    
    // EQ buttons
    juce::TextButton eqNABBtn {"NAB"};
    juce::TextButton eqIECBtn {"IEC/CCIR"};
    
    // Tape type buttons
    juce::TextButton tape406Btn {"406"};
    juce::TextButton tape456Btn {"456"};
    juce::TextButton tape499Btn {"499"};
    juce::TextButton tapeGP9Btn {"GP9"};
    juce::TextButton tapeSM900Btn {"SM900"};
    juce::TextButton tapeSM911Btn {"SM911"};
    
    // Toggle buttons
    juce::ToggleButton transformerBtn {"Transformer I/O"};
    juce::ToggleButton autoCalBtn {"Auto Cal"};
    
    // Info panel toggle
    juce::TextButton techPanelBtn {"Technician's Setup Panel"};
    bool techPanelOpen = false;
    
    // Labels
    juce::Label titleLabel;
    juce::Label subtitleLabel;
    
    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> biasAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> transformerAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> autoCalAttachment;
    
    // Hidden sliders for knob attachments
    juce::Slider inputSlider;
    juce::Slider outputSlider;
    juce::Slider biasSlider;
    
    void setupButton (juce::TextButton& button, const juce::String& tooltip = {});
    void updateMonitorButtons();
    void updateHeadblockButtons();
    void updateSpeedButtons();
    void updateFluxButtons();
    void updateEQButtons();
    void updateTapeButtons();
    void updateKnobLabels();
    
    juce::Colour getPanelColour() const { return juce::Colour (0xff3a4454); }
    juce::Colour getModuleColour() const { return juce::Colour (0xff2d3748); }
    juce::Colour getActiveColour() const { return juce::Colour (0xfff6ad55); }
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogExactAudioProcessorEditor)
};
