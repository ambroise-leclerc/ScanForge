#include "LASProcessor.hpp"
#include "PCDProcessor.hpp"
#include "PointCloudTypes.hpp"
#include "tooling/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <print>
#include <string>

#include <CLI/CLI.hpp>

namespace fs = std::filesystem;
using namespace scanforge;
using namespace scanforge::tooling;

/**
 * @brief Detect file format based on extension
 * @param filename File path to analyze
 * @return String indicating format ("pcd", "las", or "unknown")
 */
std::string detectFileFormat(const std::string& filename) {
    fs::path filePath(filename);
    std::string extension = filePath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), 
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (extension == ".pcd") {
        return "pcd";
    } else if (extension == ".las") {
        return "las";
    } else {
        return "unknown";
    }
}

/**
 * @brief Configuration structure for the application
 */
struct AppConfig {
    std::string inputFile;
    std::string outputFile;
    std::string outputFormat = "pcd";
    std::string pcdVariant = "ascii";
    bool showInfo = false;
    bool showStats = false;
    bool verbose = false;
};

/**
 * @brief Print file information from LAS header
 * @param header The LAS header containing file metadata
 * @param filename The name of the file being analyzed
 */
void printFileInfo(const LASProcessor::LASHeader& header, const std::string& filename) {
    std::println(R"(
File Information for: {}
========================
Version:      {}
Points:       {}
Dimensions:   {} x {}
Point Format: {}
Has XYZ:      Yes
Has RGB:      {}
Has GPS Time: {}
Bounding Box: ({:.3f}, {:.3f}, {:.3f}) to ({:.3f}, {:.3f}, {:.3f})
Scale Factor: ({:.6f}, {:.6f}, {:.6f})
Software:     {}
)",
                 filename, header.getVersion(), header.getTotalPointCount(), header.width, header.height, static_cast<int>(header.pointDataRecordFormat), header.hasRGB() ? "Yes" : "No",
                 header.hasGPSTime() ? "Yes" : "No", header.minX, header.minY, header.minZ, header.maxX, header.maxY, header.maxZ, header.xScaleFactor, header.yScaleFactor, header.zScaleFactor,
                 std::string(header.generatingSoftware.begin(), header.generatingSoftware.end()));
}

/**
 * @brief Print file information from PCD header
 * @param header The PCD header containing file metadata
 * @param filename The name of the file being analyzed
 */
void printFileInfo(const PCDProcessor::PCDHeader& header, const std::string& filename) {
    std::println(
        R"(
File Information for: {}
========================
Version:    {}
Points:     {}
Dimensions: {} x {}
Data Type:  {}
Fields:     {}
Viewpoint:  {}
Has XYZ:    {}
Has RGB:    {}
)",
        filename, header.version, header.points, header.width, header.height, header.dataType,
        [&]() {
            std::string fields;
            for (size_t i = 0; i < header.fields.size(); ++i) {
                if (i > 0)
                    fields += ", ";
                fields += header.fields[i];
            }
            return fields;
        }(),
        header.viewpoint.empty() ? "Not specified" : header.viewpoint, header.hasXYZ() ? "Yes" : "No", header.hasRGB() ? "Yes" : "No");
}

/**
 * @brief Print detailed statistics about the point cloud
 * @param cloud The point cloud to analyze
 */
void printStatistics(const PointCloudXYZRGB& cloud) {
    if (cloud.empty()) {
        std::println("No points to analyze.");
        return;
    }

    auto [minPt, maxPt] = cloud.getBoundingBox();
    Point3D center = (minPt + maxPt) * 0.5f;
    Point3D size = maxPt - minPt;

    // Calculate centroid
    Point3D centroid{0, 0, 0};
    for (const auto& point : cloud.points) {
        centroid = centroid + point.position;
    }
    centroid = centroid * (1.0f / static_cast<float>(cloud.size()));

    std::println(R"(
Point Cloud Statistics
======================
Total Points:    {}
Is Dense:        {}

Bounding Box:
  Min:           ({:.3f}, {:.3f}, {:.3f})
  Max:           ({:.3f}, {:.3f}, {:.3f})
  Center:        ({:.3f}, {:.3f}, {:.3f})
  Size:          ({:.3f}, {:.3f}, {:.3f})

Centroid:        ({:.3f}, {:.3f}, {:.3f})
)",
                 cloud.size(), cloud.is_dense ? "Yes" : "No", minPt.x, minPt.y, minPt.z, maxPt.x, maxPt.y, maxPt.z, center.x, center.y, center.z, size.x, size.y, size.z, centroid.x, centroid.y,
                 centroid.z);
}

int main(int argc, char* argv[]) {
    CLI::App app{"ScanForge CLI Tool v1.0.0 - Modern C++23 Point Cloud Processing"};

    AppConfig config;

    // Required positional argument
    app.add_option("input", config.inputFile, "Input file path (PCD or LAS format)")->required()->check(CLI::ExistingFile);

    // Optional arguments
    app.add_option("-o,--output", config.outputFile, "Output file path");
    app.add_option("-f,--format", config.outputFormat, "Output format")->check(CLI::IsMember({"pcd", "las"}))->default_val("pcd");
    app.add_option("--variant", config.pcdVariant, "PCD variant (only used when format is 'pcd')")->check(CLI::IsMember({"ascii", "binary", "compressed"}))->default_val("ascii");

    // Flags
    app.add_flag("-i,--info", config.showInfo, "Show file information");
    app.add_flag("-s,--stats", config.showStats, "Show detailed statistics");
    app.add_flag("-v,--verbose", config.verbose, "Enable verbose logging");

    // Parse command line
    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
        return app.exit(e);
    }

    // Configure logging
    if (config.verbose) {
        Logger::getInstance().setLevel(LogLevel::DEBUG);
    }

    Log::info("ScanForge CLI Tool starting...");

    auto startTime = std::chrono::high_resolution_clock::now();

    try {
        // Detect file format
        std::string fileFormat = detectFileFormat(config.inputFile);
        Log::info("Detected file format: {}", fileFormat);

        // Load the point cloud based on format
        Log::info("Loading point cloud from: {}", config.inputFile);

        PointCloudXYZRGB cloud;
        PCDProcessor::PCDHeader pcdHeader;
        LASProcessor::LASHeader lasHeader;
        bool isLAS = false;

        if (fileFormat == "pcd") {
            PCDProcessor processor;
            auto [header, loadedCloud] = processor.loadPCD(config.inputFile);
            pcdHeader = header;
            cloud = loadedCloud;

            if (!header.isValid()) {
                Log::error("Failed to load PCD file or invalid header");
                return 1;
            }
        } else if (fileFormat == "las") {
            LASProcessor processor;
            auto [header, loadedCloud] = processor.loadLAS(config.inputFile);
            lasHeader = header;
            cloud = loadedCloud;
            isLAS = true;

            if (!header.isValid()) {
                Log::error("Failed to load LAS file or invalid header");
                return 1;
            }
        } else {
            Log::error("Unsupported file format: {}. Supported formats: PCD, LAS", fileFormat);
            return 1;
        }

        auto loadTime = std::chrono::high_resolution_clock::now();
        auto loadDuration = std::chrono::duration_cast<std::chrono::milliseconds>(loadTime - startTime);

        Log::info("Successfully loaded {} points in {} ms", cloud.size(), loadDuration.count());

        // Show file information
        if (config.showInfo) {
            if (isLAS) {
                printFileInfo(lasHeader, config.inputFile);
            } else {
                printFileInfo(pcdHeader, config.inputFile);
            }
        }

        // Show statistics
        if (config.showStats) {
            printStatistics(cloud);
        }

        // Convert and save if output file is specified
        if (!config.outputFile.empty()) {
            std::string actualFormat = config.outputFormat;
            if (actualFormat == "pcd") {
                actualFormat = config.pcdVariant;  // Use the specified PCD variant
            }
            Log::info("Converting to format: {} {}", config.outputFormat, 
                     config.outputFormat == "pcd" ? "(" + config.pcdVariant + ")" : "");
            Log::info("Saving to: {}", config.outputFile);

            // Create output directory if it doesn't exist
            fs::path outputPath(config.outputFile);
            if (outputPath.has_parent_path()) {
                fs::create_directories(outputPath.parent_path());
            }

            auto saveStartTime = std::chrono::high_resolution_clock::now();
            bool saveResult = false;

            if (config.outputFormat == "las") {
                // Save as LAS
                LASProcessor lasProcessor;
                auto outputHeader = LASProcessor::createLASHeader(cloud, LASProcessor::PointFormat::FORMAT_3);
                saveResult = lasProcessor.saveLAS(config.outputFile, outputHeader, cloud);
            } else if (config.outputFormat == "pcd") {
                // Save as PCD with specified variant
                PCDProcessor pcdProcessor;
                auto outputHeader = PCDProcessor::createXYZRGBHeader(cloud, config.pcdVariant);

                if (config.pcdVariant == "ascii") {
                    saveResult = pcdProcessor.savePCD_ASCII(config.outputFile, outputHeader, cloud);
                } else if (config.pcdVariant == "binary") {
                    saveResult = pcdProcessor.savePCD_Binary(config.outputFile, outputHeader, cloud);
                } else if (config.pcdVariant == "compressed") {
                    saveResult = pcdProcessor.savePCD_BinaryCompressed(config.outputFile, outputHeader, cloud);
                } else {
                    Log::error("Unsupported PCD variant: {}", config.pcdVariant);
                    return 1;
                }
            } else {
                Log::error("Unsupported output format: {}", config.outputFormat);
                return 1;
            }

            auto saveEndTime = std::chrono::high_resolution_clock::now();
            auto saveDuration = std::chrono::duration_cast<std::chrono::milliseconds>(saveEndTime - saveStartTime);

            if (saveResult) {
                Log::info("Successfully saved {} points to {} format in {} ms", cloud.size(), actualFormat, saveDuration.count());

                // Show file size comparison
                auto inputSize = fs::file_size(config.inputFile);
                auto outputSize = fs::file_size(config.outputFile);
                Log::info("File size: {} bytes -> {} bytes ({:.1f}%)", inputSize, outputSize, (static_cast<double>(outputSize) / static_cast<double>(inputSize)) * 100.0);
            } else {
                Log::error("Failed to save point cloud to: {}", config.outputFile);
                return 1;
            }
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);

        Log::info("Total processing time: {} ms", totalDuration.count());

    } catch (const std::exception& e) {
        Log::error("Exception occurred: {}", e.what());
        return 1;
    }

    Log::info("ScanForge CLI Tool completed successfully");
    return 0;
}
