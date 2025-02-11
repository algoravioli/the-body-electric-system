#include "JUCE/modules/juce_audio_devices/juce_audio_devices.h"
#include "JUCE/modules/juce_audio_formats/juce_audio_formats.h"
#include <iostream>
#include <memory>
#include <thread>

class AudioPlayer : public juce::AudioSource {
public:
    explicit AudioPlayer(const juce::File& file) {
        if (!file.existsAsFile()) {
            std::cerr << "Error: File does not exist -> " << file.getFullPathName() << std::endl;
            return;
        }

        formatManager.registerBasicFormats();

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (!reader) {
            std::cerr << "Error: Failed to create AudioFormatReader for file: " << file.getFullPathName() << std::endl;
            return;
        }

        audioSource = std::make_unique<juce::AudioFormatReaderSource>(reader.release(), true);
        transportSource.setSource(audioSource.get(), 0, nullptr, audioSource->getAudioFormatReader()->sampleRate);
    }

    ~AudioPlayer() override {
        transportSource.stop();
        transportSource.setSource(nullptr);
        audioSource.reset();
    }

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override {
        transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override {
        transportSource.getNextAudioBlock(bufferToFill);
    }

    void releaseResources() override {
        transportSource.releaseResources();
    }

    void start() { transportSource.start(); }
    void stop() { transportSource.stop(); }

private:
    juce::AudioFormatManager formatManager;
    juce::AudioTransportSource transportSource;
    std::unique_ptr<juce::AudioFormatReaderSource> audioSource;
};

void playAudioFile(const juce::File& file, juce::AudioDeviceManager& deviceManager)
{
    if (!file.existsAsFile()) {
        std::cerr << "Error: File does not exist -> " << file.getFullPathName() << std::endl;
        return;
    }

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader) {
        std::cerr << "Error: Failed to create AudioFormatReader for file: "
                  << file.getFullPathName() << std::endl;
        return;
    }

    // Calculate duration (samples / sampleRate)
    double audioDuration = static_cast<double>(reader->lengthInSamples) / reader->sampleRate;
    std::cout << "Playing: " << file.getFileName().toStdString()
              << " (" << audioDuration << " seconds)" << std::endl;

    juce::AudioSourcePlayer player;
    std::unique_ptr<AudioPlayer> audioSource = std::make_unique<AudioPlayer>(file);

    deviceManager.initialise(0, 2, nullptr, false);
    player.setSource(audioSource.get());
    deviceManager.addAudioCallback(&player);

    audioSource->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(audioDuration * 1000)));

    // Cleanup
    audioSource->stop();
    player.setSource(nullptr);
    deviceManager.removeAudioCallback(&player);
    audioSource.reset();
}


