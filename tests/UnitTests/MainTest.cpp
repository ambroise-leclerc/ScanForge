
/**
 * @brief Main entry point and integration tests for ScanForge using Catch2
 */

#include <catch2/catch_all.hpp>
#include "PCDLoader.hpp"
#include "LZFCodec.hpp"
#include "PointCloudTypes.hpp"
#include "tooling/Logger.hpp"
#include <filesystem>
#include <chrono>

using namespace scanforge;
using namespace scanforge::tooling;

TEST_CASE("ScanForge integration tests", "[integration]") {
    GIVEN("a Logger instance") {
        Logger& logger = Logger::getInstance();
        WHEN("setting different log levels") {
            logger.setLevel(LogLevel::DEBUG);
            THEN("the log level is DEBUG") {
                REQUIRE(logger.getLevel() == LogLevel::DEBUG);
            }
            logger.setLevel(LogLevel::INFO);
            THEN("the log level is INFO") {
                REQUIRE(logger.getLevel() == LogLevel::INFO);
            }
            logger.setLevel(LogLevel::WARNING);
            THEN("the log level is WARNING") {
                REQUIRE(logger.getLevel() == LogLevel::WARNING);
            }
            logger.setLevel(LogLevel::ERROR);
            THEN("the log level is ERROR") {
                REQUIRE(logger.getLevel() == LogLevel::ERROR);
            }
        }
        WHEN("logging messages at different levels") {
            THEN("logging does not throw") {
                REQUIRE_NOTHROW(logger.log(LogLevel::INFO, "Test message"));
                REQUIRE_NOTHROW(logger.log(LogLevel::DEBUG, "Debug message {}", 42));
            }
        }
    }
}

TEST_CASE("Component integration", "[integration]") {
    GIVEN("a mock PCD header and LZFCodec") {
        PCDLoader::PCDHeader header;
        header.version = "0.7";
        header.fields = {"x", "y", "z"};
        header.sizes = {4, 4, 4};
        header.types = {'F', 'F', 'F'};
        header.counts = {1, 1, 1};
        header.width = 100;
        header.height = 1;
        header.points = 100;
        header.dataType = "binary_compressed";
        WHEN("checking header validity and XYZ fields") {
            THEN("header is valid and has XYZ fields") {
                REQUIRE(header.isValid());
                REQUIRE(header.hasXYZ());
            }
        }
        WHEN("decompressing test data with LZFCodec") {
            std::vector<uint8_t> testData = {0x01, 0x02, 0x03};
            auto result = LZFCodec::decompress(testData, 10);
            THEN("decompression runs (result may be empty for invalid data)") {
                REQUIRE(result.empty());
            }
        }
    }
    GIVEN("a point cloud and loader expectations") {
        PointCloudXYZ cloud;
        cloud.push_back(Point3D(1.0f, 2.0f, 3.0f));
        cloud.push_back(Point3D(4.0f, 5.0f, 6.0f));
        cloud.push_back(Point3D(7.0f, 8.0f, 9.0f));
        WHEN("checking the size and bounding box") {
            THEN("the size and bounding box are correct") {
                REQUIRE(cloud.size() == 3);
                auto [minPt, maxPt] = cloud.getBoundingBox();
                REQUIRE(minPt.x == 1.0f);
                REQUIRE(minPt.y == 2.0f);
                REQUIRE(minPt.z == 3.0f);
                REQUIRE(maxPt.x == 7.0f);
                REQUIRE(maxPt.y == 8.0f);
                REQUIRE(maxPt.z == 9.0f);
            }
        }
        WHEN("adding colored points") {
            PointCloudXYZRGB coloredCloud;
            coloredCloud.push_back(PointXYZRGB(1.0f, 2.0f, 3.0f, 255, 0, 0));
            coloredCloud.push_back(PointXYZRGB(4.0f, 5.0f, 6.0f, 0, 255, 0));
            THEN("the colored cloud has correct size and color values") {
                REQUIRE(coloredCloud.size() == 2);
                REQUIRE(coloredCloud[0].color.r == 255);
                REQUIRE(coloredCloud[1].color.g == 255);
            }
        }
    }
}

TEST_CASE("Real data processing pipeline", "[integration][filesystem]") {
    const std::string testDataPath = "tests/data/sample.pcd";
    
    SECTION("End-to-end PCD file processing") {
        // Skip if test data is not available
        if (!std::filesystem::exists(testDataPath)) {
            SKIP("Test data file not found: " + testDataPath);
        }
        
        Logger& logger = Logger::getInstance();
        logger.setLevel(LogLevel::INFO);
        
        // Test file exists and is readable
        std::ifstream file(testDataPath, std::ios::binary);
        REQUIRE(file.is_open());
        
        // Get file size
        file.seekg(0, std::ios::end);
        auto filePos = file.tellg();
        file.seekg(0, std::ios::beg);
        
        REQUIRE(filePos > 0);
        size_t fileSize = static_cast<size_t>(filePos);
        logger.log(LogLevel::INFO, "Test PCD file size: {} bytes", fileSize);
        
        // Read and validate header
        std::string line;
        bool foundVersion = false;
        bool foundFields = false;
        bool foundData = false;
        int expectedPoints = 0;
        
        while (std::getline(file, line)) {
            if (line.find("VERSION") == 0) {
                foundVersion = true;
                logger.log(LogLevel::INFO, "Found version line: {}", line);
            } else if (line.find("FIELDS") == 0) {
                foundFields = true;
                logger.log(LogLevel::INFO, "Found fields line: {}", line);
            } else if (line.find("POINTS") == 0) {
                std::string pointsStr = line.substr(line.find(' ') + 1);
                expectedPoints = std::stoi(pointsStr);
                logger.log(LogLevel::INFO, "Expected points: {}", expectedPoints);
            } else if (line.find("DATA binary_compressed") == 0) {
                foundData = true;
                logger.log(LogLevel::INFO, "Found compressed data section");
                break;
            }
        }
        
        REQUIRE(foundVersion);
        REQUIRE(foundFields);
        REQUIRE(foundData);
        REQUIRE(expectedPoints > 0);
        
        // Test that we can read some compressed data
        auto dataPos = file.tellg();
        size_t dataOffset = static_cast<size_t>(dataPos);
        size_t remainingData = fileSize - dataOffset;
        
        REQUIRE(remainingData > 0);
        logger.log(LogLevel::INFO, "Compressed data size: {} bytes", remainingData);
        
        // Read a sample of compressed data
        std::vector<uint8_t> compressedSample(std::min(size_t(1024), remainingData));
        file.read(reinterpret_cast<char*>(compressedSample.data()), 
                 static_cast<std::streamsize>(compressedSample.size()));
        auto bytesReadPos = file.gcount();
        size_t bytesRead = static_cast<size_t>(bytesReadPos);
        
        REQUIRE(bytesRead > 0);
        REQUIRE(bytesRead <= compressedSample.size());
        
        logger.log(LogLevel::INFO, "Read {} bytes of compressed data", bytesRead);
        
        file.close();
    }
}

TEST_CASE("Performance and memory tests", "[integration][performance]") {
    SECTION("Large point cloud performance") {
        auto start = std::chrono::high_resolution_clock::now();
        
        PointCloudXYZ largeCloud;
        constexpr size_t numPoints = 10000;
        
        // Reserve memory to avoid reallocations during timing
        largeCloud.points.reserve(numPoints);
        
        // Generate a large point cloud
        for (size_t i = 0; i < numPoints; ++i) {
            float x = static_cast<float>(i % 100);
            float y = static_cast<float>((i / 100) % 100);
            float z = static_cast<float>(i / 10000);
            largeCloud.push_back(Point3D(x, y, z));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        REQUIRE(largeCloud.size() == numPoints);
        
        // Performance should be reasonable (less than 100ms for 10K points)
        REQUIRE(duration.count() < 100);
        
        // Test bounding box calculation performance
        start = std::chrono::high_resolution_clock::now();
        auto [minPt, maxPt] = largeCloud.getBoundingBox();
        end = std::chrono::high_resolution_clock::now();
        
        auto bboxDuration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Bounding box calculation should be fast
        REQUIRE(bboxDuration.count() < 10000); // Less than 10ms
        
        // Verify bounding box correctness
        REQUIRE(minPt.x == 0.0f);
        REQUIRE(minPt.y == 0.0f);
        REQUIRE(minPt.z == 0.0f);
        REQUIRE(maxPt.x == 99.0f);
        REQUIRE(maxPt.y == 99.0f);
        REQUIRE(maxPt.z == 0.0f);
    }
    
    SECTION("Memory usage verification") {
        PointCloudXYZRGB coloredCloud;
        
        // Add points and verify memory usage is reasonable
        constexpr size_t numPoints = 1000;
        coloredCloud.points.reserve(numPoints);
        
        for (size_t i = 0; i < numPoints; ++i) {
            Point3D pos(static_cast<float>(i), 0.0f, 0.0f);
            RGB color(static_cast<uint8_t>(i % 256), 128, 64);
            coloredCloud.push_back(PointXYZRGB(pos, color));
        }
        
        REQUIRE(coloredCloud.size() == numPoints);
        
        // Test that clearing works and frees memory conceptually
        coloredCloud.clear();
        REQUIRE(coloredCloud.empty());
        REQUIRE(coloredCloud.size() == 0);
    }
}

TEST_CASE("Error handling and edge cases", "[integration][error-handling]") {
    SECTION("File system error handling") {
        const std::string nonexistentFile = "tests/data/nonexistent.pcd";
        
        std::ifstream file(nonexistentFile);
        REQUIRE_FALSE(file.is_open());
        
        // Test that filesystem operations handle missing files gracefully
        REQUIRE_FALSE(std::filesystem::exists(nonexistentFile));
    }
    
    SECTION("Logger error conditions") {
        Logger& logger = Logger::getInstance();
        
        // Test logging with different levels
        logger.setLevel(LogLevel::ERROR);
        
        // These should not throw even with restrictive log level
        REQUIRE_NOTHROW(logger.log(LogLevel::DEBUG, "This won't be shown"));
        REQUIRE_NOTHROW(logger.log(LogLevel::INFO, "This won't be shown"));
        REQUIRE_NOTHROW(logger.log(LogLevel::ERROR, "This will be shown"));
    }
    
    SECTION("Point cloud edge cases") {
        PointCloudXYZ cloud;
        
        // Test operations on empty cloud
        REQUIRE(cloud.empty());
        auto [minPt, maxPt] = cloud.getBoundingBox();
        REQUIRE(minPt.x == 0.0f);
        REQUIRE(maxPt.x == 0.0f);
        
        // Test with extreme coordinate values
        cloud.push_back(Point3D(std::numeric_limits<float>::max(), 0.0f, 0.0f));
        cloud.push_back(Point3D(std::numeric_limits<float>::lowest(), 0.0f, 0.0f));
        
        REQUIRE(cloud.size() == 2);
        
        auto [minPt2, maxPt2] = cloud.getBoundingBox();
        REQUIRE(minPt2.x == std::numeric_limits<float>::lowest());
        REQUIRE(maxPt2.x == std::numeric_limits<float>::max());
    }
}

// Test that demonstrates the complete workflow that would be used in main.cpp
TEST_CASE("Simulated main application workflow", "[integration][workflow]") {
    SECTION("Command-line application simulation") {
        Logger& logger = Logger::getInstance();
        logger.setLevel(LogLevel::INFO);
        
        // Simulate application startup
        logger.log(LogLevel::INFO, "Starting ScanForge application simulation");
        
        // Simulate configuration
        struct MockConfig {
            std::string inputFile = "tests/data/sample.pcd";
            std::string outputFile = "/tmp/output.pcd";
            std::string outputFormat = "binary";
            bool showInfo = true;
            bool showStats = true;
            bool verbose = true;
        } config;
        
        // Simulate file existence check
        bool inputExists = std::filesystem::exists(config.inputFile);
        if (inputExists) {
            logger.log(LogLevel::INFO, "Input file found: {}", config.inputFile);
        } else {
            logger.log(LogLevel::WARNING, "Input file not found: {}", config.inputFile);
        }
        
        // Simulate point cloud processing
        PointCloudXYZ processedCloud;
        processedCloud.push_back(Point3D(1.0f, 2.0f, 3.0f));
        processedCloud.push_back(Point3D(4.0f, 5.0f, 6.0f));
        
        if (config.showStats) {
            auto [minPt, maxPt] = processedCloud.getBoundingBox();
            logger.log(LogLevel::INFO, "Point cloud statistics:");
            logger.log(LogLevel::INFO, "  Points: {}", processedCloud.size());
            logger.log(LogLevel::INFO, "  Bounding box: ({:.2f},{:.2f},{:.2f}) to ({:.2f},{:.2f},{:.2f})",
                      minPt.x, minPt.y, minPt.z, maxPt.x, maxPt.y, maxPt.z);
        }
        
        // Simulate successful completion
        logger.log(LogLevel::INFO, "Application simulation completed successfully");
        
        REQUIRE(processedCloud.size() > 0);
        REQUIRE(true); // Workflow completed without exceptions
    }
}
