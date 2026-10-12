#include "PluginEditor.h"

namespace
{
const juce::Colour bgColour { 0xff15151b };
const juce::Colour panelColour { 0xff1f1f28 };
const juce::Colour accent { 0xffb3001b }; // JAJA dark red

void styleSlider (juce::Slider& s, const juce::String& suffix)
{
    s.setSliderStyle (juce::Slider::LinearHorizontal);
    s.setTextBoxStyle (juce::Slider::TextBoxRight, false, 80, 22);
    s.setTextValueSuffix (suffix);
    s.setColour (juce::Slider::thumbColourId, accent);
    s.setColour (juce::Slider::trackColourId, accent.withAlpha (0.7f));
}
} // namespace

//==============================================================================
JAJAMultiSendAudioProcessorEditor::JAJAMultiSendAudioProcessorEditor (JAJAMultiSendAudioProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    for (int i = 0; i < JAJAMultiSendAudioProcessor::numSlots; ++i)
    {
        auto& s = slots[(size_t) i];

        s.title.setText ("Send " + juce::String (i + 1), juce::dontSendNotification);
        s.title.setColour (juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible (s.title);

        s.on.setColour (juce::ToggleButton::tickColourId, accent);
        s.on.setColour (juce::ToggleButton::textColourId, juce::Colours::white);
        addAndMakeVisible (s.on);
        s.onAtt = std::make_unique<Apvts::ButtonAttachment> (proc.apvts, "on" + juce::String (i), s.on);

        addAndMakeVisible (s.device);
        s.device.onChange = [this, i] { deviceChosen (i); };

        styleSlider (s.gain, " dB");
        addAndMakeVisible (s.gain);
        s.gainAtt = std::make_unique<Apvts::SliderAttachment> (proc.apvts, "gain" + juce::String (i), s.gain);

        s.status.setColour (juce::Label::textColourId, juce::Colours::grey);
        s.status.setFont (juce::Font (juce::FontOptions (12.0f)));
        addAndMakeVisible (s.status);
    }

    latencyLabel.setText ("Buffer", juce::dontSendNotification);
    latencyLabel.setColour (juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible (latencyLabel);

    styleSlider (latency, " ms");
    addAndMakeVisible (latency);
    latencyAtt = std::make_unique<Apvts::SliderAttachment> (proc.apvts, "latency", latency);

    refresh.onClick = [this] { refreshDeviceLists(); };
    addAndMakeVisible (refresh);

    hint.setText ("Pick a different device for each send (e.g. CABLE Input for TikTok, your headphones, ...). "
                  "Audio to the host is never changed.",
                  juce::dontSendNotification);
    hint.setColour (juce::Label::textColourId, juce::Colours::grey);
    hint.setFont (juce::Font (juce::FontOptions (12.0f)));
    addAndMakeVisible (hint);

    refreshDeviceLists();
    setSize (600, 360);
    startTimerHz (2);
}

JAJAMultiSendAudioProcessorEditor::~JAJAMultiSendAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void JAJAMultiSendAudioProcessorEditor::refreshDeviceLists()
{
    const auto list = jaja::listOutputDevices();

    for (int i = 0; i < JAJAMultiSendAudioProcessor::numSlots; ++i)
    {
        auto& box = slots[(size_t) i].device;
        const auto current = proc.getDevice (i);

        box.clear (juce::dontSendNotification);
        box.addItem ("(off)", 1);
        box.addItemList (list.names, 2);

        const int idx = current.isEmpty() ? -1 : list.names.indexOf (current);
        box.setSelectedId (idx >= 0 ? idx + 2 : 1, juce::dontSendNotification);
    }
}

void JAJAMultiSendAudioProcessorEditor::deviceChosen (int slot)
{
    auto& s = slots[(size_t) slot];
    const juce::String name = s.device.getSelectedId() <= 1 ? juce::String() : s.device.getText();

    const auto err = proc.setDevice (slot, name);
    s.status.setText (err.isNotEmpty() ? "Error: " + err : proc.getSender (slot).getStatus(),
                      juce::dontSendNotification);
}

void JAJAMultiSendAudioProcessorEditor::timerCallback()
{
    for (int i = 0; i < JAJAMultiSendAudioProcessor::numSlots; ++i)
        slots[(size_t) i].status.setText (proc.getSender (i).getStatus(), juce::dontSendNotification);
}

//==============================================================================
void JAJAMultiSendAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bgColour);

    g.setColour (juce::Colours::white);
    g.setFont (juce::Font (juce::FontOptions (22.0f, juce::Font::bold)));
    g.drawText ("JAJA Multi Send", 16, 8, 300, 32, juce::Justification::centredLeft);

    g.setColour (accent);
    g.fillRect (16, 42, 568, 2);

    g.setColour (panelColour);
    for (int i = 0; i < JAJAMultiSendAudioProcessor::numSlots; ++i)
        g.fillRoundedRectangle (12.0f, 54.0f + 80.0f * (float) i, 576.0f, 74.0f, 6.0f);
}

void JAJAMultiSendAudioProcessorEditor::resized()
{
    for (int i = 0; i < JAJAMultiSendAudioProcessor::numSlots; ++i)
    {
        auto& s = slots[(size_t) i];
        auto row = juce::Rectangle<int> (20, 58 + 80 * i, 560, 66);

        auto top = row.removeFromTop (28);
        s.title.setBounds (top.removeFromLeft (60));
        s.on.setBounds (top.removeFromLeft (60));
        s.device.setBounds (top.reduced (0, 2));

        row.removeFromTop (4);
        auto bottom = row.removeFromTop (28);
        s.gain.setBounds (bottom.removeFromLeft (300));
        s.status.setBounds (bottom.reduced (8, 0));
    }

    auto footer = juce::Rectangle<int> (20, 298, 560, 24);
    latencyLabel.setBounds (footer.removeFromLeft (60));
    refresh.setBounds (footer.removeFromRight (130));
    latency.setBounds (footer.reduced (4, 0));

    hint.setBounds (20, 326, 560, 24);
}
