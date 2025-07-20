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

namespace scanforge {
using namespace tooling;

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
      LOG_ERROR("Failed to open file: {}", filename);
      return {PCDHeader{}, PointCloudXYZRGB{}};
    }

    PCDHeader header;
    if (!parseHeader(file, header)) {
      LOG_ERROR("Failed to parse header from file: {}", filename);
      return {PCDHeader{}, PointCloudXYZRGB{}};
    }

    if (!header.isValid() || !header.hasXYZ()) {
      LOG_ERROR("Invalid header or missing XYZ fields in file: {}", filename);
      return {header, PointCloudXYZRGB{}};
    }

    PointCloudXYZRGB pointCloud;
    pointCloud.points.reserve(header.points);
    pointCloud.width = header.width;
    pointCloud.height = header.height;

    if (header.dataType == "binary_compressed") {
      if (!loadBinaryCompressed(file, header, pointCloud)) {
        LOG_ERROR("Failed to load binary compressed data from file: {}", filename);
        return {header, PointCloudXYZRGB{}};
      }
    } else if (header.dataType == "binary") {
      if (!loadBinary(file, header, pointCloud)) {
        LOG_ERROR("Failed to load binary data from file: {}", filename);
        return {header, PointCloudXYZRGB{}};
      }
    } else if (header.dataType == "ascii") {
      if (!loadASCII(file, header, pointCloud)) {
        LOG_ERROR("Failed to load ASCII data from file: {}", filename);
        return {header, PointCloudXYZRGB{}};
      }
    } else {
      LOG_ERROR("Unsupported data type '{}' in file: {}", header.dataType, filename);
      return {header, PointCloudXYZRGB{}};
    }

    LOG_DEBUG("Successfully loaded {} points from file: {}", pointCloud.size(), filename);
    return {header, pointCloud};
  }

 private:
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
      LOG_ERROR("Header field count mismatch: fields={}, sizes={}, types={}, counts={}", 
                header.fields.size(), header.sizes.size(), header.types.size(), header.counts.size());
      return false;
    }

    return header.isValid();
  }

  bool loadBinaryCompressed(std::ifstream& file, const PCDHeader& header, PointCloudXYZRGB& pointCloud) {
    // Read compressed size
    uint32_t compressedSize, uncompressedSize;
    file.read(reinterpret_cast<char*>(&compressedSize), sizeof(compressedSize));
    file.read(reinterpret_cast<char*>(&uncompressedSize), sizeof(uncompressedSize));

    if (file.fail()) {
      LOG_ERROR("Failed to read compression header");
      return false;
    }

    std::vector<uint8_t> compressedData(compressedSize);
    file.read(reinterpret_cast<char*>(compressedData.data()), compressedSize);

    if (file.fail()) {
      LOG_ERROR("Failed to read compressed data");
      return false;
    }

    auto uncompressedData = LZFCodec::decompress(compressedData, uncompressedSize);
    if (uncompressedData.empty()) {
      LOG_ERROR("Failed to decompress data");
      return false;
    }

    auto reorderedData = reorderFields(uncompressedData, header);
    if (reorderedData.empty()) {
      LOG_ERROR("Failed to reorder fields");
      return false;
    }

    return parseBinaryData(reorderedData, header, pointCloud);
  }

  bool loadBinary(std::ifstream& file, const PCDHeader& header, PointCloudXYZRGB& pointCloud) {
    std::vector<char> binaryData;

    // Calculate expected data size
    size_t pointSize = 0;
    for (size_t i = 0; i < header.sizes.size(); ++i) {
      pointSize += header.sizes[i] * header.counts[i];
    }

    size_t totalSize = pointSize * header.points;
    binaryData.resize(totalSize);

    file.read(binaryData.data(), static_cast<std::streamsize>(totalSize));
    if (file.gcount() != static_cast<std::streamsize>(totalSize)) {
      LOG_ERROR("Failed to read expected amount of binary data");
      return false;
    }

    std::vector<uint8_t> data(binaryData.begin(), binaryData.end());
    return parseBinaryData(data, header, pointCloud);
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
    size_t xIdx = header.getFieldIndex("x");
    size_t yIdx = header.getFieldIndex("y");
    size_t zIdx = header.getFieldIndex("z");
    size_t rgbIdx = header.getFieldIndex("rgb");

    if (xIdx == SIZE_MAX || yIdx == SIZE_MAX || zIdx == SIZE_MAX) {
      LOG_ERROR("Missing required XYZ fields");
      return false;
    }

    size_t pointSize = 0;
    for (size_t i = 0; i < header.sizes.size(); ++i) {
      pointSize += header.sizes[i] * header.counts[i];
    }

    for (uint32_t i = 0; i < header.points; ++i) {
      size_t offset = i * pointSize;
      if (offset + pointSize > data.size()) {
        LOG_ERROR("Data size mismatch");
        return false;
      }

      // Extract position
      Point3D position;
      size_t fieldOffset = 0;
      
      for (size_t j = 0; j < header.fields.size(); ++j) {
        if (j == xIdx) {
          std::memcpy(&position.x, &data[offset + fieldOffset], sizeof(float));
        } else if (j == yIdx) {
          std::memcpy(&position.y, &data[offset + fieldOffset], sizeof(float));
        } else if (j == zIdx) {
          std::memcpy(&position.z, &data[offset + fieldOffset], sizeof(float));
        }
        fieldOffset += header.sizes[j] * header.counts[j];
      }

      // Extract color
      RGB color(255, 255, 255);  // Default white
      if (rgbIdx != SIZE_MAX) {
        fieldOffset = 0;
        for (size_t j = 0; j <= rgbIdx; ++j) {
          if (j == rgbIdx) {
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
};

}  // namespace scanforge
