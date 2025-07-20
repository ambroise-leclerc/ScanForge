# ScanForge

![Build Status](https://img.shields.io/badge/build-passing-brightgreen)
![C++23](https://img.shields.io/badge/C%2B%2B-23-blue)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS%20%7C%20Android-lightgrey)

ScanForge is a modern C++23 point cloud processing library designed for high-performance and cross-platform compatibility. It provides a clean, modern API for loading, processing, and converting point cloud data.

## Features

- 🚀 **Modern C++23** - Leverages the latest C++ features for performance and safety
- 🌐 **Cross-Platform** - Supports Windows, Linux, macOS, and Android
- 📊 **Point Cloud Support** - Currently supports PCD format (ASCII, Binary, Binary Compressed)
- 🛠️ **CLI Tool** - Command-line interface for point cloud analysis and conversion
- 🧪 **Comprehensive Testing** - Full unit test coverage
- 📚 **Well Documented** - Extensive documentation and examples

## Supported Formats

### Input Formats
- **PCD (Point Cloud Data)** - ASCII, Binary, Binary Compressed

### Output Formats  
- **PCD (Point Cloud Data)** - ASCII, Binary (Binary Compressed coming soon)

## Requirements

- **CMake** 3.28 or higher
- **C++23** compatible compiler:
  - GCC 12+ 
  - Clang 15+
  - MSVC 2022 (17.0+)
  - Android NDK r25+

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

# Install (optional)
cmake --install .
```

### Using the CLI Tool

```bash
# Show file information
./scanforge input.pcd --info

# Show detailed statistics
./scanforge input.pcd --stats --verbose

# Convert format
./scanforge input.pcd -o output.pcd -f binary

# Convert and show stats
./scanforge input.pcd -o output.pcd -f ascii --stats
```

### Using as a Library

```cpp
#include <scanforge/PCDLoader.hpp>
#include <scanforge/PointCloudTypes.hpp>

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
    
    std::pair<PCDHeader, PointCloudXYZRGB> loadPCD(const std::string& filename);
    bool isValidPCD(const std::string& filename);
};
```

### Point Types

- `Point3D` - Basic 3D point (x, y, z)
- `RGB` - Color information (r, g, b)
- `PointXYZRGB` - Point with position and color
- `PointCloudXYZ` - Point cloud with XYZ points
- `PointCloudXYZRGB` - Point cloud with colored points

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
