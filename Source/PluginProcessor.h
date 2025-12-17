#pragma once

#include <JuceHeader.h>

//==============================================================================
class AnalogExactAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    AnalogExactAudioProcessor();
    ~AnalogExactAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Parameter IDs
    static constexpr const char* INPUT_GAIN_ID = "inputGain";
    static constexpr const char* OUTPUT_GAIN_ID = "outputGain";
    static constexpr const char* BIAS_ID = "bias";
    static constexpr const char* MONITOR_ID = "monitor";
    static constexpr const char* SPEED_ID = "speed";
    static constexpr const char* FLUX_ID = "flux";
    static constexpr const char* EQ_ID = "eq";
    static constexpr const char* TAPE_TYPE_ID = "tapeType";
    static constexpr const char* TRANSFORMER_ID = "transformer";
    static constexpr const char* HEADBLOCK_ID = "headblock";
    static constexpr const char* AUTO_CAL_ID = "autoCal";

    // VU meter access
    float getInputLevelL() const { return inputLevelL.load(); }
    float getInputLevelR() const { return inputLevelR.load(); }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    
    std::atomic<float> inputLevelL {0.0f};
    std::atomic<float> inputLevelR {0.0f};
    
    // DSP components for each channel
    struct ChannelDSP
    {
        // Filters
        juce::dsp::IIR::Filter<float> biasHF;
        juce::dsp::IIR::Filter<float> headBump;
        juce::dsp::IIR::Filter<float> deemph;
        juce::dsp::IIR::Filter<float> lpOut;
        
        // Level smoothers
        juce::SmoothedValue<float> inputGainSmooth;
        juce::SmoothedValue<float> fluxGainSmooth;
        juce::SmoothedValue<float> headroomGainSmooth;
        juce::SmoothedValue<float> outGainSmooth;
        juce::SmoothedValue<float> xfTrimSmooth;
        
        void reset()
        {
            biasHF.reset();
            headBump.reset();
            deemph.reset();
            lpOut.reset();
        }
    };
    
    std::array<ChannelDSP, 2> channelDSP;
    
    // Mono summing
    juce::SmoothedValue<float> stereoMix {1.0f};
    juce::SmoothedValue<float> monoMix {0.0f};
    
    // Constants
    static constexpr float INPUT_DB_OFFSET = -5.0f;
    static constexpr float DEFAULT_HEADROOM_DB = -12.0f;
    static constexpr float CROSSTALK_DB = -40.0f;
    
    // Current state
    float currentHeadroomDb = DEFAULT_HEADROOM_DB;
    float xfTrimTarget = 1.0f;
    
    // Helper functions
    static float dbToGain(float db);
    static float clampDb(float db, float min, float max);
    void updateDSPParameters();
    float applyBiasShaping(float input, float biasDb);
    float applyTransformerShaping(float input);
    
    struct TapeProfile
    {
        float knee;
        float asym;
        float satScale;
        float bump;
    };
    
    static TapeProfile getTapeProfile(int tapeType);
    
    struct EQCurve
    {
        float reproFreq;
        float reproGain;
        float bumpFreq;
        float bumpGain;
    };
    
    static EQCurve getEQCurve(int speed, int eq);
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalogExactAudioProcessor)
};
