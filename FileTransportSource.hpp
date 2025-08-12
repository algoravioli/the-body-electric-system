#pragma once
#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_formats/juce_audio_formats.h>

class FileTransportSource final : public juce::AudioSource
{
public:
    explicit FileTransportSource(const juce::File& file)
        : thread("reader")
    {
        if (!file.existsAsFile())
            return;

        formatManager.registerBasicFormats();

        if (auto* r = formatManager.createReaderFor(file))
        {
            std::unique_ptr<juce::AudioFormatReader> reader(r);
            durationSeconds = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
            readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
            thread.startThread();
            transport.setSource(readerSource.get(), 32768, &thread, readerSource->getAudioFormatReader()->sampleRate);
        }
    }

    ~FileTransportSource() override
    {
        transport.stop();
        transport.setSource(nullptr);
        readerSource.reset();
        thread.stopThread(100);
    }

    FileTransportSource(const FileTransportSource&) = delete;
    FileTransportSource& operator=(const FileTransportSource&) = delete;

    bool   isReady()            const noexcept { return readerSource != nullptr; }
    double getDurationSeconds() const noexcept { return durationSeconds; }
    bool   isPlaying()          const noexcept { return transport.isPlaying(); }

    void start() noexcept { transport.start(); }
    void stop()  noexcept { transport.stop(); }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override
    {
        juce::ignoreUnused(sampleRate);
        transport.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& info) override
    {
        transport.getNextAudioBlock(info);
    }

    void releaseResources() override
    {
        transport.releaseResources();
    }

private:
    juce::AudioFormatManager formatManager;
    juce::TimeSliceThread    thread;
    juce::AudioTransportSource transport;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    double durationSeconds = 0.0;
};