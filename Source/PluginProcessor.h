#pragma once
#include <JuceHeader.h>
#include "Parameters.h"
#include "Voice.h"
#include "Effects.h"
#include "FactoryPresets.h"

class GritwaveProcessor : public juce::AudioProcessor
{
public:
    GritwaveProcessor();
    ~GritwaveProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Gritwave"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override { return (int) Gw::getFactoryPresets().size(); }
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static constexpr int numVoices = 8;

    void readParams();
    void handleMidi (const juce::MidiMessage& m);
    void noteOn (int note, float vel);
    void noteOff (int note);
    void allNotesOff();
    void renderVoices (float* l, float* r, int count);

    Gw::Voice voices[numVoices];
    Gw::VoiceParams vp;
    Gw::FxParams fxp;
    Gw::FxChain fx;

    juce::AudioBuffer<float> synthBuf;
    std::vector<std::pair<int, float>> monoStack;
    uint32_t voiceCounter = 0;
    float pitchBend = 0.f;
    int currentProgram = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GritwaveProcessor)
};
