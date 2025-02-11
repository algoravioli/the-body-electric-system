#!/bin/zsh

# Exit on first error
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Function to check if a command exists
command_exists() {
    whence -p "$1" >/dev/null 2>&1
}

echo "${YELLOW}Checking dependencies...${NC}"

# Check if brew is installed (macOS)
if [[ "$OSTYPE" == "darwin"* ]]; then
    if ! command_exists brew; then
        echo "${RED}Homebrew is not installed. Installing Homebrew...${NC}"
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    else
        echo "${GREEN}Homebrew is installed${NC}"
    fi
fi

# Check if cmake is installed
if ! command_exists cmake; then
    echo "${RED}CMake is not installed${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "Installing CMake via Homebrew..."
        brew install cmake
    else
        echo "Please install CMake manually and try again"
        exit 1
    fi
else
    echo "${GREEN}CMake is installed${NC}"
fi

# Check cmake version
CMAKE_VERSION=$(cmake --version | head -n1 | cut -d" " -f3)
if (( $(echo "$CMAKE_VERSION 3.15" | awk '{print ($1 < $2)}') )); then
    echo "${RED}CMake version must be 3.15 or higher. Current version: $CMAKE_VERSION${NC}"
    exit 1
fi

# Check if git is installed
if ! command_exists git; then
    echo "${RED}Git is not installed${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo "Installing Git via Homebrew..."
        brew install git
    else
        echo "Please install Git manually and try again"
        exit 1
    fi
else
    echo "${GREEN}Git is installed${NC}"
fi

# Check if JUCE submodule is initialized
if [[ ! -f "JUCE/CMakeLists.txt" ]]; then
    echo "${YELLOW}Initializing JUCE submodule...${NC}"
    git submodule update --init --recursive
fi

# Create build directory and build in Release mode
echo "${YELLOW}Building project in Release mode...${NC}"
cmake -B build -DCMAKE_BUILD_TYPE=Release > /dev/null
cmake --build build

# Move AudioPlayer to repo root
if [[ -f "build/AudioPlayer" ]]; then
    echo "${YELLOW}Moving AudioPlayer to repository root...${NC}"
    mv build/AudioPlayer .
    echo "${GREEN}AudioPlayer has been moved to repository root${NC}"
else
    echo "${RED}AudioPlayer executable not found in build directory${NC}"
    exit 1
fi

echo "${GREEN}Build completed successfully!${NC}"
