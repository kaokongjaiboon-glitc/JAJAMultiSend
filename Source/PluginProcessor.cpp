#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout JAJAMultiSendAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < numSlots; ++i)
    {
        const auto n = juce::String (i + 1);

        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { "on" + juce::String (i), 1 }, "Send " + n + " On", false));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { "gain" + juce::String (i), 1 }, "Send " + n + " Gain",
            juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f, 2.5f), 0.0f));
    }

    layout.add (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "latency", 1 }, "Buffer (ms)", 20, 300, 60));

    return layout;
}

JAJAMultiSendAudioProcessor::JAJAMultiSendAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    for (int i = 0; i < numSlots; ++i)
    {
        senders[(size_t) i] = std::make_unique<jaja::DeviceSender>();
        onParam[i] = apvts.getRawParameterValue ("on" + juce::String (i));
        gainParam[i] = apvts.getRawParameterValue ("gain" + juce::String (i));
    }
    latencyParam = apvts.getRawParameterValue ("latency");
}

JAJAMultiSendAudioProcessor::~JAJAMultiSendAudioProcessor()
{
    cancelPendingUpdate();
    for (auto& s : senders)
        s->close();
}

//==============================================================================
void JAJAMultiSendAudioProcessor::prepareToPlay (double sampleRate, int)
{
    hostRate = sampleRate;
    for (auto& s : senders)
        s->setHostRate (sampleRate);
}

bool JAJAMultiSendAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void JAJAMultiSendAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // The audio itself is left untouched: it passes straight through to the host.
    // We only hand copies to the selected output devices.
    const int targetSamples = (int) (latencyParam->load() * 0.001 * hostRate);

    for (int i = 0; i < numSlots; ++i)
    {
        auto& sender = *senders[(size_t) i];
        sender.setTarget (targetSamples);

        if (onParam[i]->load() > 0.5f)
            sender.push (buffer, juce::Decibels::decibelsToGain (gainParam[i]->load()));
    }
}

//==============================================================================
juce::String JAJAMultiSendAudioProcessor::setDevice (int slot, const juce::String& deviceName)
{
    apvts.state.setProperty ("dev" + juce::String (slot), deviceName, nullptr);

    if (deviceName.isNotEmpty() && deviceType.isEmpty())
        deviceType = jaja::listOutputDevices().typeName;

    auto& sender = *senders[(size_t) slot];
    sender.setHostRate (hostRate);
    return sender.open (deviceType, deviceName);
}

juce::String JAJAMultiSendAudioProcessor::getDevice (int slot) const
{
    return apvts.state.getProperty ("dev" + juce::String (slot)).toString();
}

void JAJAMultiSendAudioProcessor::handleAsyncUpdate()
{
    for (int i = 0; i < numSlots; ++i)
        setDevice (i, getDevice (i));
}

//==============================================================================
juce::AudioProcessorEditor* JAJAMultiSendAudioProcessor::createEditor()
{
    return new JAJAMultiSendAudioProcessorEditor (*this);
}

void JAJAMultiSendAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void JAJAMultiSendAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            triggerAsyncUpdate(); // (re)open the saved devices on the message thread
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new JAJAMultiSendAudioProcessor();
}
