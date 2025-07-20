# ScanForge

![CI](https://github.com/ambroise-leclerc/ScanForge/workflows/CI/badge.svg)
![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux)
![Tests](https://img.shields.io/badge/tests-31%20passing-brightgreen)

ScanForge is a C++23 point cloud data loading/saving library. It provides a uniform interface for loading or saving point cloud data in various formats.

## Basic CLI Usage
```bash
# Show file information
./scanforge input.pcd --info

# Show detailed statistics
./scanforge input.pcd --stats

# Convert PCD to LAS format
./scanforge input.pcd -o output.las --format las

# Convert to different PCD variants
./scanforge input.pcd -o output_ascii.pcd --format pcd --variant ascii
./scanforge input.pcd -o output_binary.pcd --format pcd --variant binary
./scanforge input.pcd -o output_compressed.pcd --format pcd --variant compressed
```

## Requirements

- **CMake** 4.0 or higher (for full C++23 modules support)
- **C++23** compatible compiler with modules support:
  - GCC 14+ (for modules)
  - Clang 17+ (for modules) 
  - MSVC 2022 (17.8+, for modules)
  - Android NDK r26+


## Supported Formats

### Input Formats
- **PCD (Point Cloud Data)**
  - ASCII
  - Binary
  - Binary Compressed
- **LAS (LASer format)**
  - Binary

### Output Formats
- **PCD (Point Cloud Data)**
  - ASCII
  - Binary
  - Binary Compressed
- **LAS (LASer format)**
  - Binary

## Quick Start

### Building from Source

```bash
# Clone the repository
git clone https://github.com/ambroise-leclerc/ScanForge.git
cd ScanForge

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build .

# Run tests (optional)
ctest --verbose
```

### Platform-Specific Build Instructions

#### Windows (MSVC)
```cmd
# Ensure you have Visual Studio 2022 17.6+ installed
# Open Developer Command Prompt for VS 2022

git clone https://github.com/ambroise-leclerc/ScanForge.git
cd ScanForge
mkdir build && cd build

# Configure with MSVC
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl

# Build
cmake --build .
```

#### Linux (GCC 14+)
```bash
# Install GCC 14+ first
sudo apt update && sudo apt install -y gcc-14 g++-14

git clone https://github.com/ambroise-leclerc/ScanForge.git
cd ScanForge
mkdir build && cd build

# Configure with GCC 14
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-14 -DCMAKE_CXX_COMPILER=g++-14

# Build
cmake --build .
```

### Platform-Specific Build Instructions

### Windows (Visual Studio)

```bash
# Configure for Visual Studio 2022
cmake .. -G "Visual Studio 17 2022" -A x64

# Build
cmake --build . --config Release
```

### Linux

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt update
sudo apt install build-essential cmake

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### macOS

```bash
# Install dependencies (using Homebrew)
brew install cmake

# Configure and build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Android

```bash
# Set Android NDK path
export ANDROID_NDK=/path/to/android-ndk

# Configure for Android
cmake .. \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_NATIVE_API_LEVEL=24

# Build
cmake --build .
```

## CLI Usage

The ScanForge CLI tool provides an easy way to convert between different point cloud formats and analyze point cloud data.

### Basic Usage

```bash
# Show file information
./scanforge input.pcd --info

# Show detailed statistics
./scanforge input.pcd --stats

# Convert PCD to LAS format
./scanforge input.pcd -o output.las --format las

# Convert to different PCD variants
./scanforge input.pcd -o output_ascii.pcd --format pcd --variant ascii
./scanforge input.pcd -o output_binary.pcd --format pcd --variant binary
./scanforge input.pcd -o output_compressed.pcd --format pcd --variant compressed
```

### Command Line Options

- `input`: Input file path (required)
- `-o, --output`: Output file path
- `-f, --format`: Output format (`pcd` or `las`, default: `pcd`)
- `--variant`: PCD variant (`ascii`, `binary`, or `compressed`, default: `ascii`)
- `-i, --info`: Show file information
- `-s, --stats`: Show detailed statistics
- `-v, --verbose`: Enable verbose logging

### Examples

```bash
# Basic file analysis
./scanforge data/bunny.pcd --info --stats

# Convert LAS to ASCII PCD
./scanforge scan.las -o scan.pcd --format pcd --variant ascii

# Convert PCD to compressed format with verbose output
./scanforge input.pcd -o output.pcd --variant compressed --verbose
```

## Project Structure


```
ScanForge/
├── app/                    # CLI application
│   ├── main.cpp
│   └── CMakeLists.txt
├── src/                    # Core library
│   ├── LASLoader.hpp       # LAS file loader
│   ├── PCDLoader.hpp       # PCD file loader
│   ├── PointCloudTypes.hpp # Point cloud data structures
│   ├── codec/              # Compression codecs
│   │   └── LZFCodec.hpp    # LZF compression/decompression
│   ├── tooling/
│   │   └── Logger.hpp      # Logging utilities
│   └── CMakeLists.txt
├── tests/                  # Unit tests
│   ├── CMakeLists.txt
│   ├── data/               # Test data
│   │   ├── bunny.pcd
│   │   ├── milk_cartoon_all_small_clorox.pcd
│   │   ├── sample_compressed.pcd
│   │   └── sample.pcd
│   └── UnitTests/
│       ├── MainTest.cpp
│       ├── PointCloudTypesTest.cpp
│       └── LzfDecompressorTest.cpp
├── cmake/                  # CMake modules
│   ├── StandardProjectSettings.cmake
│   ├── CompilerWarnings.cmake
│   ├── Cache.cmake
│   ├── CPM.cmake
│   ├── Doxygen.cmake
│   └── ...
├── docs/                   # Documentation
│   ├── CMakeLists.txt
│   └── Doxyfile.in
├── CONTRIBUTING.md         # Contribution guidelines
└── CMakeLists.txt          # Main CMake file
```

## API Reference

### Core Classes

#### `PointCloud<T>`
Generic point cloud container supporting different point types.

```cpp
template<typename PointT>
class PointCloud {
public:
    std::vector<PointT> points;
    uint32_t width, height;
    bool is_dense;

    void push_back(const PointT& point);
    size_t size() const;
    bool empty() const;
    std::pair<Point3D, Point3D> getBoundingBox() const;
};
```

#### `PCDLoader`
Loads and saves PCD files with support for multiple formats.

```cpp
class PCDLoader {
public:
    struct PCDHeader { /* ... */ };

    std::pair<PCDHeader, PointCloudXYZRGB> loadPCD(const std::string& filename);
    bool savePCD_ASCII(const std::string& filename, const PCDHeader& header, const PointCloudXYZRGB& cloud);
    bool savePCD_Binary(const std::string& filename, const PCDHeader& header, const PointCloudXYZRGB& cloud);
    bool savePCD_BinaryCompressed(const std::string& filename, const PCDHeader& header, const PointCloudXYZRGB& cloud);
    bool isValidPCD(const std::string& filename);
};
```

#### `LASLoader`
Loads and saves LAS files.

```cpp
class LASLoader {
public:
    struct LASHeader { /* ... */ };

    std::pair<LASHeader, PointCloudXYZRGB> loadLAS(const std::string& filename);
    bool saveLAS(const std::string& filename, const LASHeader& header, const PointCloudXYZRGB& cloud);
    bool isValidLAS(const std::string& filename);
};
```

