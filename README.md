# The Body Electric System

## Prerequisites

- CMake (3.15 or higher)
- C++ compiler with C++17 support
- Git

## Building from Source

1. Clone the repository with submodules:
```bash
git clone --recurse-submodules https://github.com/algoravioli/the-body-electric-system.git
cd the-body-electric-system
```

2. Create and configure the build directory:
```bash
cmake -B build
```

3. Build the project:
```bash
cmake --build build
```

For development builds with debug symbols:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

For release builds with optimizations:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Project Structure

```
.
├── CMakeLists.txt      # Main CMake configuration
├── JUCE/               # JUCE framework (submodule)
├── main.cpp            # Main application source
└── main.hpp           # Main header file
```

## Quick Start

If in doubt, please run this shell script:
```bash
sh build.sh
```
