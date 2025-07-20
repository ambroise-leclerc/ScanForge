# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ScanForge is a modern C++23 point cloud processing library that provides a uniform interface for loading and saving point cloud data. The library is header-only and currently supports PCD (Point Cloud Data) format with ASCII, Binary, and Binary Compressed variants.

## Build System & Commands

### Primary Build Commands
```bash
# Standard build process
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .

# Run tests
ctest --verbose
# Or use the custom target:
cmake --build . --target run_tests

# Windows-specific build (MSVC)
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl

# Linux with specific GCC version
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-14 -DCMAKE_CXX_COMPILER=g++-14
```

### Test Execution
- Unit tests are built with Catch2 framework
- Test executable: `scanforgeUnitTests`
- Test data is automatically copied from `tests/data/` to build directory
- Individual tests are discoverable via Catch2's CTest integration

## Architecture & Code Structure

### Core Components

**Header-Only Library Design**: The library is interface-only (no .cpp files), implemented entirely in headers located in `src/`.

**Key Classes**:
- `PCDLoader` (`src/PCDLoader.hpp`): Main PCD file loader supporting multiple formats
- `PointCloud<T>` (`src/PointCloudTypes.hpp`): Generic point cloud container
- `Point3D`, `RGB`, `PointXYZRGB`: Point type definitions
- `LZFCodec` (`src/LZFCodec.hpp`): LZF compression/decompression for binary compressed PCD files
- `Logger` (`src/tooling/Logger.hpp`): Logging utilities

### Project Structure
```
src/                    # Core library headers
├── PCDLoader.hpp       # PCD file loading logic
├── PointCloudTypes.hpp # Point and cloud data structures
├── LZFCodec.hpp        # LZF compression support
└── tooling/
    └── Logger.hpp      # Logging utilities

app/                    # CLI application
├── main.cpp           # Command-line interface
└── CMakeLists.txt

tests/                  # Test suite
├── UnitTests/         # Catch2-based unit tests
│   ├── MainTest.cpp
│   ├── LzfDecompressorTest.cpp
│   └── PointCloudTypesTest.cpp
└── data/              # Test data files (PCD samples)
```

### Dependencies & Requirements

**Compiler Requirements**: C++23 with modules support required
- GCC 14+, Clang 17+, or MSVC 2022 (17.8+)
- CMake 4.0+ for full C++23 modules support

**External Dependencies**: 
- Catch2 (automatically fetched via CPM for tests)
- No runtime dependencies - header-only library

### Development Patterns

**Namespace**: All code lives in `scanforge` namespace, with `tooling` sub-namespace for utilities.

**Error Handling**: Uses return values and validation methods (e.g., `PCDHeader::isValid()`) rather than exceptions.

**Platform Support**: Cross-platform design with Windows, Linux, macOS, and Android support built into CMake configuration.

**CMake Modules**: Extensive use of custom CMake modules in `cmake/` directory for project configuration, warnings, sanitizers, and static analysis.

## Key APIs

### Loading Point Clouds
```cpp
PCDLoader loader;
auto [header, cloud] = loader.loadPCD("input.pcd");
if (header.isValid() && header.hasXYZ()) {
    // Process point cloud
}
```

### Point Cloud Operations
```cpp
PointCloud<PointXYZRGB> cloud;
auto [min_pt, max_pt] = cloud.getBoundingBox();
cloud.push_back(PointXYZRGB{...});
```

## Testing Strategy

- Unit tests use Catch2 v3.4.0 with automatic test discovery
- Test data includes various PCD file formats in `tests/data/`
- Tests are registered individually with CTest for granular execution
- Working directory set to project root for test data access