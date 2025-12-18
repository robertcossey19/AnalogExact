#include "PluginProcessor.h"
#include "PluginEditor.h"

AnalogExactAudioProcessorEditor::AnalogExactAudioProcessorEditor(AnalogExactAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor(&p), audioProcessor(p), valueTreeState(vts)
{
    // Initialize all sliders and labels first
    warmthSlider = std::make_unique<juce::Slider>();
    bitDepthSlider = std::make_unique<juce::Slider>();
    saturationSlider = std::make_unique<juce::Slider>();
    noiseSlider = std::make_unique<juce::Slider>();
    dryWetSlider = std::make_unique<juce::Slider>();

    warmthLabel = std::make_unique<juce::Label>();
    bitDepthLabel = std::make_unique<juce::Label>();
    saturationLabel = std::make_unique<juce::Label>();
    noiseLabel = std::make_unique<juce::Label>();
    dryWetLabel = std::make_unique<juce::Label>();

    // Setup each slider with its label
    setupSlider(warmthSlider, warmthLabel, "Warmth", "warmth");
    setupSlider(bitDepthSlider, bitDepthLabel, "Bit Depth", "bitDepth");
    setupSlider(saturationSlider, saturationLabel, "Saturation", "saturation");
    setupSlider(noiseSlider, noiseLabel, "Noise", "noise");
    setupSlider(dryWetSlider, dryWetLabel, "Dry/Wet", "dryWet");

    // Create attachments after sliders are initialized
    warmthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "warmth", *warmthSlider);
    bitDepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "bitDepth", *bitDepthSlider);
    saturationAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "saturation", *saturationSlider);
    noiseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "noise", *noiseSlider);
    dryWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(valueTreeState, "dryWet", *dryWetSlider);

    // Set editor size last
    setSize(400, 500);
}

AnalogExactAudioProcessorEditor::~AnalogExactAudioProcessorEditor()
{
}

void AnalogExactAudioProcessorEditor::setupSlider(std::unique_ptr<juce::Slider>& slider,
                                                   std::unique_ptr<juce::Label>& label,
                                                   const juce::String& labelText,
                                                   const juce::String& parameterID)
{
    // Verify slider is initialized
    if (!slider) return;
    
    slider->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible(slider.get());

    // Setup label if it exists
    if (label)
    {
        label->setText(labelText, juce::dontSendNotification);
        label->setJustificationType(juce::Justification::centred);
        label->attachToComponent(slider.get(), false);
        addAndMakeVisible(label.get());
    }
}

void AnalogExactAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawFittedText("AnalogExact", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);
}

void AnalogExactAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(40); // Title space

    int sliderHeight = 100;
    int spacing = 10;

    // Only resize if sliders exist
    if (warmthSlider) warmthSlider->setBounds(area.removeFromTop(sliderHeight));
    area.removeFromTop(spacing);
    
    if (bitDepthSlider) bitDepthSlider->setBounds(area.removeFromTop(sliderHeight));
    area.removeFromTop(spacing);
    
    if (saturationSlider) saturationSlider->setBounds(area.removeFromTop(sliderHeight));
    area.removeFromTop(spacing);
    
    if (noiseSlider) noiseSlider->setBounds(area.removeFromTop(sliderHeight));
    area.removeFromTop(spacing);
    
    if (dryWetSlider) dryWetSlider->setBounds(area.removeFromTop(sliderHeight));
}
