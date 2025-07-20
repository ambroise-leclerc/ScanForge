# Test Data

This directory contains test datasets used by the ScanForge unit tests.

## Files

- `sample.pcd` - Binary compressed PCD file from Point Cloud Library (table_scene_lms400.pcd)
  - Source: https://raw.githubusercontent.com/PointCloudLibrary/data/master/tutorials/table_scene_lms400.pcd
  - Used by: "Real data processing pipeline" and "LZFDecompressor with real PCD data" tests
  - Format: Binary compressed with LZF compression
  - Points: Large dataset suitable for testing real-world scenarios

- `sample_compressed.pcd` - Minimal binary compressed PCD header for testing
  - Used by: Tests that only need to verify header parsing

## Adding New Test Data

When adding new test datasets:

1. Place files in `tests/data/` in the source directory
2. The CMake build system will automatically copy them to `build/tests/data/` during compilation
3. Tests run with working directory set to project root, so use relative paths like `tests/data/filename.pcd`
4. Update this README to document the new files

## Test Data Sources

- Point Cloud Library (PCL): https://github.com/PointCloudLibrary/data
- Open3D: https://www.open3d.org/docs/release/tutorial/data/index.html
