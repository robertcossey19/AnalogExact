#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// VUMeter Implementation
//==============================================================================
VUMeter::VUMeter()
{
    startTimerHz (30);
}

void VUMeter::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Housing
    g.setColour (juce::Colour (0xff111827));
    g.fillRoundedRectangle (bounds, 8.0f);
    
    g.setColour (juce::Colour (0xff4b5563));
    g.drawRoundedRectangle (bounds.reduced (2.0f), 8.0f, 3.0f);
    
    // Scale background
    auto scaleBounds = bounds.reduced (12.0f, 30.0f);
    g.setColour (juce::Colour (0xffffebcd));
    g.fillRoundedRectangle (scaleBounds, 4.0f);
    
    // Needle position (-70 to +20 degrees)
    float dbLevel = 20.0f * std::log10 (displayLevel + 1e-9f);
    float angle = juce::jmap (dbLevel, -50.0f, 5.0f, -70.0f, 20.0f);
    angle = juce::jlimit (-70.0f, 20.0f, angle);
    
    // Draw needle
    auto center = scaleBounds.getCentre();
    float needleLength = scaleBounds.getHeight();
    float angleRad = angle * juce::MathConstants<float>::pi / 180.0f;
    
    juce::Point<float> needleEnd (
        center.x + needleLength * 0.5f * std::sin (angleRad),
        scaleBounds.getBottom() - needleLength * 0.5f * std::cos (angleRad)
    );
    
    g.setColour (juce::Colour (0xffdc2626));
    g.drawLine (center.x, scaleBounds.getBottom(), needleEnd.x, needleEnd.y, 2.0f);
}

void VUMeter::setLevel (float level)
{
    currentLevel = level;
}

void VUMeter::timerCallback()
{
    // Smooth decay
    float attack = 0.3f;
    float decay = 0.1f;
    
    if (currentLevel > displayLevel)
        displayLevel += (currentLevel - displayLevel) * attack;
    else
        displayLevel += (currentLevel - displayLevel) * decay;
    
    repaint();
}

//==============================================================================
// RotaryKnob Implementation
//==============================================================================
RotaryKnob::RotaryKnob (const juce::String& labelText)
    : label (labelText)
{
}

void RotaryKnob::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto knobArea = bounds.reduced (10.0f, 10.0f);
    
    // Knob body
    g.setGradientFill (juce::ColourGradient (
        juce::Colour (0xff5a6578), knobArea.getTopLeft(),
        juce::Colour (0xff2d3748), knobArea.getBottomRight(),
        false));
    g.fillEllipse (knobArea);
    
    // Border
    g.setColour (juce::Colour (0xff1a202c));
    g.drawEllipse (knobArea, 2.0f);
    
    // Indicator
    float angle = juce::jmap (value, 0.0f, 1.0f, -135.0f, 135.0f);
    float angleRad = angle * juce::MathConstants<float>::pi / 180.0f;
    
    auto center = knobArea.getCentre();
    float radius = knobArea.getWidth() * 0.42f;
    
    juce::Point<float> indicatorPos (
        center.x + radius * std::sin (angleRad),
        center.y - radius * std::cos (angleRad)
    );
    
    g.setColour (juce::Colour (0xffe2e8f0));
    g.fillEllipse (indicatorPos.x - 3.0f, indicatorPos.y - 3.0f, 6.0f, 6.0f);
    
    // Label
    g.setColour (juce::Colours::white);
    g.setFont (14.0f);
    g.drawText (label, bounds.removeFromBottom (20), juce::Justification::centred);
}

void RotaryKnob::resized()
{
}

void RotaryKnob::setValue (float newValue)
{
    value = juce::jlimit (0.0f, 1.0f, newValue);
    repaint();
}

void RotaryKnob::mouseDown (const juce::MouseEvent& e)
{
    dragStart = e.getPosition();
    dragStartValue = value;
}

void RotaryKnob::mouseDrag (const juce::MouseEvent& e)
{
    float dragDist = (dragStart.y - e.y) / 150.0f;
    float newValue = juce::jlimit (0.0f, 1.0f, dragStartValue + dragDist);
    
    if (newValue != value)
    {
        value = newValue;
        repaint();
        
        if (onValueChange)
            onValueChange (value);
    }
}

void RotaryKnob::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel)
{
    juce::ignoreUnused (e);
    
    float delta = wheel.deltaY * 0.01f;
    float newValue = juce::jlimit (0.0f, 1.0f, value + delta);
    
    if (newValue != value)
    {
        value = newValue;
        repaint();
        
        if (onValueChange)
            onValueChange (value);
    }
}

//==============================================================================
// Editor Implementation
//==============================================================================
AnalogExactAudioProcessorEditor::AnalogExactAudioProcessorEditor (
    AnalogExactAudioProcessor& p,
    juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState (vts)
{
    setSize (900, 750);
    startTimerHz (30);
    
    // Setup sliders (hidden, for parameter attachments)
    inputSlider.setRange (-12.0, 12.0, 0.1);
    outputSlider.setRange (-24.0, 6.0, 0.1);
    biasSlider.setRange (-5.0, 5.0, 0.1);
    
    addChildComponent (inputSlider);
    addChildComponent (outputSlider);
    addChildComponent (biasSlider);
    
    // Create attachments
    inputAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (
        valueTreeState, AnalogExactAudioProcessor::INPUT_GAIN_ID, inputSlider));
    outputAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (
        valueTreeState, AnalogExactAudioProcessor::OUTPUT_GAIN_ID, outputSlider));
    biasAttachment.reset (new juce::AudioProcessorValueTreeState::SliderAttachment (
        valueTreeState, AnalogExactAudioProcessor::BIAS_ID, biasSlider));
    
    // Setup knobs
    inputKnob.onValueChange = [this] (float val) {
        float dbVal = juce::jmap (val, 0.0f, 1.0f, -12.0f, 12.0f);
        inputSlider.setValue (dbVal, juce::sendNotificationSync);
        updateKnobLabels();
    };
    
    outputKnob.onValueChange = [this] (float val) {
        float dbVal = juce::jmap (val, 0.0f, 1.0f, -24.0f, 6.0f);
        outputSlider.setValue (dbVal, juce::sendNotificationSync);
        updateKnobLabels();
    };
    
    biasKnob.onValueChange = [this] (float val) {
        float dbVal = juce::jmap (val, 0.0f, 1.0f, -5.0f, 5.0f);
        biasSlider.setValue (dbVal, juce::sendNotificationSync);
        updateKnobLabels();
    };
    
    addAndMakeVisible (inputKnob);
    addAndMakeVisible (outputKnob);
    addAndMakeVisible (biasKnob);
    
    // Setup labels
    auto setupLabel = [this] (juce::Label& label) {
        label.setJustificationType (juce::Justification::centred);
        label.setColour (juce::Label::textColourId, juce::Colour (0xfffcd34d));
        addAndMakeVisible (label);
    };
    
    setupLabel (inputLabel);
    setupLabel (outputLabel);
    setupLabel (biasLabel);
    
    // VU Meters
    addAndMakeVisible (vuMeterL);
    addAndMakeVisible (vuMeterR);
    
    // Monitor buttons
    setupButton (monitorInputBtn, "Dry input passthrough");
    setupButton (monitorReproBtn, "Processed tape repro");
    
    monitorInputBtn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::MONITOR_ID);
        param->store (0.0f);
        updateMonitorButtons();
    };
    
    monitorReproBtn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::MONITOR_ID);
        param->store (1.0f);
        updateMonitorButtons();
    };
    
    // Headblock buttons
    setupButton (headblockStereoBtn, "Stereo 2-track recording");
    setupButton (headblockMonoBtn, "Full-track mono");
    
    headblockStereoBtn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::HEADBLOCK_ID);
        param->store (0.0f);
        updateHeadblockButtons();
    };
    
    headblockMonoBtn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::HEADBLOCK_ID);
        param->store (1.0f);
        updateHeadblockButtons();
    };
    
    // Speed buttons
    setupButton (speed75Btn);
    setupButton (speed15Btn);
    setupButton (speed30Btn);
    
    speed75Btn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::SPEED_ID);
        param->store (0.0f);
        updateSpeedButtons();
    };
    
    speed15Btn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::SPEED_ID);
        param->store (1.0f);
        updateSpeedButtons();
    };
    
    speed30Btn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::SPEED_ID);
        param->store (2.0f);
        updateSpeedButtons();
    };
    
    // Flux buttons
    setupButton (flux185Btn);
    setupButton (flux250Btn);
    setupButton (flux370Btn);
    
    flux185Btn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::FLUX_ID);
        param->store (0.0f);
        updateFluxButtons();
    };
    
    flux250Btn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::FLUX_ID);
        param->store (1.0f);
        updateFluxButtons();
    };
    
    flux370Btn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::FLUX_ID);
        param->store (2.0f);
        updateFluxButtons();
    };
    
    // EQ buttons
    setupButton (eqNABBtn);
    setupButton (eqIECBtn);
    
    eqNABBtn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::EQ_ID);
        param->store (0.0f);
        updateEQButtons();
    };
    
    eqIECBtn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::EQ_ID);
        param->store (1.0f);
        updateEQButtons();
    };
    
    // Tape buttons
    setupButton (tape406Btn);
    setupButton (tape456Btn);
    setupButton (tape499Btn);
    setupButton (tapeGP9Btn);
    setupButton (tapeSM900Btn);
    setupButton (tapeSM911Btn);
    
    tape406Btn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::TAPE_TYPE_ID);
        param->store (0.0f);
        updateTapeButtons();
    };
    
    tape456Btn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::TAPE_TYPE_ID);
        param->store (1.0f);
        updateTapeButtons();
    };
    
    tape499Btn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::TAPE_TYPE_ID);
        param->store (2.0f);
        updateTapeButtons();
    };
    
    tapeGP9Btn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::TAPE_TYPE_ID);
        param->store (3.0f);
        updateTapeButtons();
    };
    
    tapeSM900Btn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::TAPE_TYPE_ID);
        param->store (4.0f);
        updateTapeButtons();
    };
    
    tapeSM911Btn.onClick = [this] {
        auto* param = valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::TAPE_TYPE_ID);
        param->store (5.0f);
        updateTapeButtons();
    };
    
    // Toggle buttons
    addAndMakeVisible (transformerBtn);
    addAndMakeVisible (autoCalBtn);
    
    transformerAttachment.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (
        valueTreeState, AnalogExactAudioProcessor::TRANSFORMER_ID, transformerBtn));
    autoCalAttachment.reset (new juce::AudioProcessorValueTreeState::ButtonAttachment (
        valueTreeState, AnalogExactAudioProcessor::AUTO_CAL_ID, autoCalBtn));
    
    transformerBtn.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    autoCalBtn.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
    
    // Tech panel button
    setupButton (techPanelBtn);
    techPanelBtn.onClick = [this] {
        techPanelOpen = !techPanelOpen;
        resized();
    };
    
    // Title
    titleLabel.setText ("ANALOGEXACT", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (28.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);
    
    subtitleLabel.setText ("Analog Tape Emulation • 768 kHz Non-linear Core", juce::dontSendNotification);
    subtitleLabel.setFont (juce::Font (12.0f));
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff9ca3af));
    subtitleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (subtitleLabel);
    
    updateMonitorButtons();
    updateHeadblockButtons();
    updateSpeedButtons();
    updateFluxButtons();
    updateEQButtons();
    updateTapeButtons();
    updateKnobLabels();
}

AnalogExactAudioProcessorEditor::~AnalogExactAudioProcessorEditor()
{
}

//==============================================================================
void AnalogExactAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1a202c));
}

void AnalogExactAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    
    // Title
    auto titleArea = area.removeFromTop (50);
    titleLabel.setBounds (titleArea.removeFromTop (30));
    subtitleLabel.setBounds (titleArea);
    
    area.removeFromTop (10);
    
    // VU Meters
    auto vuArea = area.removeFromTop (100);
    vuMeterL.setBounds (vuArea.removeFromLeft (getWidth() / 2 - 15).reduced (5));
    vuMeterR.setBounds (vuArea.reduced (5));
    
    area.removeFromTop (10);
    
    // Knobs row
    auto knobArea = area.removeFromTop (140);
    int knobWidth = getWidth() / 3;
    
    auto inputArea = knobArea.removeFromLeft (knobWidth);
    inputKnob.setBounds (inputArea.removeFromTop (100).reduced (20));
    inputLabel.setBounds (inputArea.removeFromTop (20));
    
    auto outputArea = knobArea.removeFromLeft (knobWidth);
    outputKnob.setBounds (outputArea.removeFromTop (100).reduced (20));
    outputLabel.setBounds (outputArea.removeFromTop (20));
    
    biasKnob.setBounds (knobArea.removeFromTop (100).reduced (20));
    biasLabel.setBounds (knobArea.removeFromTop (20));
    
    area.removeFromTop (10);
    
    // Monitor section
    auto monitorArea = area.removeFromTop (50);
    auto monBtnArea = monitorArea.reduced (10);
    monitorInputBtn.setBounds (monBtnArea.removeFromLeft (monBtnArea.getWidth() / 2).reduced (5));
    monitorReproBtn.setBounds (monBtnArea.reduced (5));
    
    // Headblock section
    auto headblockArea = area.removeFromTop (50);
    auto headBtnArea = headblockArea.reduced (10);
    headblockStereoBtn.setBounds (headBtnArea.removeFromLeft (headBtnArea.getWidth() / 2).reduced (5));
    headblockMonoBtn.setBounds (headBtnArea.reduced (5));
    
    area.removeFromTop (10);
    
    // Tech panel button
    auto techBtnArea = area.removeFromTop (30);
    techPanelBtn.setBounds (techBtnArea.reduced (getWidth() / 3, 0));
    
    // Tech panel content
    if (techPanelOpen)
    {
        area.removeFromTop (10);
        
        // Speed
        auto speedArea = area.removeFromTop (40);
        int btnW = speedArea.getWidth() / 3;
        speed75Btn.setBounds (speedArea.removeFromLeft (btnW).reduced (5));
        speed15Btn.setBounds (speedArea.removeFromLeft (btnW).reduced (5));
        speed30Btn.setBounds (speedArea.reduced (5));
        
        // Flux
        auto fluxArea = area.removeFromTop (40);
        btnW = fluxArea.getWidth() / 3;
        flux185Btn.setBounds (fluxArea.removeFromLeft (btnW).reduced (5));
        flux250Btn.setBounds (fluxArea.removeFromLeft (btnW).reduced (5));
        flux370Btn.setBounds (fluxArea.reduced (5));
        
        // EQ
        auto eqArea = area.removeFromTop (40);
        eqNABBtn.setBounds (eqArea.removeFromLeft (eqArea.getWidth() / 2).reduced (5));
        eqIECBtn.setBounds (eqArea.reduced (5));
        
        // Tape type
        auto tapeArea = area.removeFromTop (80);
        auto tape1 = tapeArea.removeFromTop (40);
        auto tape2 = tapeArea;
        
        btnW = tape1.getWidth() / 3;
        tape406Btn.setBounds (tape1.removeFromLeft (btnW).reduced (5));
        tape456Btn.setBounds (tape1.removeFromLeft (btnW).reduced (5));
        tape499Btn.setBounds (tape1.reduced (5));
        
        btnW = tape2.getWidth() / 3;
        tapeGP9Btn.setBounds (tape2.removeFromLeft (btnW).reduced (5));
        tapeSM900Btn.setBounds (tape2.removeFromLeft (btnW).reduced (5));
        tapeSM911Btn.setBounds (tape2.reduced (5));
        
        // Toggles
        auto toggleArea = area.removeFromTop (40);
        transformerBtn.setBounds (toggleArea.removeFromLeft (toggleArea.getWidth() / 2).reduced (10));
        autoCalBtn.setBounds (toggleArea.reduced (10));
    }
}

void AnalogExactAudioProcessorEditor::timerCallback()
{
    // Update VU meters
    vuMeterL.setLevel (audioProcessor.getInputLevelL());
    vuMeterR.setLevel (audioProcessor.getInputLevelR());
    
    // Update knobs from parameter values
    float inputVal = juce::jmap (inputSlider.getValue(), -12.0, 12.0, 0.0, 1.0);
    float outputVal = juce::jmap (outputSlider.getValue(), -24.0, 6.0, 0.0, 1.0);
    float biasVal = juce::jmap (biasSlider.getValue(), -5.0, 5.0, 0.0, 1.0);
    
    inputKnob.setValue (inputVal);
    outputKnob.setValue (outputVal);
    biasKnob.setValue (biasVal);
    
    updateKnobLabels();
}

void AnalogExactAudioProcessorEditor::setupButton (juce::TextButton& button, const juce::String& tooltip)
{
    addAndMakeVisible (button);
    button.setColour (juce::TextButton::buttonColourId, getModuleColour());
    button.setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    
    if (tooltip.isNotEmpty())
        button.setTooltip (tooltip);
}

void AnalogExactAudioProcessorEditor::updateMonitorButtons()
{
    auto monitor = (int) valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::MONITOR_ID)->load();
    
    auto activeCol = getActiveColour();
    auto inactiveCol = getModuleColour();
    
    monitorInputBtn.setColour (juce::TextButton::buttonColourId, monitor == 0 ? activeCol : inactiveCol);
    monitorReproBtn.setColour (juce::TextButton::buttonColourId, monitor == 1 ? activeCol : inactiveCol);
}

void AnalogExactAudioProcessorEditor::updateHeadblockButtons()
{
    auto headblock = (int) valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::HEADBLOCK_ID)->load();
    
    auto activeCol = getActiveColour();
    auto inactiveCol = getModuleColour();
    
    headblockStereoBtn.setColour (juce::TextButton::buttonColourId, headblock == 0 ? activeCol : inactiveCol);
    headblockMonoBtn.setColour (juce::TextButton::buttonColourId, headblock == 1 ? activeCol : inactiveCol);
}

void AnalogExactAudioProcessorEditor::updateSpeedButtons()
{
    auto speed = (int) valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::SPEED_ID)->load();
    
    auto activeCol = getActiveColour();
    auto inactiveCol = getModuleColour();
    
    speed75Btn.setColour (juce::TextButton::buttonColourId, speed == 0 ? activeCol : inactiveCol);
    speed15Btn.setColour (juce::TextButton::buttonColourId, speed == 1 ? activeCol : inactiveCol);
    speed30Btn.setColour (juce::TextButton::buttonColourId, speed == 2 ? activeCol : inactiveCol);
}

void AnalogExactAudioProcessorEditor::updateFluxButtons()
{
    auto flux = (int) valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::FLUX_ID)->load();
    
    auto activeCol = getActiveColour();
    auto inactiveCol = getModuleColour();
    
    flux185Btn.setColour (juce::TextButton::buttonColourId, flux == 0 ? activeCol : inactiveCol);
    flux250Btn.setColour (juce::TextButton::buttonColourId, flux == 1 ? activeCol : inactiveCol);
    flux370Btn.setColour (juce::TextButton::buttonColourId, flux == 2 ? activeCol : inactiveCol);
}

void AnalogExactAudioProcessorEditor::updateEQButtons()
{
    auto eq = (int) valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::EQ_ID)->load();
    
    auto activeCol = getActiveColour();
    auto inactiveCol = getModuleColour();
    
    eqNABBtn.setColour (juce::TextButton::buttonColourId, eq == 0 ? activeCol : inactiveCol);
    eqIECBtn.setColour (juce::TextButton::buttonColourId, eq == 1 ? activeCol : inactiveCol);
}

void AnalogExactAudioProcessorEditor::updateTapeButtons()
{
    auto tape = (int) valueTreeState.getRawParameterValue (AnalogExactAudioProcessor::TAPE_TYPE_ID)->load();
    
    auto activeCol = getActiveColour();
    auto inactiveCol = getModuleColour();
    
    tape406Btn.setColour (juce::TextButton::buttonColourId, tape == 0 ? activeCol : inactiveCol);
    tape456Btn.setColour (juce::TextButton::buttonColourId, tape == 1 ? activeCol : inactiveCol);
    tape499Btn.setColour (juce::TextButton::buttonColourId, tape == 2 ? activeCol : inactiveCol);
    tapeGP9Btn.setColour (juce::TextButton::buttonColourId, tape == 3 ? activeCol : inactiveCol);
    tapeSM900Btn.setColour (juce::TextButton::buttonColourId, tape == 4 ? activeCol : inactiveCol);
    tapeSM911Btn.setColour (juce::TextButton::buttonColourId, tape == 5 ? activeCol : inactiveCol);
}

void AnalogExactAudioProcessorEditor::updateKnobLabels()
{
    inputLabel.setText (juce::String (inputSlider.getValue(), 1) + " dB", juce::dontSendNotification);
    outputLabel.setText (juce::String (outputSlider.getValue(), 1) + " dB", juce::dontSendNotification);
    biasLabel.setText (juce::String (biasSlider.getValue(), 1) + " dB", juce::dontSendNotification);
}
