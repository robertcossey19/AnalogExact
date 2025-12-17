#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AnalogExactAudioProcessor::AnalogExactAudioProcessor()
    : AudioProcessor (BusesProperties()
                      .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, juce::Identifier ("AnalogExact"), createParameterLayout())
{
}

AnalogExactAudioProcessor::~AnalogExactAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AnalogExactAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        INPUT_GAIN_ID, "Input Gain", -12.0f, 12.0f, 0.0f));
    
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        OUTPUT_GAIN_ID, "Output Gain", -24.0f, 6.0f, 0.0f));
    
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        BIAS_ID, "Bias", -5.0f, 5.0f, 0.0f));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        MONITOR_ID, "Monitor", juce::StringArray {"Input", "Repro"}, 0));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        SPEED_ID, "Speed", juce::StringArray {"7.5", "15", "30"}, 1));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        FLUX_ID, "Fluxivity", juce::StringArray {"185", "250", "370"}, 1));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        EQ_ID, "EQ Standard", juce::StringArray {"NAB", "IEC"}, 0));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        TAPE_TYPE_ID, "Tape Type", 
        juce::StringArray {"406", "456", "499", "GP9", "SM900", "SM911"}, 1));
    
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        TRANSFORMER_ID, "Transformer I/O", true));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        HEADBLOCK_ID, "Headblock", juce::StringArray {"Stereo", "Mono"}, 0));
    
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        AUTO_CAL_ID, "Auto Cal", true));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String AnalogExactAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AnalogExactAudioProcessor::acceptsMidi() const
{
    return false;
}

bool AnalogExactAudioProcessor::producesMidi() const
{
    return false;
}

bool AnalogExactAudioProcessor::isMidiEffect() const
{
    return false;
}

double AnalogExactAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AnalogExactAudioProcessor::getNumPrograms()
{
    return 1;
}

int AnalogExactAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AnalogExactAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AnalogExactAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AnalogExactAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AnalogExactAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    
    for (auto& ch : channelDSP)
    {
        ch.reset();
        
        ch.inputGainSmooth.reset (sampleRate, 0.01);
        ch.fluxGainSmooth.reset (sampleRate, 0.01);
        ch.headroomGainSmooth.reset (sampleRate, 0.01);
        ch.outGainSmooth.reset (sampleRate, 0.01);
        ch.xfTrimSmooth.reset (sampleRate, 0.02);
    }
    
    stereoMix.reset (sampleRate, 0.01);
    monoMix.reset (sampleRate, 0.01);
    
    updateDSPParameters();
}

void AnalogExactAudioProcessor::releaseResources()
{
}

bool AnalogExactAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != layouts.getMainOutputChannelSet())
        return false;

    return true;
}

//==============================================================================
float AnalogExactAudioProcessor::dbToGain (float db)
{
    return std::pow (10.0f, juce::jlimit (-60.0f, 20.0f, db) / 20.0f);
}

float AnalogExactAudioProcessor::clampDb (float db, float min, float max)
{
    return juce::jlimit (min, max, db);
}

AnalogExactAudioProcessor::TapeProfile AnalogExactAudioProcessor::getTapeProfile (int tapeType)
{
    const TapeProfile profiles[] = {
        {0.78f, 0.78f, 1.00f, +0.3f}, // 406
        {0.82f, 0.75f, 1.05f, +0.4f}, // 456
        {0.88f, 0.68f, 1.20f, +0.2f}, // 499
        {0.90f, 0.66f, 1.30f,  0.0f}, // GP9
        {0.92f, 0.65f, 1.35f, -0.1f}, // SM900
        {0.86f, 0.70f, 1.12f, +0.1f}  // SM911
    };
    
    return profiles[juce::jlimit (0, 5, tapeType)];
}

AnalogExactAudioProcessor::EQCurve AnalogExactAudioProcessor::getEQCurve (int speed, int eq)
{
    // speed: 0=7.5, 1=15, 2=30; eq: 0=NAB, 1=IEC
    const EQCurve curves[2][3] = {
        // NAB
        {
            {3180.0f, -1.25f, 60.0f, 2.4f},  // 7.5 ips
            {3180.0f, -0.75f, 80.0f, 1.8f},  // 15 ips
            {3180.0f, -0.50f, 100.0f, 1.4f}  // 30 ips
        },
        // IEC
        {
            {2270.0f, -0.75f, 55.0f, 1.8f},  // 7.5 ips
            {4550.0f, -0.25f, 75.0f, 1.6f},  // 15 ips
            {9100.0f,  0.00f, 95.0f, 0.7f}   // 30 ips
        }
    };
    
    int speedIdx = juce::jlimit (0, 2, speed);
    int eqIdx = juce::jlimit (0, 1, eq);
    
    return curves[eqIdx][speedIdx];
}

float AnalogExactAudioProcessor::applyBiasShaping (float input, float biasDb)
{
    // Bias asymmetry curve
    const float baseK = 1.6f;
    const float asym = 1.0f - juce::jlimit (-0.25f, 0.25f, biasDb * 0.05f);
    
    if (input >= 0.0f)
    {
        return std::tanh (input * baseK);
    }
    else
    {
        return std::tanh (input * baseK / asym);
    }
}

float AnalogExactAudioProcessor::applyTransformerShaping (float input)
{
    const float k = 3.0f;
    return std::tanh (input * k) * 0.98f;
}

void AnalogExactAudioProcessor::updateDSPParameters()
{
    auto inputDb = parameters.getRawParameterValue (INPUT_GAIN_ID)->load();
    auto outputDb = parameters.getRawParameterValue (OUTPUT_GAIN_ID)->load();
    auto biasDb = parameters.getRawParameterValue (BIAS_ID)->load();
    auto flux = (int) parameters.getRawParameterValue (FLUX_ID)->load();
    auto speed = (int) parameters.getRawParameterValue (SPEED_ID)->load();
    auto eq = (int) parameters.getRawParameterValue (EQ_ID)->load();
    auto tapeType = (int) parameters.getRawParameterValue (TAPE_TYPE_ID)->load();
    auto transformer = parameters.getRawParameterValue (TRANSFORMER_ID)->load() > 0.5f;
    auto headblock = (int) parameters.getRawParameterValue (HEADBLOCK_ID)->load();
    
    // Flux mapping
    const float fluxDbValues[] = {0.0f, 2.6f, 6.0f};
    float fluxDb = fluxDbValues[juce::jlimit (0, 2, flux)];
    
    // Input with offset
    float inDbEff = inputDb + INPUT_DB_OFFSET;
    float inGain = dbToGain (inDbEff);
    float flGain = dbToGain (fluxDb);
    
    // Transformer reduces flux by 1 dB
    if (transformer)
        flGain *= dbToGain (-1.0f);
    
    // Headroom and output
    float hrGain = dbToGain (currentHeadroomDb);
    float outMakeupDb = transformer ? 1.0f : 0.0f;
    float outGain = dbToGain (outputDb + outMakeupDb);
    
    // Tape profile
    auto profile = getTapeProfile (tapeType);
    
    // EQ curve
    auto curve = getEQCurve (speed, eq);
    
    // Bias HF tilt
    float biasTilt = juce::jlimit (-3.0f, 3.0f, -0.6f * biasDb);
    
    // Headblock routing
    bool isMono = (headblock == 1);
    stereoMix.setTargetValue (isMono ? 0.0f : 1.0f);
    monoMix.setTargetValue (isMono ? 1.0f : 0.0f);
    
    // Update all channel DSP
    for (int ch = 0; ch < 2; ++ch)
    {
        auto& dsp = channelDSP[ch];
        
        dsp.inputGainSmooth.setTargetValue (inGain);
        dsp.fluxGainSmooth.setTargetValue (flGain);
        dsp.headroomGainSmooth.setTargetValue (hrGain);
        dsp.outGainSmooth.setTargetValue (outGain);
        dsp.xfTrimSmooth.setTargetValue (xfTrimTarget);
        
        // Bias HF filter (high shelf @ 8kHz)
        auto biasHFCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            getSampleRate(), 8000.0f, 0.707f, dbToGain (biasTilt));
        *dsp.biasHF.coefficients = *biasHFCoeffs;
        
        // Head bump (peaking)
        auto headBumpCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            getSampleRate(), curve.bumpFreq, 1.1f, dbToGain (curve.bumpGain + profile.bump));
        *dsp.headBump.coefficients = *headBumpCoeffs;
        
        // De-emphasis (high shelf)
        auto deemphCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            getSampleRate(), curve.reproFreq, 0.707f, dbToGain (curve.reproGain));
        *dsp.deemph.coefficients = *deemphCoeffs;
        
        // Lowpass
        float lpFreq = transformer ? 22000.0f : 22050.0f;
        auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (
            getSampleRate(), lpFreq);
        *dsp.lpOut.coefficients = *lpCoeffs;
    }
}

//==============================================================================
void AnalogExactAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateDSPParameters();
    
    auto monitor = (int) parameters.getRawParameterValue (MONITOR_ID)->load();
    auto biasDb = parameters.getRawParameterValue (BIAS_ID)->load();
    auto transformer = parameters.getRawParameterValue (TRANSFORMER_ID)->load() > 0.5f;
    auto headblock = (int) parameters.getRawParameterValue (HEADBLOCK_ID)->load();
    bool isMono = (headblock == 1);
    
    const int numSamples = buffer.getNumSamples();
    
    // Create dry buffer for INPUT monitor
    juce::AudioBuffer<float> dryBuffer;
    if (monitor == 0) // Input
    {
        dryBuffer.makeCopyOf (buffer);
    }
    
    // Process based on headblock mode
    if (isMono)
    {
        // Full-track mono: sum L+R, process as mono, duplicate to both outputs
        auto* leftData = buffer.getWritePointer (0);
        auto* rightData = buffer.getWritePointer (1);
        
        for (int i = 0; i < numSamples; ++i)
        {
            float monoSample = (leftData[i] + rightData[i]) * 0.5f;
            
            // Processed chain (mono)
            float processed = monoSample;
            
            if (monitor == 1) // Repro
            {
                auto& dsp = channelDSP[0]; // Use channel 0 for mono
                
                // Input gain
                processed *= dsp.inputGainSmooth.getNextValue();
                
                // Flux gain
                processed *= dsp.fluxGainSmooth.getNextValue();
                
                // Bias shaping
                processed = applyBiasShaping (processed, biasDb);
                
                // Bias HF
                processed = dsp.biasHF.processSample (processed);
                
                // Headroom
                processed *= dsp.headroomGainSmooth.getNextValue();
                
                // Head bump
                processed = dsp.headBump.processSample (processed);
                
                // De-emphasis
                processed = dsp.deemph.processSample (processed);
                
                // Output gain
                processed *= dsp.outGainSmooth.getNextValue();
                
                // Transformer trim
                processed *= dsp.xfTrimSmooth.getNextValue();
                
                // Transformer shaping
                if (transformer)
                    processed = applyTransformerShaping (processed);
                
                // Lowpass
                processed = dsp.lpOut.processSample (processed);
            }
            
            // Output to both channels
            leftData[i] = processed;
            rightData[i] = processed;
        }
    }
    else
    {
        // Stereo processing with crosstalk
        auto* leftData = buffer.getWritePointer (0);
        auto* rightData = buffer.getWritePointer (1);
        
        const float ctLin = dbToGain (CROSSTALK_DB); // ~0.01
        const float mainLin = 1.0f - ctLin;
        
        for (int i = 0; i < numSamples; ++i)
        {
            float leftIn = leftData[i];
            float rightIn = rightData[i];
            
            float leftOut = leftIn;
            float rightOut = rightIn;
            
            if (monitor == 1) // Repro
            {
                // Process left channel
                {
                    auto& dsp = channelDSP[0];
                    float processed = leftIn;
                    
                    processed *= dsp.inputGainSmooth.getNextValue();
                    processed *= dsp.fluxGainSmooth.getNextValue();
                    processed = applyBiasShaping (processed, biasDb);
                    processed = dsp.biasHF.processSample (processed);
                    processed *= dsp.headroomGainSmooth.getNextValue();
                    processed = dsp.headBump.processSample (processed);
                    processed = dsp.deemph.processSample (processed);
                    processed *= dsp.outGainSmooth.getNextValue();
                    processed *= dsp.xfTrimSmooth.getNextValue();
                    if (transformer)
                        processed = applyTransformerShaping (processed);
                    processed = dsp.lpOut.processSample (processed);
                    
                    leftOut = processed;
                }
                
                // Process right channel
                {
                    auto& dsp = channelDSP[1];
                    float processed = rightIn;
                    
                    processed *= dsp.inputGainSmooth.getNextValue();
                    processed *= dsp.fluxGainSmooth.getNextValue();
                    processed = applyBiasShaping (processed, biasDb);
                    processed = dsp.biasHF.processSample (processed);
                    processed *= dsp.headroomGainSmooth.getNextValue();
                    processed = dsp.headBump.processSample (processed);
                    processed = dsp.deemph.processSample (processed);
                    processed *= dsp.outGainSmooth.getNextValue();
                    processed *= dsp.xfTrimSmooth.getNextValue();
                    if (transformer)
                        processed = applyTransformerShaping (processed);
                    processed = dsp.lpOut.processSample (processed);
                    
                    rightOut = processed;
                }
                
                // Apply crosstalk matrix
                float leftWithCrosstalk = leftOut * mainLin + rightOut * ctLin;
                float rightWithCrosstalk = rightOut * mainLin + leftOut * ctLin;
                
                leftOut = leftWithCrosstalk;
                rightOut = rightWithCrosstalk;
            }
            
            leftData[i] = leftOut;
            rightData[i] = rightOut;
        }
    }
    
    // If INPUT monitor, replace with dry signal
    if (monitor == 0)
    {
        buffer.makeCopyOf (dryBuffer);
    }
    
    // Update VU meters
    float sumL = 0.0f, sumR = 0.0f;
    auto* leftData = buffer.getReadPointer (0);
    auto* rightData = buffer.getReadPointer (1);
    
    for (int i = 0; i < numSamples; ++i)
    {
        sumL += leftData[i] * leftData[i];
        sumR += rightData[i] * rightData[i];
    }
    
    float rmsL = std::sqrt (sumL / numSamples);
    float rmsR = std::sqrt (sumR / numSamples);
    
    inputLevelL.store (rmsL);
    inputLevelR.store (rmsR);
}

//==============================================================================
bool AnalogExactAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* AnalogExactAudioProcessor::createEditor()
{
    return new AnalogExactAudioProcessorEditor (*this, parameters);
}

//==============================================================================
void AnalogExactAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void AnalogExactAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AnalogExactAudioProcessor();
}
