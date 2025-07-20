#pragma once

#include "LZFCodec.hpp"
#include "PointCloudTypes.hpp"
#include "tooling/Logger.hpp"

#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#include <tuple>
#include <filesystem>
#include <span>
#include <ranges>
#include <optional>
#include <algorithm>

namespace scanforge {
using namespace tooling;
using Log = scanforge::tooling::Log;

/**
 * @brief PCD (Point Cloud Data) file loader supporting binary compressed format
 */
class PCDLoader {
 public:
  struct PCDHeader {
    std::string version;
    std::vector<std::string> fields;
    std::vector<uint32_t> sizes;
    std::vector<char> types;
    std::vector<uint32_t> counts;
    uint32_t width = 0;
    uint32_t height = 0;
    std::string viewpoint;
    uint32_t points = 0;
    std::string dataType;

    bool isValid() const { return !fields.empty() && width > 0 && points > 0; }

    bool hasXYZ() const {
      return std::find(fields.begin(), fields.end(), "x") != fields.end() && 
             std::find(fields.begin(), fields.end(), "y") != fields.end() &&
             std::find(fields.begin(), fields.end(), "z") != fields.end();
    }

    bool hasRGB() const { return std::find(fields.begin(), fields.end(), "rgb") != fields.end(); }

    size_t getFieldIndex(const std::string& field) const {
      auto it = std::find(fields.begin(), fields.end(), field);
      return (it != fields.end()) ? static_cast<size_t>(it - fields.begin()) : SIZE_MAX;
    }
  };

  /**
   * @brief Load point cloud from PCD file
   * @param filename Path to PCD file
   * @return Tuple of header and point cloud
   */
  std::tuple<PCDHeader, PointCloudXYZRGB> loadPCD(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
      Log::error("Failed to open file: {}", filename);
      return {PCDHeader{}, PointCloudXYZRGB{}};
    }

    PCDHeader header;
    if (!parseHeader(file, header)) {
      Log::error("Failed to parse header from file: {}", filename);
      return {PCDHeader{}, PointCloudXYZRGB{}};
    }

    if (!header.isValid() || !header.hasXYZ()) {
      Log::error("Invalid header or missing XYZ fields in file: {}", filename);
      return {header, PointCloudXYZRGB{}};
    }

    PointCloudXYZRGB pointCloud;
    pointCloud.points.reserve(header.points);
    pointCloud.width = header.width;
    pointCloud.height = header.height;

    if (header.dataType == "binary_compressed") {
      if (!loadBinaryCompressed(file, header, pointCloud)) {
        Log::error("Failed to load binary compressed data from file: {}", filename);
        return {header, PointCloudXYZRGB{}};
      }
    } else if (header.dataType == "binary") {
      if (!loadBinary(file, header, pointCloud)) {
        Log::error("Failed to load binary data from file: {}", filename);
        return {header, PointCloudXYZRGB{}};
      }
    } else if (header.dataType == "ascii") {
      if (!loadASCII(file, header, pointCloud)) {
        Log::error("Failed to load ASCII data from file: {}", filename);
        return {header, PointCloudXYZRGB{}};
      }
    } else {
      Log::error("Unsupported data type '{}' in file: {}", header.dataType, filename);
      return {header, PointCloudXYZRGB{}};
    }

    Log::debug("Successfully loaded {} points from file: {}", pointCloud.size(), filename);
    return {header, pointCloud};
  }

  /**
   * @brief Save point cloud to PCD file in ASCII format
   * @param filename Path to output PCD file
   * @param header PCD header information
   * @param pointCloud Point cloud data to save
   * @return True if save was successful, false otherwise
   */
  bool savePCD_ASCII(const std::string& filename, const PCDHeader& header, const PointCloudXYZRGB& pointCloud) {
    std::ofstream file(filename);
    if (!file.is_open()) {
      Log::error("Failed to create file: {}", filename);
      return false;
    }

    if (!writeHeader(file, header, "ascii")) {
      Log::error("Failed to write header to file: {}", filename);
      return false;
    }

    return writeASCII(file, header, pointCloud);
  }

  /**
   * @brief Save point cloud to PCD file in binary format
   * @param filename Path to output PCD file
   * @param header PCD header information
   * @param pointCloud Point cloud data to save
   * @return True if save was successful, false otherwise
   */
  bool savePCD_Binary(const std::string& filename, const PCDHeader& header, const PointCloudXYZRGB& pointCloud) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
      Log::error("Failed to create file: {}", filename);
      return false;
    }

    if (!writeHeader(file, header, "binary")) {
      Log::error("Failed to write header to file: {}", filename);
      return false;
    }

    return writeBinary(file, header, pointCloud);
  }

  /**
   * @brief Save point cloud to PCD file in binary compressed format
   * @param filename Path to output PCD file
   * @param header PCD header information
   * @param pointCloud Point cloud data to save
   * @return True if save was successful, false otherwise
   */
  bool savePCD_BinaryCompressed(const std::string& filename, const PCDHeader& header, const PointCloudXYZRGB& pointCloud) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
      Log::error("Failed to create file: {}", filename);
      return false;
    }

    if (!writeHeader(file, header, "binary_compressed")) {
      Log::error("Failed to write header to file: {}", filename);
      return false;
    }

    return writeBinaryCompressed(file, header, pointCloud);
  }

  /**
   * @brief Generic save method that automatically determines format from header
   * @param filename Path to output PCD file
   * @param header PCD header information (dataType field determines format)
   * @param pointCloud Point cloud data to save
   * @return True if save was successful, false otherwise
   */
  bool savePCD(const std::string& filename, const PCDHeader& header, const PointCloudXYZRGB& pointCloud) {
    if (header.dataType == "ascii") {
      return savePCD_ASCII(filename, header, pointCloud);
    } else if (header.dataType == "binary") {
      return savePCD_Binary(filename, header, pointCloud);
    } else if (header.dataType == "binary_compressed") {
      return savePCD_BinaryCompressed(filename, header, pointCloud);
    } else {
      Log::error("Unsupported data type '{}' for saving", header.dataType);
      return false;
    }
  }

  /**
   * @brief Create a standard PCD header for XYZRGB point clouds
   * @param pointCloud Point cloud to create header for
   * @param dataType Format type ("ascii", "binary", or "binary_compressed")
   * @return PCDHeader configured for the point cloud
   */
  static PCDHeader createXYZRGBHeader(const PointCloudXYZRGB& pointCloud, const std::string& dataType = "ascii") {
    PCDHeader header;
    header.version = "0.7";
    header.fields = {"x", "y", "z", "rgb"};
    header.sizes = {4, 4, 4, 4};  // float, float, float, uint32
    header.types = {'F', 'F', 'F', 'U'};  // Float, Float, Float, Unsigned
    header.counts = {1, 1, 1, 1};
    header.width = pointCloud.width > 0 ? pointCloud.width : static_cast<uint32_t>(pointCloud.points.size());
    header.height = pointCloud.height > 0 ? pointCloud.height : 1;
    header.viewpoint = "0 0 0 1 0 0 0";
    header.points = static_cast<uint32_t>(pointCloud.points.size());
    header.dataType = dataType;
    return header;
  }

 private:
  // Modern file I/O helper using RAII and C++23 features
  template<typename FileType>
  class FileHandle {
  public:
    FileHandle(const std::filesystem::path& path, std::ios_base::openmode mode = std::ios_base::in) 
      : file_(path, mode), path_(path) {
      if (!file_.is_open()) {
        Log::error("Failed to open file: {}", path_.string());
      }
    }

    ~FileHandle() = default;
    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;
    FileHandle(FileHandle&&) = default;
    FileHandle& operator=(FileHandle&&) = default;

    bool is_open() const { return file_.is_open(); }
    bool good() const { return file_.good(); }
    
    FileType& stream() { return file_; }
    const FileType& stream() const { return file_; }
    
    const std::filesystem::path& path() const { return path_; }

    // Modern read operations using spans
    template<typename T>
    bool read_binary(std::span<T> buffer) requires std::is_trivially_copyable_v<T> {
      if constexpr (std::is_same_v<FileType, std::ifstream>) {
        file_.read(reinterpret_cast<char*>(buffer.data()), buffer.size_bytes());
        return file_.good();
      } else {
        return false;
      }
    }

    // Modern write operations using spans  
    template<typename T>
    bool write_binary(std::span<const T> buffer) requires std::is_trivially_copyable_v<T> {
      if constexpr (std::is_same_v<FileType, std::ofstream>) {
        file_.write(reinterpret_cast<const char*>(buffer.data()), buffer.size_bytes());
        return file_.good();
      } else {
        return false;
      }
    }

    // Convenience methods for reading/writing single values
    template<typename T>
    bool read_value(T& value) requires std::is_trivially_copyable_v<T> {
      return read_binary(std::span{&value, 1});
    }

    template<typename T>
    bool write_value(const T& value) requires std::is_trivially_copyable_v<T> {
      return write_binary(std::span{&value, 1});
    }

  private:
    FileType file_;
    std::filesystem::path path_;
  };

  using InputFile = FileHandle<std::ifstream>;
  using OutputFile = FileHandle<std::ofstream>;

  bool parseHeader(std::ifstream& file, PCDHeader& header) {
    std::string line;

    while (std::getline(file, line)) {
      if (line.empty() || line[0] == '#')
        continue;

      std::istringstream iss(line);
      std::string key;
      iss >> key;

      if (key == "VERSION") {
        iss >> header.version;
      } else if (key == "FIELDS") {
        std::string field;
        while (iss >> field) {
          header.fields.push_back(field);
        }
      } else if (key == "SIZE") {
        uint32_t size;
        while (iss >> size) {
          header.sizes.push_back(size);
        }
      } else if (key == "TYPE") {
        char type;
        while (iss >> type) {
          header.types.push_back(type);
        }
      } else if (key == "COUNT") {
        uint32_t count;
        while (iss >> count) {
          header.counts.push_back(count);
        }
      } else if (key == "WIDTH") {
        iss >> header.width;
      } else if (key == "HEIGHT") {
        iss >> header.height;
      } else if (key == "VIEWPOINT") {
        std::getline(iss, header.viewpoint);
      } else if (key == "POINTS") {
        iss >> header.points;
      } else if (key == "DATA") {
        iss >> header.dataType;
        break;  // DATA is the last header line
      }
    }

    // Validate header consistency
    if (header.fields.size() != header.sizes.size() || 
        header.fields.size() != header.types.size() || 
        header.fields.size() != header.counts.size()) {
      Log::error("Header field count mismatch: fields={}, sizes={}, types={}, counts={}", 
                header.fields.size(), header.sizes.size(), header.types.size(), header.counts.size());
      return false;
    }

    return header.isValid();
  }

  bool loadBinaryCompressed(std::ifstream& file, const PCDHeader& header, PointCloudXYZRGB& pointCloud) {
    // Read compressed size using modern approach
    uint32_t compressedSize, uncompressedSize;
    
    // Create temporary wrapper for legacy ifstream
    auto readValue = [&file](auto& value) {
      file.read(reinterpret_cast<char*>(&value), sizeof(value));
      return file.good();
    };
    
    if (!readValue(compressedSize) || !readValue(uncompressedSize)) {
      Log::error("Failed to read compression header");
      return false;
    }

    std::vector<uint8_t> compressedData(compressedSize);
    file.read(reinterpret_cast<char*>(compressedData.data()), static_cast<std::streamsize>(compressedSize));

    if (file.fail()) {
      Log::error("Failed to read compressed data");
      return false;
    }

    auto uncompressedData = LZFCodec::decompress(compressedData, uncompressedSize);
    if (uncompressedData.empty()) {
      Log::error("Failed to decompress data");
      return false;
    }

    auto reorderedData = reorderFields(uncompressedData, header);
    if (reorderedData.empty()) {
      Log::error("Failed to reorder fields");
      return false;
    }

    return parseBinaryData(reorderedData, header, pointCloud);
  }

  bool loadBinary(std::ifstream& file, const PCDHeader& header, PointCloudXYZRGB& pointCloud) {
    // Calculate expected data size using ranges
    auto pointSize = std::ranges::fold_left(
      std::views::zip(header.sizes, header.counts),
      0uz,
      [](size_t sum, const auto& size_count) {
        const auto& [size, count] = size_count;
        return sum + size * count;
      }
    );

    size_t totalSize = pointSize * header.points;
    std::vector<uint8_t> binaryData(totalSize);

    file.read(reinterpret_cast<char*>(binaryData.data()), static_cast<std::streamsize>(totalSize));
    if (file.gcount() != static_cast<std::streamsize>(totalSize)) {
      Log::error("Failed to read expected amount of binary data");
      return false;
    }

    return parseBinaryData(binaryData, header, pointCloud);
  }

  bool loadASCII(std::ifstream& file, const PCDHeader& header, PointCloudXYZRGB& pointCloud) {
    std::string line;
    size_t xIdx = header.getFieldIndex("x");
    size_t yIdx = header.getFieldIndex("y");
    size_t zIdx = header.getFieldIndex("z");
    size_t rgbIdx = header.getFieldIndex("rgb");

    for (uint32_t i = 0; i < header.points && std::getline(file, line); ++i) {
      std::istringstream iss(line);
      std::vector<std::string> values;
      std::string value;

      while (iss >> value) {
        values.push_back(value);
      }

      if (values.size() < header.fields.size())
        continue;

      Point3D position(std::stof(values[xIdx]), std::stof(values[yIdx]), std::stof(values[zIdx]));

      RGB color(255, 255, 255);  // Default white
      if (rgbIdx != SIZE_MAX && rgbIdx < values.size()) {
        uint32_t rgbPacked = static_cast<uint32_t>(std::stoul(values[rgbIdx]));
        color = RGB(rgbPacked);
      }

      // Check for NaN/Inf values
      bool isValid = std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z);
      if (!isValid) {
        pointCloud.is_dense = false;
        continue;
      }

      pointCloud.push_back(PointXYZRGB(position, color));
    }

    return true;
  }

  bool parseBinaryData(const std::vector<uint8_t>& data, const PCDHeader& header, PointCloudXYZRGB& pointCloud) {
    // Find field indices using modern approach
    auto findFieldIndex = [&header](const std::string& name) -> std::optional<size_t> {
      auto it = std::ranges::find(header.fields, name);
      return it != header.fields.end() ? 
        std::optional{static_cast<size_t>(std::distance(header.fields.begin(), it))} : std::nullopt;
    };

    auto xIdx = findFieldIndex("x");
    auto yIdx = findFieldIndex("y"); 
    auto zIdx = findFieldIndex("z");
    auto rgbIdx = findFieldIndex("rgb");

    if (!xIdx || !yIdx || !zIdx) {
      Log::error("Missing required XYZ fields");
      return false;
    }

    // Calculate point size using ranges
    auto pointSize = std::ranges::fold_left(
      std::views::zip(header.sizes, header.counts),
      0uz,
      [](size_t sum, const auto& size_count) {
        const auto& [size, count] = size_count;
        return sum + size * count;
      }
    );

    for (uint32_t i = 0; i < header.points; ++i) {
      size_t offset = i * pointSize;
      if (offset + pointSize > data.size()) {
        Log::error("Data size mismatch");
        return false;
      }

      // Extract position using modern approach with span safety
      Point3D position{0.0f, 0.0f, 0.0f};
      size_t fieldOffset = 0;
      
      for (size_t j = 0; j < header.fields.size(); ++j) {
        if (j == *xIdx) {
          std::memcpy(&position.x, &data[offset + fieldOffset], sizeof(float));
        } else if (j == *yIdx) {
          std::memcpy(&position.y, &data[offset + fieldOffset], sizeof(float));
        } else if (j == *zIdx) {
          std::memcpy(&position.z, &data[offset + fieldOffset], sizeof(float));
        }
        fieldOffset += header.sizes[j] * header.counts[j];
      }

      // Extract color
      RGB color(255, 255, 255);  // Default white
      if (rgbIdx) {
        fieldOffset = 0;
        for (size_t j = 0; j <= *rgbIdx; ++j) {
          if (j == *rgbIdx) {
            uint32_t rgbPacked;
            std::memcpy(&rgbPacked, &data[offset + fieldOffset], sizeof(uint32_t));
            color = RGB(rgbPacked);
            break;
          }
          fieldOffset += header.sizes[j] * header.counts[j];
        }
      }

      // Check for NaN/Inf values
      bool isValid = std::isfinite(position.x) && std::isfinite(position.y) && std::isfinite(position.z);
      if (!isValid) {
        pointCloud.is_dense = false;
        continue;
      }

      pointCloud.push_back(PointXYZRGB(position, color));
    }

    return true;
  }

  std::vector<uint8_t> reorderFields(const std::vector<uint8_t>& data, const PCDHeader& /* header */) {
    // This is a simplified implementation
    // In a real PCD loader, you would need to handle the proper field reordering
    // based on the PCD specification for binary_compressed format
    
    if (data.empty()) {
      return {};
    }

    // For now, return the data as-is
    // TODO: Implement proper field reordering for binary_compressed format
    return data;
  }

  bool writeHeader(std::ofstream& file, const PCDHeader& header, const std::string& dataType) {
    file << "# .PCD v" << header.version << " - Point Cloud Data file format\n";
    file << "VERSION " << header.version << "\n";
    
    file << "FIELDS";
    for (const auto& field : header.fields) {
      file << " " << field;
    }
    file << "\n";
    
    file << "SIZE";
    for (const auto& size : header.sizes) {
      file << " " << size;
    }
    file << "\n";
    
    file << "TYPE";
    for (const auto& type : header.types) {
      file << " " << type;
    }
    file << "\n";
    
    file << "COUNT";
    for (const auto& count : header.counts) {
      file << " " << count;
    }
    file << "\n";
    
    file << "WIDTH " << header.width << "\n";
    file << "HEIGHT " << header.height << "\n";
    file << "VIEWPOINT " << header.viewpoint << "\n";
    file << "POINTS " << header.points << "\n";
    file << "DATA " << dataType << "\n";
    
    return file.good();
  }

  bool writeASCII(std::ofstream& file, const PCDHeader& header, const PointCloudXYZRGB& pointCloud) {
    size_t xIdx = header.getFieldIndex("x");
    size_t yIdx = header.getFieldIndex("y");
    size_t zIdx = header.getFieldIndex("z");
    size_t rgbIdx = header.getFieldIndex("rgb");

    if (xIdx == SIZE_MAX || yIdx == SIZE_MAX || zIdx == SIZE_MAX) {
      Log::error("Missing required XYZ fields in header");
      return false;
    }

    for (const auto& point : pointCloud.points) {
      // Write all fields in the order specified by the header
      for (size_t i = 0; i < header.fields.size(); ++i) {
        if (i > 0) file << " ";
        
        if (i == xIdx) {
          file << point.position.x;
        } else if (i == yIdx) {
          file << point.position.y;
        } else if (i == zIdx) {
          file << point.position.z;
        } else if (i == rgbIdx && rgbIdx != SIZE_MAX) {
          file << point.color.toPacked();
        } else {
          file << "0"; // Default value for unknown fields
        }
      }
      file << "\n";
    }
    
    return file.good();
  }

  bool writeBinary(std::ofstream& file, const PCDHeader& header, const PointCloudXYZRGB& pointCloud) {
    size_t xIdx = header.getFieldIndex("x");
    size_t yIdx = header.getFieldIndex("y");
    size_t zIdx = header.getFieldIndex("z");
    size_t rgbIdx = header.getFieldIndex("rgb");

    if (xIdx == SIZE_MAX || yIdx == SIZE_MAX || zIdx == SIZE_MAX) {
      Log::error("Missing required XYZ fields in header");
      return false;
    }

    for (const auto& point : pointCloud.points) {
      // Write all fields in the order specified by the header
      for (size_t i = 0; i < header.fields.size(); ++i) {
        if (i == xIdx) {
          file.write(reinterpret_cast<const char*>(&point.position.x), sizeof(float));
        } else if (i == yIdx) {
          file.write(reinterpret_cast<const char*>(&point.position.y), sizeof(float));
        } else if (i == zIdx) {
          file.write(reinterpret_cast<const char*>(&point.position.z), sizeof(float));
        } else if (i == rgbIdx && rgbIdx != SIZE_MAX) {
          uint32_t rgbPacked = point.color.toPacked();
          file.write(reinterpret_cast<const char*>(&rgbPacked), sizeof(uint32_t));
        } else {
          // Write default values for unknown fields based on their size
          std::vector<uint8_t> defaultValue(header.sizes[i], 0);
          file.write(reinterpret_cast<const char*>(defaultValue.data()), header.sizes[i]);
        }
      }
    }
    
    return file.good();
  }

  bool writeBinaryCompressed(std::ofstream& file, const PCDHeader& header, const PointCloudXYZRGB& pointCloud) {
    size_t xIdx = header.getFieldIndex("x");
    size_t yIdx = header.getFieldIndex("y");
    size_t zIdx = header.getFieldIndex("z");
    size_t rgbIdx = header.getFieldIndex("rgb");

    if (xIdx == SIZE_MAX || yIdx == SIZE_MAX || zIdx == SIZE_MAX) {
      Log::error("Missing required XYZ fields in header");
      return false;
    }

    // Generate binary data into a vector
    std::vector<uint8_t> uncompressedData;
    for (const auto& point : pointCloud.points) {
      for (size_t i = 0; i < header.fields.size(); ++i) {
        if (i == xIdx) {
          const auto* ptr = reinterpret_cast<const uint8_t*>(&point.position.x);
          uncompressedData.insert(uncompressedData.end(), ptr, ptr + sizeof(float));
        } else if (i == yIdx) {
          const auto* ptr = reinterpret_cast<const uint8_t*>(&point.position.y);
          uncompressedData.insert(uncompressedData.end(), ptr, ptr + sizeof(float));
        } else if (i == zIdx) {
          const auto* ptr = reinterpret_cast<const uint8_t*>(&point.position.z);
          uncompressedData.insert(uncompressedData.end(), ptr, ptr + sizeof(float));
        } else if (i == rgbIdx && rgbIdx != SIZE_MAX) {
          uint32_t rgbPacked = point.color.toPacked();
          const auto* ptr = reinterpret_cast<const uint8_t*>(&rgbPacked);
          uncompressedData.insert(uncompressedData.end(), ptr, ptr + sizeof(uint32_t));
        } else {
          // Add default values for unknown fields
          for (uint32_t j = 0; j < header.sizes[i]; ++j) {
            uncompressedData.push_back(0);
          }
        }
      }
    }

    // Compress the data
    auto compressedData = LZFCodec::compress(uncompressedData);
    if (compressedData.empty()) {
      Log::error("Failed to compress point cloud data");
      return false;
    }

    // Write compression header
    uint32_t compressedSize = static_cast<uint32_t>(compressedData.size());
    uint32_t uncompressedSize = static_cast<uint32_t>(uncompressedData.size());
    
    file.write(reinterpret_cast<const char*>(&compressedSize), sizeof(compressedSize));
    file.write(reinterpret_cast<const char*>(&uncompressedSize), sizeof(uncompressedSize));
    
    // Write compressed data
    file.write(reinterpret_cast<const char*>(compressedData.data()), static_cast<std::streamsize>(compressedData.size()));
    
    return file.good();
  }
};

}  // namespace scanforge
