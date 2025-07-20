#pragma once

#include "PointCloudTypes.hpp"
#include "tooling/Logger.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <vector>

namespace scanforge {
using namespace tooling;
using Log = scanforge::tooling::Log;

/**
 * @brief LAS (LASer) file processor supporting LAS 1.2/1.3/1.4 formats
 * Handles both loading and saving of LAS files
 * Based on ASPRS LAS specification
 * Reference: https://www.asprs.org/divisions-committees/lidar-division/laser-las-file-format-exchange-activities
 */
class LASProcessor {
   public:
    // LAS Point Data Record Formats
    enum class PointFormat : uint8_t {
        FORMAT_0 = 0,   // x, y, z, intensity, return info, classification, scan angle, user data, point source ID
        FORMAT_1 = 1,   // Format 0 + GPS time
        FORMAT_2 = 2,   // Format 0 + RGB
        FORMAT_3 = 3,   // Format 1 + RGB
        FORMAT_4 = 4,   // Format 1 + wave packets
        FORMAT_5 = 5,   // Format 3 + wave packets
        FORMAT_6 = 6,   // LAS 1.4: x, y, z, intensity, return info, classification, scan angle, user data, point source ID, GPS time
        FORMAT_7 = 7,   // Format 6 + RGB
        FORMAT_8 = 8,   // Format 7 + NIR
        FORMAT_9 = 9,   // Format 6 + wave packets
        FORMAT_10 = 10  // Format 8 + wave packets
    };

    struct LASHeader {
        std::array<char, 4> fileSignature;  // "LASF"
        uint16_t fileSourceID;
        uint16_t globalEncoding;
        uint32_t projectID1, projectID2, projectID3, projectID4;  // GUID
        uint8_t versionMajor, versionMinor;
        std::array<char, 32> systemIdentifier;
        std::array<char, 32> generatingSoftware;
        uint16_t creationDayOfYear, creationYear;
        uint16_t headerSize;
        uint32_t offsetToPointData;
        uint32_t numberOfVariableLengthRecords;
        PointFormat pointDataRecordFormat;
        uint16_t pointDataRecordLength;
        uint32_t legacyNumberOfPointRecords;  // For LAS 1.2
        std::array<uint32_t, 5> legacyNumberOfPointsByReturn;
        double xScaleFactor, yScaleFactor, zScaleFactor;
        double xOffset, yOffset, zOffset;
        double maxX, minX, maxY, minY, maxZ, minZ;

        // LAS 1.3+ extended fields
        uint64_t startOfWaveformDataPacketRecord{0};           // LAS 1.3+
        uint64_t startOfFirstExtendedVariableLengthRecord{0};  // LAS 1.4+
        uint32_t numberOfExtendedVariableLengthRecords{0};     // LAS 1.4+
        uint64_t numberOfPointRecords{0};                      // LAS 1.4+
        std::array<uint64_t, 15> numberOfPointsByReturn{};     // LAS 1.4+

        // Additional fields for compatibility with PCD-style usage
        uint32_t width{0};
        uint32_t height{1};

        bool isValid() const { return std::string_view(fileSignature.data(), 4) == "LASF" && versionMajor == 1 && versionMinor >= 2; }

        std::string getVersion() const { return std::format("{}.{}", versionMajor, versionMinor); }

        uint64_t getTotalPointCount() const {
            // Use extended field for LAS 1.4+, legacy field for older versions
            return (versionMajor == 1 && versionMinor >= 4) ? numberOfPointRecords : legacyNumberOfPointRecords;
        }

        bool hasRGB() const {
            return pointDataRecordFormat == PointFormat::FORMAT_2 || pointDataRecordFormat == PointFormat::FORMAT_3 || pointDataRecordFormat == PointFormat::FORMAT_7 ||
                   pointDataRecordFormat == PointFormat::FORMAT_8 || pointDataRecordFormat == PointFormat::FORMAT_10;
        }

        bool hasGPSTime() const {
            return pointDataRecordFormat == PointFormat::FORMAT_1 || pointDataRecordFormat == PointFormat::FORMAT_3 || pointDataRecordFormat == PointFormat::FORMAT_4 ||
                   pointDataRecordFormat == PointFormat::FORMAT_5 || static_cast<uint8_t>(pointDataRecordFormat) >= 6;
        }
    };

    // Modern LAS point structure using C++23 features
    struct LASPoint {
        Point3D position;
        uint16_t intensity{0};
        uint8_t returnInfo{0};  // Return number and number of returns only
        uint8_t classification{0};
        int8_t scanAngle{0};
        uint8_t userData{0};
        uint16_t pointSourceID{0};

        // Additional LAS fields that are typically separate
        bool scanDirection{false};
        bool edgeOfFlightLine{false};
        double gpsTime{0.0};
        RGB color{255, 255, 255};  // Default white
        uint16_t nearInfrared{0};  // LAS 1.4 Format 8+

        // Convenience methods for LAS return info byte
        // Bits 0-3: Return Number (1-15)
        // Bits 4-7: Number of Returns (1-15)
        uint8_t getReturnNumber() const { return returnInfo & 0x0F; }
        uint8_t getNumberOfReturns() const { return (returnInfo >> 4) & 0x0F; }
        bool getScanDirection() const { return scanDirection; }
        bool getEdgeOfFlightLine() const { return edgeOfFlightLine; }

        void setReturnInfo(uint8_t returnNum, uint8_t numReturns, bool scanDir = false, bool edgeOfFlight = false) {
            returnInfo = (returnNum & 0x0F) | ((numReturns & 0x0F) << 4);
            scanDirection = scanDir;
            edgeOfFlightLine = edgeOfFlight;
        }
    };

    /**
     * @brief Load point cloud from LAS file
     * @param filename Path to LAS file
     * @return Tuple of header and point cloud
     */
    std::tuple<LASHeader, PointCloudXYZRGB> loadLAS(const std::filesystem::path& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            Log::error("Failed to open LAS file: {}", filename.string());
            return {LASHeader{}, PointCloudXYZRGB{}};
        }

        LASHeader header;
        if (!parseHeader(file, header)) {
            Log::error("Failed to parse LAS header from file: {}", filename.string());
            return {LASHeader{}, PointCloudXYZRGB{}};
        }

        if (!header.isValid()) {
            Log::error("Invalid LAS header in file: {}", filename.string());
            return {header, PointCloudXYZRGB{}};
        }

        PointCloudXYZRGB pointCloud;
        if (!loadPointData(file, header, pointCloud)) {
            Log::error("Failed to load point data from LAS file: {}", filename.string());
            return {header, PointCloudXYZRGB{}};
        }

        Log::debug("Successfully loaded {} points from LAS file: {}", pointCloud.size(), filename.string());
        return {header, pointCloud};
    }

    /**
     * @brief Save point cloud to LAS file
     * @param filename Path to output LAS file
     * @param header LAS header information
     * @param pointCloud Point cloud data to save
     * @return True if save was successful, false otherwise
     */
    bool saveLAS(const std::filesystem::path& filename, const LASHeader& header, const PointCloudXYZRGB& pointCloud) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            Log::error("Failed to create LAS file: {}", filename.string());
            return false;
        }

        if (!writeHeader(file, header)) {
            Log::error("Failed to write LAS header to file: {}", filename.string());
            return false;
        }

        if (!writePointData(file, header, pointCloud)) {
            Log::error("Failed to write point data to LAS file: {}", filename.string());
            return false;
        }

        return true;
    }

    /**
     * @brief Create a standard LAS header for point clouds
     * @param pointCloud Point cloud to create header for
     * @param format Point data record format
     * @return LASHeader configured for the point cloud
     */
    static LASHeader createLASHeader(const PointCloudXYZRGB& pointCloud, PointFormat format = PointFormat::FORMAT_3) {
        LASHeader header{};

        // File signature
        header.fileSignature = {'L', 'A', 'S', 'F'};

        // Version
        header.versionMajor = 1;
        header.versionMinor = 3;  // Use LAS 1.3 by default

        // Basic info
        header.fileSourceID = 0;
        header.globalEncoding = 0;
        header.headerSize = 235;  // Standard LAS 1.3 header size
        header.offsetToPointData = header.headerSize;
        header.numberOfVariableLengthRecords = 0;
        header.pointDataRecordFormat = format;

        // Point record length based on format
        header.pointDataRecordLength = getPointRecordLength(format);
        header.legacyNumberOfPointRecords = static_cast<uint32_t>(pointCloud.size());

        // Set width/height for compatibility
        header.width = static_cast<uint32_t>(pointCloud.size());
        header.height = 1;

        // Calculate bounding box and scale factors
        if (!pointCloud.empty()) {
            auto [minPt, maxPt] = pointCloud.getBoundingBox();

            header.minX = minPt.x;
            header.maxX = maxPt.x;
            header.minY = minPt.y;
            header.maxY = maxPt.y;
            header.minZ = minPt.z;
            header.maxZ = maxPt.z;

            // Use appropriate scale factors (typically 0.01 for meter precision)
            header.xScaleFactor = header.yScaleFactor = header.zScaleFactor = 0.01;
            header.xOffset = header.yOffset = header.zOffset = 0.0;
        }

        // Software identifier
        std::string software = "ScanForge v1.0.0";
        std::ranges::copy(software | std::views::take(31), header.generatingSoftware.begin());

        // Creation date (current date)
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto tm = *std::localtime(&time_t);
        header.creationYear = static_cast<uint16_t>(tm.tm_year + 1900);
        header.creationDayOfYear = static_cast<uint16_t>(tm.tm_yday + 1);

        return header;
    }

    /**
     * @brief Get the point record length for a given LAS format
     * @param format LAS point data record format
     * @return Length in bytes
     */
    static uint16_t getPointRecordLength(PointFormat format) {
        switch (format) {
            case PointFormat::FORMAT_0:
                return 20;
            case PointFormat::FORMAT_1:
                return 28;
            case PointFormat::FORMAT_2:
                return 26;
            case PointFormat::FORMAT_3:
                return 34;
            case PointFormat::FORMAT_4:
                return 57;
            case PointFormat::FORMAT_5:
                return 63;
            case PointFormat::FORMAT_6:
                return 30;
            case PointFormat::FORMAT_7:
                return 36;
            case PointFormat::FORMAT_8:
                return 38;
            case PointFormat::FORMAT_9:
                return 59;
            case PointFormat::FORMAT_10:
                return 67;
            default:
                return 20;
        }
    }

   private:
    // Modern file I/O helper using RAII and C++23 features
    template <typename T>
    bool readBinary(std::ifstream& file, T& value)
        requires std::is_trivially_copyable_v<T>
    {
        file.read(reinterpret_cast<char*>(&value), sizeof(T));
        return file.good();
    }

    template <typename T>
    bool writeBinary(std::ofstream& file, const T& value)
        requires std::is_trivially_copyable_v<T>
    {
        file.write(reinterpret_cast<const char*>(&value), sizeof(T));
        return file.good();
    }

    bool parseHeader(std::ifstream& file, LASHeader& header) {
        // Read file signature
        if (!readBinary(file, header.fileSignature))
            return false;

        // Validate signature
        if (std::string_view(header.fileSignature.data(), 4) != "LASF") {
            Log::error("Invalid LAS file signature");
            return false;
        }

        // Read basic header fields
        if (!readBinary(file, header.fileSourceID) || !readBinary(file, header.globalEncoding) || !readBinary(file, header.projectID1) || !readBinary(file, header.projectID2) ||
            !readBinary(file, header.projectID3) || !readBinary(file, header.projectID4) || !readBinary(file, header.versionMajor) || !readBinary(file, header.versionMinor)) {
            return false;
        }

        // Read system identifier and generating software
        file.read(reinterpret_cast<char*>(header.systemIdentifier.data()), 32);
        file.read(reinterpret_cast<char*>(header.generatingSoftware.data()), 32);

        // Read creation date and header info
        if (!readBinary(file, header.creationDayOfYear) || !readBinary(file, header.creationYear) || !readBinary(file, header.headerSize) || !readBinary(file, header.offsetToPointData) ||
            !readBinary(file, header.numberOfVariableLengthRecords)) {
            return false;
        }

        // Read point data format info
        uint8_t formatByte;
        if (!readBinary(file, formatByte))
            return false;
        header.pointDataRecordFormat = static_cast<PointFormat>(formatByte);

        if (!readBinary(file, header.pointDataRecordLength) || !readBinary(file, header.legacyNumberOfPointRecords)) {
            return false;
        }

        // Read legacy number of points by return
        for (auto& count : header.legacyNumberOfPointsByReturn) {
            if (!readBinary(file, count))
                return false;
        }

        // Read scale factors and offsets
        if (!readBinary(file, header.xScaleFactor) || !readBinary(file, header.yScaleFactor) || !readBinary(file, header.zScaleFactor) || !readBinary(file, header.xOffset) ||
            !readBinary(file, header.yOffset) || !readBinary(file, header.zOffset)) {
            return false;
        }

        // Read bounding box
        if (!readBinary(file, header.maxX) || !readBinary(file, header.minX) || !readBinary(file, header.maxY) || !readBinary(file, header.minY) || !readBinary(file, header.maxZ) ||
            !readBinary(file, header.minZ)) {
            return false;
        }

        // Read extended fields for LAS 1.3+
        if (header.versionMajor == 1 && header.versionMinor >= 3) {
            if (!readBinary(file, header.startOfWaveformDataPacketRecord)) {
                return false;
            }
        }

        // Read LAS 1.4+ extended fields
        if (header.versionMajor == 1 && header.versionMinor >= 4) {
            if (!readBinary(file, header.startOfFirstExtendedVariableLengthRecord) || !readBinary(file, header.numberOfExtendedVariableLengthRecords) ||
                !readBinary(file, header.numberOfPointRecords)) {
                return false;
            }

            for (auto& count : header.numberOfPointsByReturn) {
                if (!readBinary(file, count))
                    return false;
            }
        }

        return true;
    }

    bool loadPointData(std::ifstream& file, const LASHeader& header, PointCloudXYZRGB& pointCloud) {
        // Seek to point data
        file.seekg(header.offsetToPointData);
        if (file.fail()) {
            Log::error("Failed to seek to point data section");
            return false;
        }

        uint64_t numPoints = header.getTotalPointCount();
        pointCloud.points.reserve(numPoints);
        pointCloud.width = static_cast<uint32_t>(numPoints);
        pointCloud.height = 1;
        pointCloud.is_dense = true;

        // Read points based on format
        for (uint64_t i = 0; i < numPoints; ++i) {
            LASPoint lasPoint;
            if (!readPointRecord(file, header, lasPoint)) {
                Log::error("Failed to read point {} of {}", i, numPoints);
                return false;
            }

            // Convert to PointXYZRGB
            PointXYZRGB point(lasPoint.position, lasPoint.color);
            pointCloud.push_back(point);
        }

        return true;
    }

    bool readPointRecord(std::ifstream& file, const LASHeader& header, LASPoint& point) {
        // Read base point data (common to all formats)
        int32_t x, y, z;
        if (!readBinary(file, x) || !readBinary(file, y) || !readBinary(file, z)) {
            return false;
        }

        // Apply scale and offset
        point.position.x = static_cast<float>(x * header.xScaleFactor + header.xOffset);
        point.position.y = static_cast<float>(y * header.yScaleFactor + header.yOffset);
        point.position.z = static_cast<float>(z * header.zScaleFactor + header.zOffset);

        // Read intensity, return info, classification, etc.
        if (!readBinary(file, point.intensity) || !readBinary(file, point.returnInfo) || !readBinary(file, point.classification) || !readBinary(file, point.scanAngle) ||
            !readBinary(file, point.userData) || !readBinary(file, point.pointSourceID)) {
            return false;
        }

        // Read GPS time if present
        if (header.hasGPSTime()) {
            if (!readBinary(file, point.gpsTime))
                return false;
        }

        // Read RGB if present
        if (header.hasRGB()) {
            uint16_t r, g, b;
            if (!readBinary(file, r) || !readBinary(file, g) || !readBinary(file, b)) {
                return false;
            }
            // Convert from 16-bit to 8-bit
            point.color = RGB(static_cast<uint8_t>(r >> 8), static_cast<uint8_t>(g >> 8), static_cast<uint8_t>(b >> 8));
        }

        // Read NIR if present (Format 8 and 10)
        if (static_cast<uint8_t>(header.pointDataRecordFormat) == 8 || static_cast<uint8_t>(header.pointDataRecordFormat) == 10) {
            if (!readBinary(file, point.nearInfrared))
                return false;
        }

        return true;
    }

    bool writeHeader(std::ofstream& file, const LASHeader& header) {
        // Write all header fields in order
        if (!writeBinary(file, header.fileSignature) || !writeBinary(file, header.fileSourceID) || !writeBinary(file, header.globalEncoding) || !writeBinary(file, header.projectID1) ||
            !writeBinary(file, header.projectID2) || !writeBinary(file, header.projectID3) || !writeBinary(file, header.projectID4) || !writeBinary(file, header.versionMajor) ||
            !writeBinary(file, header.versionMinor)) {
            return false;
        }

        // Write system identifier and generating software (32 bytes each)
        file.write(reinterpret_cast<const char*>(header.systemIdentifier.data()), 32);
        file.write(reinterpret_cast<const char*>(header.generatingSoftware.data()), 32);

        // Continue with remaining header fields...
        if (!writeBinary(file, header.creationDayOfYear) || !writeBinary(file, header.creationYear) || !writeBinary(file, header.headerSize) || !writeBinary(file, header.offsetToPointData) ||
            !writeBinary(file, header.numberOfVariableLengthRecords) || !writeBinary(file, static_cast<uint8_t>(header.pointDataRecordFormat)) || !writeBinary(file, header.pointDataRecordLength) ||
            !writeBinary(file, header.legacyNumberOfPointRecords)) {
            return false;
        }

        // Write legacy number of points by return
        for (const auto& count : header.legacyNumberOfPointsByReturn) {
            if (!writeBinary(file, count))
                return false;
        }

        // Write scale factors, offsets, and bounding box
        if (!writeBinary(file, header.xScaleFactor) || !writeBinary(file, header.yScaleFactor) || !writeBinary(file, header.zScaleFactor) || !writeBinary(file, header.xOffset) ||
            !writeBinary(file, header.yOffset) || !writeBinary(file, header.zOffset) || !writeBinary(file, header.maxX) || !writeBinary(file, header.minX) || !writeBinary(file, header.maxY) ||
            !writeBinary(file, header.minY) || !writeBinary(file, header.maxZ) || !writeBinary(file, header.minZ)) {
            return false;
        }

        // Write extended fields for LAS 1.3+
        if (header.versionMajor == 1 && header.versionMinor >= 3) {
            if (!writeBinary(file, header.startOfWaveformDataPacketRecord)) {
                return false;
            }
        }

        return true;
    }

    bool writePointData(std::ofstream& file, const LASHeader& header, const PointCloudXYZRGB& pointCloud) {
        for (const auto& point : pointCloud.points) {
            if (!writePointRecord(file, header, point)) {
                return false;
            }
        }
        return true;
    }

    bool writePointRecord(std::ofstream& file, const LASHeader& header, const PointXYZRGB& point) {
        // Convert coordinates to scaled integers
        int32_t x = static_cast<int32_t>((static_cast<double>(point.position.x) - header.xOffset) / header.xScaleFactor);
        int32_t y = static_cast<int32_t>((static_cast<double>(point.position.y) - header.yOffset) / header.yScaleFactor);
        int32_t z = static_cast<int32_t>((static_cast<double>(point.position.z) - header.zOffset) / header.zScaleFactor);

        // Write base point data
        if (!writeBinary(file, x) || !writeBinary(file, y) || !writeBinary(file, z)) {
            return false;
        }

        // Write default values for other fields
        uint16_t intensity = 0;
        uint8_t returnInfo = 0x11;   // Return 1 of 1
        uint8_t classification = 1;  // Unclassified
        int8_t scanAngle = 0;
        uint8_t userData = 0;
        uint16_t pointSourceID = 0;

        if (!writeBinary(file, intensity) || !writeBinary(file, returnInfo) || !writeBinary(file, classification) || !writeBinary(file, scanAngle) || !writeBinary(file, userData) ||
            !writeBinary(file, pointSourceID)) {
            return false;
        }

        // Write GPS time if required
        if (header.hasGPSTime()) {
            double gpsTime = 0.0;
            if (!writeBinary(file, gpsTime))
                return false;
        }

        // Write RGB if required
        if (header.hasRGB()) {
            // Convert from 8-bit to 16-bit
            uint16_t r = static_cast<uint16_t>(point.color.r) << 8;
            uint16_t g = static_cast<uint16_t>(point.color.g) << 8;
            uint16_t b = static_cast<uint16_t>(point.color.b) << 8;

            if (!writeBinary(file, r) || !writeBinary(file, g) || !writeBinary(file, b)) {
                return false;
            }
        }

        return true;
    }
};

}  // namespace scanforge