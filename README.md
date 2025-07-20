# ScanForge

![B## Requirements

- **CMake** 4.0 or higher (for full C++23 modules support)
- **C++23** compatible compiler with modules support:
  - GCC 14+ (for modules)
  - Clang 17+ (for modules) 
  - MSVC 2022 (17.8+, for modules)
  - Android NDK r26+tus](https://img.shields.io/badge/build-passing-brightgreen)
![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS%20%7C%20Android-lightgrey)

ScanForge is point cloud data loading library. It provides a uniform interface for loading or saving point cloud data in various formats.



## Supported Formats

### Input Formats
- **PCD (Point Cloud Data)** - ASCII, Binary, Binary Compressed

### Output Formats  
- **PCD (Point Cloud Data)** - ASCII, Binary (Binary Compressed coming soon)

## Requirements

- **CMake** 4.0 or higher (for full C++23 modules support)
- **C++23** compatible compiler with modules support:
  - GCC 14+ (for modules)
  - Clang 17+ (for modules) 
  - **MSVC 2022 17.6+** (Visual Studio 2022 version 17.6 or later for C++23 support)
  - Android NDK r26+

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

## Project Structure

```
ScanForge/
├── app/                    # CLI application
│   ├── main.cpp
│   └── CMakeLists.txt
├── src/                    # Core library
│   ├── PCDLoader.hpp       # PCD file loader
│   ├── LZFDecompressor.hpp # LZF decompression
│   ├── PointCloudTypes.hpp # Point cloud data structures
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
│   └── ...
└── CMakeLists.txt         # Main CMake file

```cpp
#include "src/PCDLoader.hpp"
#include "src/PointCloudTypes.hpp"

using namespace scanforge;

int main() {
    PCDLoader loader;
    auto [header, cloud] = loader.loadPCD("input.pcd");
    
    if (header.isValid()) {
        std::cout << "Loaded " << cloud.size() << " points\n";
        // Get bounding box
        auto [min_pt, max_pt] = cloud.getBoundingBox();
        std::cout << "Bounding box: (" << min_pt.x << ", " << min_pt.y 
                  << ", " << min_pt.z << ") to (" << max_pt.x << ", " 
                  << max_pt.y << ", " << max_pt.z << ")\n";
    }
    return 0;
}
```

## Platform-Specific Build Instructions

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

## Project Structure

```
ScanForge/
├── app/                    # CLI application
│   ├── main.cpp
│   └── CMakeLists.txt
├── src/                    # Core library
│   ├── PCDLoader.hpp       # PCD file loader
│   ├── LZFDecompressor.hpp # LZF decompression
│   ├── PointCloudTypes.hpp # Point cloud data structures
│   ├── tooling/
│   │   └── Logger.hpp      # Logging utilities
│   └── CMakeLists.txt
├── tests/                  # Unit tests
│   └── unit_tests/
│       ├── test_main.cpp
│       ├── test_point_cloud_types.cpp
│       └── test_lzf_decompressor.cpp
├── cmake/                  # CMake modules
│   ├── StandardProjectSettings.cmake
│   ├── CompilerWarnings.cmake
│   └── ...
└── CMakeLists.txt         # Main CMake file
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
Loads PCD files with support for multiple formats.

```cpp
class PCDLoader {
public:
    struct PCDHeader { /* ... */ };

    std::pair<PCDHeader, PointCloud<PointXYZRGB>> loadPCD(const std::string& filename);
    bool isValidPCD(const std::string& filename);
};
```

### Point Types

- `Point3D` - Basic 3D point (x, y, z)
- `RGB` - Color information (r, g, b)
- `PointXYZRGB` - Point with position and color
- `PointCloud<Point3D>` - Point cloud with XYZ points
- `PointCloud<PointXYZRGB>` - Point cloud with colored points

## Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Roadmap

- [ ] **v1.1**: PLY format support
- [ ] **v1.2**: LAS/LAZ format support  
- [ ] **v1.3**: Advanced point cloud algorithms (filtering, segmentation)
- [ ] **v1.4**: GPU acceleration support
- [ ] **v2.0**: Real-time processing capabilities

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments

- LZF compression algorithm by Marc Lehmann
- Point Cloud Library (PCL) for inspiration
- Modern CMake practices from various community resources

## Support

- 📧 **Email**: [support@scanforge.dev](mailto:support@scanforge.dev)
- 🐛 **Issues**: [GitHub Issues](https://github.com/ambroise-leclerc/ScanForge/issues)
- 💬 **Discussions**: [GitHub Discussions](https://github.com/ambroise-leclerc/ScanForge/discussions)
