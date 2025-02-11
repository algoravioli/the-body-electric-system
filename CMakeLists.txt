cmake_minimum_required(VERSION 3.15)

project(AudioPlayer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set JUCE source directory (pointing to the submodule)
set(JUCE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/JUCE")
set(JUCE_MODULES_DIR "${JUCE_SOURCE_DIR}/modules")

# Set CMake module path to include JUCE's CMake scripts
list(APPEND CMAKE_MODULE_PATH "${JUCE_SOURCE_DIR}/extras/Build/CMake")

# Enable JUCE module mode
include("${JUCE_SOURCE_DIR}/extras/Build/CMake/JUCEUtils.cmake")

# Add JUCE as a subdirectory
add_subdirectory(${JUCE_SOURCE_DIR})

# Create the executable
add_executable(AudioPlayer main.cpp main.hpp)

# Link JUCE modules explicitly
target_link_libraries(AudioPlayer PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_audio_basics
        juce::juce_core
)

# Ensure JUCE module paths are correctly set
target_include_directories(AudioPlayer PRIVATE ${JUCE_MODULES_DIR})