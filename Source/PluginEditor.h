#pragma once

#include "PluginProcessor.h"
#include <array>

class JAJAMultiSendAudioProcessorEditor : public juce::AudioProcessorEditor,
                                          private juce::Timer
{
public:
    explicit JAJAMultiSendAudioProcessorEditor (JAJAMultiSendAudioProcessor&);
    ~JAJAMultiSendAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using Apvts = juce::AudioProcessorValueTreeState;

    struct SlotUI
    {
        juce::Label title;
        juce::ToggleButton on { "On" };
        juce::ComboBox device;
        juce::Slider gain;
        juce::Label status;
        std::unique_ptr<Apvts::ButtonAttachment> onAtt;
        std::unique_ptr<Apvts::SliderAttachment> gainAtt;
    };

    void timerCallback() override;
    void refreshDeviceLists();
    void deviceChosen (int slot);

    JAJAMultiSendAudioProcessor& proc;
    std::array<SlotUI, JAJAMultiSendAudioProcessor::numSlots> slots;

    juce::Label latencyLabel;
    juce::Slider latency;
    std::unique_ptr<Apvts::SliderAttachment> latencyAtt;
    juce::TextButton refresh { "Refresh devices" };
    juce::Label hint;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JAJAMultiSendAudioProcessorEditor)
};
