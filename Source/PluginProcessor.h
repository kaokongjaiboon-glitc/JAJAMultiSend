#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <memory>
#include "DeviceSender.h"

class JAJAMultiSendAudioProcessor : public juce::AudioProcessor,
                                    private juce::AsyncUpdater
{
public:
    static constexpr int numSlots = 3;

    JAJAMultiSendAudioProcessor();
    ~JAJAMultiSendAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "JAJA Multi Send"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Message thread only. Empty name = off. Returns an error message or "".
    juce::String setDevice (int slot, const juce::String& deviceName);
    juce::String getDevice (int slot) const;
    jaja::DeviceSender& getSender (int slot) { return *senders[(size_t) slot]; }

    juce::AudioProcessorValueTreeState apvts;

private:
    void handleAsyncUpdate() override;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    std::array<std::unique_ptr<jaja::DeviceSender>, numSlots> senders;
    std::atomic<float>* onParam[numSlots] {};
    std::atomic<float>* gainParam[numSlots] {};
    std::atomic<float>* latencyParam = nullptr;

    juce::String deviceType;
    double hostRate = 48000.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JAJAMultiSendAudioProcessor)
};
