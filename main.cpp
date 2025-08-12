#include <iostream>
#include <chrono>
#include <thread>
#include <array>
#include <vector>
#include <juce_audio_devices/juce_audio_devices.h>
#include "FileTransportSource.hpp"

// Wraps a stereo source and outputs to up to 4 channels with per-channel gain.
// Duplication scheme: out 0 <- L, out 1 <- R, out 2 <- L, out 3 <- R.
class QuadGainRouter final : public juce::AudioSource
{
public:
    QuadGainRouter(FileTransportSource& src, const std::array<float,4>& gainsIn)
        : source(src), gains(gainsIn) {}

    void prepareToPlay(int samplesPerBlockExpected, const double sampleRate) override
    {
        source.prepareToPlay(samplesPerBlockExpected, sampleRate);
        scratch.setSize(2, samplesPerBlockExpected, false, false, true);
        maxBlock = samplesPerBlockExpected;
    }

    void releaseResources() override
    {
        source.releaseResources();
        scratch.setSize(0, 0);
        maxBlock = 0;
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override
    {
        jassert(info.buffer != nullptr);
        const int n = info.numSamples;
        if (n > maxBlock) { info.clearActiveBufferRegion(); return; }

        scratch.clear();
        juce::AudioSourceChannelInfo scratchInfo(&scratch, 0, n);
        source.getNextAudioBlock(scratchInfo);

        const float* L = scratch.getReadPointer(0);
        const float* R = (scratch.getNumChannels() > 1 ? scratch.getReadPointer(1) : L);

        juce::AudioBuffer<float>* out = info.buffer;
        const int start = info.startSample;
        const int outChans = std::min(4, out->getNumChannels());

        for (int ch = 0; ch < out->getNumChannels(); ++ch)
            out->clear(ch, start, n);

        float* ch0 = outChans > 0 ? out->getWritePointer(0) + start : nullptr;
        float* ch1 = outChans > 1 ? out->getWritePointer(1) + start : nullptr;
        float* ch2 = outChans > 2 ? out->getWritePointer(2) + start : nullptr;
        float* ch3 = outChans > 3 ? out->getWritePointer(3) + start : nullptr;

        const float g0 = gains[0], g1 = gains[1], g2 = gains[2], g3 = gains[3];

        for (int i = 0; i < n; ++i)
        {
            const float l = L[i], r = R[i];
            if (ch0) ch0[i] = g0 * l;
            if (ch1) ch1[i] = g1 * r;
            if (ch2) ch2[i] = g2 * l;
            if (ch3) ch3[i] = g3 * r;
        }
    }

private:
    FileTransportSource&       source;
    std::array<float,4>        gains;
    juce::AudioBuffer<float>   scratch;
    int                        maxBlock = 0;
};

static inline bool parseGain(const char* s, float& out)
{
    try { out = static_cast<float>(std::stof(s)); return std::isfinite(out); }
    catch (...) { return false; }
}

static int printDeviceInfoAndExit()
{
    juce::AudioDeviceManager dm;

    // Open with 4 outs so we can see a realistic active mask for quad
    if (const auto err = dm.initialise(0, 4, nullptr, false); !err.isEmpty())
    {
        std::cerr << "Audio device error: " << err << "\n";
        return 1;
    }

    if (auto* dev = dm.getCurrentAudioDevice())
    {
        const auto name = dev->getName();
        const auto type = dev->getTypeName();
        const double sr = dev->getCurrentSampleRate();
        const int buf = dev->getCurrentBufferSizeSamples();

        const auto outNames = dev->getOutputChannelNames();
        const juce::BigInteger active = dev->getActiveOutputChannels();

        std::vector<int> activeIndices;
        for (int i = 0; i < outNames.size(); ++i)
            if (active[i]) activeIndices.push_back(i);

        std::cout << "Output device: " << name << " (" << type << ")\n"
                  << "Sample rate: " << sr << " Hz\n"
                  << "Buffer size: " << buf << " samples\n"
                  << "Available output channels (" << outNames.size() << "):\n";

        for (int i = 0; i < outNames.size(); ++i)
        {
            const bool isActive = active[i];
            std::cout << "  [" << i << "] " << outNames[(int)i].toStdString()
                      << (isActive ? "  *active" : "") << "\n";
        }

        std::cout << "Active output indices: ";
        if (activeIndices.empty()) std::cout << "(none)\n";
        else {
            for (size_t k = 0; k < activeIndices.size(); ++k)
            {
                std::cout << activeIndices[k] << (k + 1 < activeIndices.size() ? ", " : "\n");
            }
        }

        // Our logical mapping for playback (if >= 4 active outs)
        // Buffer channel 0..3 map to the first 4 active device outputs in ascending index order.
        std::cout << "Player mapping (logical -> device):\n";
        for (int logical = 0; logical < 4; ++logical)
        {
            if (static_cast<int>(activeIndices.size()) > logical)
                std::cout << "  ch" << logical << " -> device[" << activeIndices[logical] << "]\n";
            else
                std::cout << "  ch" << logical << " -> (not available)\n";
        }
    }
    else
    {
        std::cerr << "No current audio device.\n";
        return 1;
    }

    // Close politely
    dm.closeAudioDevice();
    return 0;
}

int main(const int argc, char* argv[])
{
    if (argc == 2 && std::string(argv[1]) == "--device-info")
        return printDeviceInfoAndExit();

    if (argc != 6 && argc != 2)
    {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << " <audio_file>\n"
                  << "  " << argv[0] << " <audio_file> <ch0_gain> <ch1_gain> <ch2_gain> <ch3_gain>\n"
                  << "  " << argv[0] << " --device-info\n";
        return 1;
    }

    std::array<float,4> gains {1.0f, 1.0f, 1.0f, 1.0f};
    if (argc == 6)
    {
        for (int i = 0; i < 4; ++i)
        {
            if (!parseGain(argv[2 + i], gains[i]))
            {
                std::cerr << "Invalid gain for channel " << i << "\n";
                return 1;
            }
        }
    }

    const juce::File file(argv[1]);
    FileTransportSource transport(file);
    if (!transport.isReady())
    {
        std::cerr << "Failed to open: " << file.getFullPathName() << "\n";
        return 1;
    }

    std::cout << "Playing: " << file.getFileName().toStdString()
              << " (" << transport.getDurationSeconds() << " s)\n"
              << "Gains: [" << gains[0] << ", " << gains[1] << ", "
              << gains[2] << ", " << gains[3] << "]\n";

    juce::AudioDeviceManager deviceManager;
    if (const auto err = deviceManager.initialise(0, 4, nullptr, false); !err.isEmpty())
    {
        std::cerr << "Audio device error: " << err << "\n";
        return 1;
    }

    if (const auto* dev = deviceManager.getCurrentAudioDevice())
    {
        if (const int outs = dev->getActiveOutputChannels().countNumberOfSetBits(); outs < 4)
            std::cerr << "Warning: device has " << outs << " active outputs; quad mix will be truncated.\n";
    }

    juce::AudioSourcePlayer player;
    QuadGainRouter router(transport, gains);
    player.setSource(&router);
    deviceManager.addAudioCallback(&player);

    transport.start();

    const double dur = transport.getDurationSeconds();
    const auto t0 = std::chrono::steady_clock::now();

    while (transport.isPlaying())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        if (const double t = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); t > dur + 0.5)
            break;
    }

    transport.stop();
    deviceManager.removeAudioCallback(&player);
    player.setSource(nullptr);
    deviceManager.closeAudioDevice();
    return 0;
}