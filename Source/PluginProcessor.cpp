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
        juce::ParameterID { INPUT_GAIN_ID, 1 }, "Input Gain",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { OUTPUT_GAIN_ID, 1 }, "Output Gain",
        juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f));
    
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { BIAS_ID, 1 }, "Bias",
        juce::NormalisableRange<float> (-5.0f, 5.0f, 0.1f), 0.0f));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { MONITOR_ID, 1 }, "Monitor",
        juce::StringArray {"Input", "Repro"}, 0));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { SPEED_ID, 1 }, "Speed",
        juce::StringArray {"7.5", "15", "30"}, 1));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { FLUX_ID, 1 }, "Fluxivity",
        juce::StringArray {"185", "250", "370"}, 1));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { EQ_ID, 1 }, "EQ Standard",
        juce::StringArray {"NAB", "IEC"}, 0));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { TAPE_TYPE_ID, 1 }, "Tape Type",
        juce::StringArray {"406", "456", "499", "GP9", "SM900", "SM911"}, 1));
    
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { TRANSFORMER_ID, 1 }, "Transformer I/O", true));
    
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { HEADBLOCK_ID, 1 }, "Headblock",
        juce::StringArray {"Stereo", "Mono"}, 0));
    
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { AUTO_CAL_ID, 1 }, "Auto Cal", true));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String AnalogExactAudioProcessor::getName() const
{
    return "AnalogExact";
}

bool AnalogExactAudioProcessor::acceptsMidi() const { return false; }
bool AnalogExactAudioProcessor::producesMidi() const { return false; }
bool AnalogExactAudioProcessor::isMidiEffect() const { return false; }
double AnalogExactAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int AnalogExactAudioProcessor::getNumPrograms() { return 1; }
int AnalogExactAudioProcessor::getCurrentProgram() { return 0; }
void AnalogExactAudioProcessor::setCurrentProgram (int) {}
const juce::String AnalogExactAudioProcessor::getProgramName (int) { return {}; }
void AnalogExactAudioProcessor::changeProgramName (int, const juce::String&) {}

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

void AnalogExactAudioProcessor::releaseResources() {}

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
        {0.78f, 0.78f, 1.00f, +0.3f},
        {0.82f, 0.75f, 1.05f, +0.4f},
        {0.88f, 0.68f, 1.20f, +0.2f},
        {0.90f, 0.66f, 1.30f,  0.0f},
        {0.92f, 0.65f, 1.35f, -0.1f},
        {0.86f, 0.70f, 1.12f, +0.1f}
    };
    return profiles[juce::jlimit (0, 5, tapeType)];
}

AnalogExactAudioProcessor::EQCurve AnalogExactAudioProcessor::getEQCurve (int speed, int eq)
{
    const EQCurve curves[2][3] = {
        {{3180.0f, -1.25f, 60.0f, 2.4f}, {3180.0f, -0.75f, 80.0f, 1.8f}, {3180.0f, -0.50f, 100.0f, 1.4f}},
        {{2270.0f, -0.75f, 55.0f, 1.8f}, {4550.0f, -0.25f, 75.0f, 1.6f}, {9100.0f, 0.00f, 95.0f, 0.7f}}
    };
    return curves[juce::jlimit (0, 1, eq)][juce::jlimit (0, 2, speed)];
}

float AnalogExactAudioProcessor::applyBiasShaping (float input, float biasDb)
{
    const float baseK = 1.6f;
    const float asym = 1.0f - juce::jlimit (-0.25f, 0.25f, biasDb * 0.05f);
    return input >= 0.0f ? std::tanh (input * baseK) : std::tanh (input * baseK / asym);
}

float AnalogExactAudioProcessor::applyTransformerShaping (float input)
{
    return std::tanh (input * 3.0f) * 0.98f;
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
    
    const float fluxDbValues[] = {0.0f, 2.6f, 6.0f};
    float fluxDb = fluxDbValues[juce::jlimit (0, 2, flux)];
    
    float inDbEff = inputDb + INPUT_DB_OFFSET;
    float inGain = dbToGain (inDbEff);
    float flGain = dbToGain (fluxDb);
    
    if (transformer) flGain *= dbToGain (-1.0f);
    
    float hrGain = dbToGain (currentHeadroomDb);
    float outMakeupDb = transformer ? 1.0f : 0.0f;
    float outGain = dbToGain (outputDb + outMakeupDb);
    
    auto profile = getTapeProfile (tapeType);
    auto curve = getEQCurve (speed, eq);
    float biasTilt = juce::jlimit (-3.0f, 3.0f, -0.6f * biasDb);
    
    bool isMono = (headblock == 1);
    stereoMix.setTargetValue (isMono ? 0.0f : 1.0f);
    monoMix.setTargetValue (isMono ? 1.0f : 0.0f);
    
    for (size_t ch = 0; ch < 2; ++ch)
    {
        auto& dsp = channelDSP[ch];
        
        dsp.inputGainSmooth.setTargetValue (inGain);
        dsp.fluxGainSmooth.setTargetValue (flGain);
        dsp.headroomGainSmooth.setTargetValue (hrGain);
        dsp.outGainSmooth.setTargetValue (outGain);
        dsp.xfTrimSmooth.setTargetValue (xfTrimTarget);
        
        auto biasHFCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            getSampleRate(), 8000.0f, 0.707f, dbToGain (biasTilt));
        *dsp.biasHF.coefficients = *biasHFCoeffs;
        
        auto headBumpCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter (
            getSampleRate(), curve.bumpFreq, 1.1f, dbToGain (curve.bumpGain + profile.bump));
        *dsp.headBump.coefficients = *headBumpCoeffs;
        
        auto deemphCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf (
            getSampleRate(), curve.reproFreq, 0.707f, dbToGain (curve.reproGain));
        *dsp.deemph.coefficients = *deemphCoeffs;
        
        float lpFreq = transformer ? 22000.0f : 22050.0f;
        auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass (getSampleRate(), lpFreq);
        *dsp.lpOut.coefficients = *lpCoeffs;
    }
}

//==============================================================================
void AnalogExactAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
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
    
    juce::AudioBuffer<float> dryBuffer;
    if (monitor == 0) dryBuffer.makeCopyOf (buffer);
    
    if (isMono)
    {
        auto* leftData = buffer.getWritePointer (0);
        auto* rightData = buffer.getWritePointer (1);
        
        for (int i = 0; i < numSamples; ++i)
        {
            float monoSample = (leftData[i] + rightData[i]) * 0.5f;
            float processed = monoSample;
            
            if (monitor == 1)
            {
                auto& dsp = channelDSP[0];
                processed *= dsp.inputGainSmooth.getNextValue();
                processed *= dsp.fluxGainSmooth.getNextValue();
                processed = applyBiasShaping (processed, biasDb);
                processed = dsp.biasHF.processSample (processed);
                processed *= dsp.headroomGainSmooth.getNextValue();
                processed = dsp.headBump.processSample (processed);
                processed = dsp.deemph.processSample (processed);
                processed *= dsp.outGainSmooth.getNextValue();
                processed *= dsp.xfTrimSmooth.getNextValue();
                if (transformer) processed = applyTransformerShaping (processed);
                processed = dsp.lpOut.processSample (processed);
            }
            
            leftData[i] = processed;
            rightData[i] = processed;
        }
    }
    else
    {
        auto* leftData = buffer.getWritePointer (0);
        auto* rightData = buffer.getWritePointer (1);
        
        const float ctLin = dbToGain (CROSSTALK_DB);
        const float mainLin = 1.0f - ctLin;
        
        for (int i = 0; i < numSamples; ++i)
        {
            float leftIn = leftData[i];
            float rightIn = rightData[i];
            float leftOut = leftIn;
            float rightOut = rightIn;
            
            if (monitor == 1)
            {
                {
                    auto& dsp = channelDSP[0];
                    float p = leftIn;
                    p *= dsp.inputGainSmooth.getNextValue();
                    p *= dsp.fluxGainSmooth.getNextValue();
                    p = applyBiasShaping (p, biasDb);
                    p = dsp.biasHF.processSample (p);
                    p *= dsp.headroomGainSmooth.getNextValue();
                    p = dsp.headBump.processSample (p);
                    p = dsp.deemph.processSample (p);
                    p *= dsp.outGainSmooth.getNextValue();
                    p *= dsp.xfTrimSmooth.getNextValue();
                    if (transformer) p = applyTransformerShaping (p);
                    p = dsp.lpOut.processSample (p);
                    leftOut = p;
                }
                {
                    auto& dsp = channelDSP[1];
                    float p = rightIn;
                    p *= dsp.inputGainSmooth.getNextValue();
                    p *= dsp.fluxGainSmooth.getNextValue();
                    p = applyBiasShaping (p, biasDb);
                    p = dsp.biasHF.processSample (p);
                    p *= dsp.headroomGainSmooth.getNextValue();
                    p = dsp.headBump.processSample (p);
                    p = dsp.deemph.processSample (p);
                    p *= dsp.outGainSmooth.getNextValue();
                    p *= dsp.xfTrimSmooth.getNextValue();
                    if (transformer) p = applyTransformerShaping (p);
                    p = dsp.lpOut.processSample (p);
                    rightOut = p;
                }
                
                float leftWithCt = leftOut * mainLin + rightOut * ctLin;
                float rightWithCt = rightOut * mainLin + leftOut * ctLin;
                leftOut = leftWithCt;
                rightOut = rightWithCt;
            }
            
            leftData[i] = leftOut;
            rightData[i] = rightOut;
        }
    }
    
    if (monitor == 0) buffer.makeCopyOf (dryBuffer);
    
    float sumL = 0.0f, sumR = 0.0f;
    auto* leftData = buffer.getReadPointer (0);
    auto* rightData = buffer.getReadPointer (1);
    
    for (int i = 0; i < numSamples; ++i)
    {
        sumL += leftData[i] * leftData[i];
        sumR += rightData[i] * rightData[i];
    }
    
    inputLevelL.store (std::sqrt (sumL / (float) numSamples));
    inputLevelR.store (std::sqrt (sumR / (float) numSamples));
}

//==============================================================================
bool AnalogExactAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* AnalogExactAudioProcessor::createEditor()
{
    return new AnalogExactAudioProcessorEditor (*this);
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
