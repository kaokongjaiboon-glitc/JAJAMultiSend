#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include <cmath>
#include <memory>

namespace jaja
{
//==============================================================================
inline juce::String preferredTypeName (const juce::OwnedArray<juce::AudioIODeviceType>& types)
{
    for (auto* wanted : { "Windows Audio", "DirectSound", "CoreAudio", "ALSA" })
        for (auto* t : types)
            if (t->getTypeName() == wanted)
                return t->getTypeName();

    return types.isEmpty() ? juce::String() : types[0]->getTypeName();
}

struct OutputList
{
    juce::String typeName;
    juce::StringArray names;
};

inline OutputList listOutputDevices()
{
    OutputList result;
    juce::AudioDeviceManager manager;
    auto& types = manager.getAvailableDeviceTypes();
    result.typeName = preferredTypeName (types);

    for (auto* t : types)
    {
        if (t->getTypeName() == result.typeName)
        {
            t->scanForDevices();
            result.names = t->getDeviceNames (false); // false = outputs
        }
    }
    return result;
}

//==============================================================================
/** One output destination.
    The plugin's audio thread pushes copies of the master signal into a lock-free
    FIFO. The destination device's own audio thread pulls from the FIFO, resampling
    slightly (adaptive ratio) so that clock differences between Ableton's audio
    device and this device never make the buffer run dry or overflow. */
class DeviceSender : private juce::AudioIODeviceCallback
{
public:
    DeviceSender() : fifo (ringSize)
    {
        for (int c = 0; c < 2; ++c)
        {
            ring[c].calloc ((size_t) ringSize);
            scratch[c].calloc ((size_t) scratchSize);
        }
    }

    ~DeviceSender() override { close(); }

    //==========================================================================
    // Message thread
    juce::String open (const juce::String& typeName, const juce::String& deviceName)
    {
        close();
        lastError = {};

        if (deviceName.isEmpty())
            return {};

        auto m = std::make_unique<juce::AudioDeviceManager>();
        auto err = m->initialise (0, 2, nullptr, false);
        if (err.isNotEmpty())
            return lastError = err;

        m->setCurrentAudioDeviceType (typeName, true);

        auto setup = m->getAudioDeviceSetup();
        setup.inputDeviceName = {};
        setup.outputDeviceName = deviceName;
        setup.useDefaultInputChannels = false;
        setup.inputChannels.clear();
        setup.useDefaultOutputChannels = true;
        setup.sampleRate = 0.0;
        setup.bufferSize = 0;

        err = m->setAudioDeviceSetup (setup, false);
        if (err.isNotEmpty())
            return lastError = err;

        auto* device = m->getCurrentAudioDevice();
        if (device == nullptr)
            return lastError = "Cannot open device";

        deviceRate.store (device->getCurrentSampleRate());
        fifo.reset();
        primed = false;
        smoothFill = (double) target.load();
        interp[0].reset();
        interp[1].reset();

        manager = std::move (m);
        manager->addAudioCallback (this);
        running.store (true, std::memory_order_release);
        return {};
    }

    void close()
    {
        running.store (false, std::memory_order_release);

        if (manager != nullptr)
        {
            manager->removeAudioCallback (this);
            manager->closeAudioDevice();
            manager.reset();
        }
    }

    juce::String getStatus() const
    {
        if (running.load())
            return juce::String ((int) deviceRate.load()) + " Hz   buffer "
                   + juce::String (fillMs.load(), 0) + " ms";

        return lastError.isNotEmpty() ? "Error: " + lastError : juce::String ("Off");
    }

    bool isRunning() const noexcept { return running.load(); }

    //==========================================================================
    // Any thread
    void setHostRate (double sr) noexcept { hostRate.store (sr); }
    void setTarget (int samples) noexcept { target.store (samples); }

    // Audio thread (the plugin's processBlock)
    void push (const juce::AudioBuffer<float>& buffer, float gain) noexcept
    {
        if (! running.load (std::memory_order_acquire) || buffer.getNumChannels() == 0)
            return;

        const int n = buffer.getNumSamples();
        int s1, z1, s2, z2;
        fifo.prepareToWrite (n, s1, z1, s2, z2);

        if (z1 + z2 < n)
            return; // FIFO full: drop this block rather than block the host

        const int chans = buffer.getNumChannels();
        for (int c = 0; c < 2; ++c)
        {
            const float* src = buffer.getReadPointer (juce::jmin (c, chans - 1));
            juce::FloatVectorOperations::copyWithMultiply (ring[c].get() + s1, src, gain, z1);
            if (z2 > 0)
                juce::FloatVectorOperations::copyWithMultiply (ring[c].get() + s2, src + z1, gain, z2);
        }
        fifo.finishedWrite (z1 + z2);
    }

private:
    //==========================================================================
    void audioDeviceAboutToStart (juce::AudioIODevice* device) override
    {
        deviceRate.store (device->getCurrentSampleRate());
    }

    void audioDeviceStopped() override {}

    void audioDeviceIOCallbackWithContext (const float* const*, int, float* const* out, int numOut,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext&) override
    {
        for (int c = 0; c < numOut; ++c)
            if (out[c] != nullptr)
                juce::FloatVectorOperations::clear (out[c], numSamples);

        if (! running.load (std::memory_order_acquire) || numOut < 1)
            return;

        const double hr = hostRate.load();
        const double dr = deviceRate.load();
        const double baseRatio = dr > 0.0 ? hr / dr : 1.0;

        const int want0 = (int) std::ceil ((double) numSamples * baseRatio) + 4;
        const int tgt = juce::jmax (target.load(), want0 * 2, 256);
        const int avail = fifo.getNumReady();

        smoothFill += 0.05 * ((double) avail - smoothFill);
        fillMs.store ((float) (smoothFill * 1000.0 / juce::jmax (1.0, hr)));

        if (! primed)
        {
            if (avail < tgt)
                return;
            primed = true;
            smoothFill = (double) avail;
        }

        if (avail > tgt * 4) // fell far behind: jump forward
        {
            fifo.finishedRead (avail - tgt);
            return;
        }

        // If the FIFO is fuller than the target we consume slightly faster, and vice versa.
        const double err = (smoothFill - (double) tgt) / (double) tgt;
        const double ratio = baseRatio * (1.0 + juce::jlimit (-0.01, 0.01, err * 0.01));

        const int want = (int) std::ceil ((double) numSamples * ratio) + 4;
        if (want > scratchSize || avail < want)
        {
            primed = false; // underrun: output silence and refill
            return;
        }

        int s1, z1, s2, z2;
        fifo.prepareToRead (want, s1, z1, s2, z2);
        for (int c = 0; c < 2; ++c)
        {
            juce::FloatVectorOperations::copy (scratch[c].get(), ring[c].get() + s1, z1);
            if (z2 > 0)
                juce::FloatVectorOperations::copy (scratch[c].get() + z1, ring[c].get() + s2, z2);
        }

        int used = 0;
        for (int c = 0; c < juce::jmin (2, numOut); ++c)
            if (out[c] != nullptr)
                used = interp[c].process (ratio, scratch[c].get(), out[c], numSamples, want, 0);

        fifo.finishedRead (juce::jmin (used, want));
    }

    //==========================================================================
    static constexpr int ringSize = 1 << 17;   // ~2.7 s at 48 kHz
    static constexpr int scratchSize = 16384;

    juce::AbstractFifo fifo;
    juce::HeapBlock<float> ring[2], scratch[2];
    juce::LagrangeInterpolator interp[2];

    std::unique_ptr<juce::AudioDeviceManager> manager;
    std::atomic<bool> running { false };
    std::atomic<double> hostRate { 48000.0 }, deviceRate { 48000.0 };
    std::atomic<int> target { 2880 };
    std::atomic<float> fillMs { 0.0f };

    bool primed = false;       // device thread only
    double smoothFill = 0.0;   // device thread only
    juce::String lastError;    // message thread only

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DeviceSender)
};

} // namespace jaja
