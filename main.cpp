#include "main.hpp"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: " << argv[0] << " <audio_file>" << std::endl;
        return 1;
    }

    // Ensure the main thread is JUCE's message thread
    juce::MessageManager::getInstance()->setCurrentThreadAsMessageThread();

    // Initialize the JUCE Audio Device Manager correctly
    std::unique_ptr<juce::AudioDeviceManager> deviceManager = std::make_unique<juce::AudioDeviceManager>();

    // Remember to disable MIDI for some reason
    juce::String error = deviceManager->initialise(0, 2, nullptr, false);
    if (!error.isEmpty()) {
        std::cerr << "Audio device error: " << error << std::endl;
        return 1;
    }

    // Start the playback thread
    std::thread audioThread(playAudioFile, juce::File(argv[1]), std::ref(*deviceManager));

    // Wait for the playback thread to finish
    if (audioThread.joinable()) {
        audioThread.join();
    }

    // Explicitly remove callbacks and cleanup before exit
    deviceManager->closeAudioDevice();
    deviceManager.reset(); // Ensure full cleanup

    // FORCE JUCE TO SHUTDOWN SINGLETONS
    juce::DeletedAtShutdown::deleteAll();

    // FORCE MessageManager to fully delete
    juce::MessageManager::getInstance()->stopDispatchLoop();
    juce::MessageManager::deleteInstance();

    return 0;
}