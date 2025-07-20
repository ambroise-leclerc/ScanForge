#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "LASProcessor.hpp"
#include "PointCloudTypes.hpp"
#include <filesystem>
#include <fstream>
#include <memory>

using namespace scanforge;
using Catch::Matchers::WithinAbs;

namespace fs = std::filesystem;

class LASTestFixture {
public:
    LASTestFixture() {
        // Create test directory
        testDir = fs::temp_directory_path() / "scanforge_las_tests";
        fs::create_directories(testDir);
    }
    
    ~LASTestFixture() {
        // Clean up test files
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }
    
    fs::path testDir;
    
    // Helper to create a minimal valid LAS file
    void createTestLASFile(const fs::path& filename, uint32_t numPoints = 3) {
        std::ofstream file(filename, std::ios::binary);
        REQUIRE(file.is_open());
        
        // Create LAS header
        LASProcessor::LASHeader header{};
        header.fileSignature = {'L', 'A', 'S', 'F'};
        header.versionMajor = 1;
        header.versionMinor = 3;
        header.headerSize = 235;
        header.offsetToPointData = 235;
        header.pointDataRecordFormat = LASProcessor::PointFormat::FORMAT_3;
        header.pointDataRecordLength = 34;
        header.legacyNumberOfPointRecords = numPoints;
        header.xScaleFactor = header.yScaleFactor = header.zScaleFactor = 0.01;
        header.xOffset = header.yOffset = header.zOffset = 0.0;
        header.minX = 0.0; header.maxX = 2.0;
        header.minY = 0.0; header.maxY = 2.0; 
        header.minZ = 0.0; header.maxZ = 2.0;
        
        // Write header fields in LAS binary format
        file.write(reinterpret_cast<const char*>(header.fileSignature.data()), 4);
        file.write(reinterpret_cast<const char*>(&header.fileSourceID), sizeof(uint16_t));
        file.write(reinterpret_cast<const char*>(&header.globalEncoding), sizeof(uint16_t));
        
        // Write GUID (16 bytes)
        file.write(reinterpret_cast<const char*>(&header.projectID1), sizeof(uint32_t));
        file.write(reinterpret_cast<const char*>(&header.projectID2), sizeof(uint32_t));
        file.write(reinterpret_cast<const char*>(&header.projectID3), sizeof(uint32_t));
        file.write(reinterpret_cast<const char*>(&header.projectID4), sizeof(uint32_t));
        
        file.write(reinterpret_cast<const char*>(&header.versionMajor), sizeof(uint8_t));
        file.write(reinterpret_cast<const char*>(&header.versionMinor), sizeof(uint8_t));
        
        // System identifier and generating software (32 bytes each)
        char sysId[32] = "ScanForge Test";
        char software[32] = "ScanForge v1.0.0";
        file.write(sysId, 32);
        file.write(software, 32);
        
        file.write(reinterpret_cast<const char*>(&header.creationDayOfYear), sizeof(uint16_t));
        file.write(reinterpret_cast<const char*>(&header.creationYear), sizeof(uint16_t));
        file.write(reinterpret_cast<const char*>(&header.headerSize), sizeof(uint16_t));
        file.write(reinterpret_cast<const char*>(&header.offsetToPointData), sizeof(uint32_t));
        file.write(reinterpret_cast<const char*>(&header.numberOfVariableLengthRecords), sizeof(uint32_t));
        
        uint8_t format = static_cast<uint8_t>(header.pointDataRecordFormat);
        file.write(reinterpret_cast<const char*>(&format), sizeof(uint8_t));
        file.write(reinterpret_cast<const char*>(&header.pointDataRecordLength), sizeof(uint16_t));
        file.write(reinterpret_cast<const char*>(&header.legacyNumberOfPointRecords), sizeof(uint32_t));
        
        // Legacy points by return
        for (const auto& count : header.legacyNumberOfPointsByReturn) {
            file.write(reinterpret_cast<const char*>(&count), sizeof(uint32_t));
        }
        
        // Scale factors and offsets
        file.write(reinterpret_cast<const char*>(&header.xScaleFactor), sizeof(double));
        file.write(reinterpret_cast<const char*>(&header.yScaleFactor), sizeof(double));
        file.write(reinterpret_cast<const char*>(&header.zScaleFactor), sizeof(double));
        file.write(reinterpret_cast<const char*>(&header.xOffset), sizeof(double));
        file.write(reinterpret_cast<const char*>(&header.yOffset), sizeof(double));
        file.write(reinterpret_cast<const char*>(&header.zOffset), sizeof(double));
        
        // Bounding box
        file.write(reinterpret_cast<const char*>(&header.maxX), sizeof(double));
        file.write(reinterpret_cast<const char*>(&header.minX), sizeof(double));
        file.write(reinterpret_cast<const char*>(&header.maxY), sizeof(double));
        file.write(reinterpret_cast<const char*>(&header.minY), sizeof(double));
        file.write(reinterpret_cast<const char*>(&header.maxZ), sizeof(double));
        file.write(reinterpret_cast<const char*>(&header.minZ), sizeof(double));
        
        // LAS 1.3 extended field
        file.write(reinterpret_cast<const char*>(&header.startOfWaveformDataPacketRecord), sizeof(uint64_t));
        
        // Pad to offset (if needed)
        while (file.tellp() < header.offsetToPointData) {
            char padding = 0;
            file.write(&padding, 1);
        }
        
        // Write test points (Format 3: x,y,z,intensity,return,class,angle,user,source,gps,rgb)
        for (uint32_t i = 0; i < numPoints; ++i) {
            // Coordinates (scaled integers)
            int32_t x = static_cast<int32_t>(i * 100);  // 0.0, 1.0, 2.0 after scaling
            int32_t y = static_cast<int32_t>(i * 100);
            int32_t z = static_cast<int32_t>(i * 100);
            file.write(reinterpret_cast<const char*>(&x), sizeof(int32_t));
            file.write(reinterpret_cast<const char*>(&y), sizeof(int32_t));
            file.write(reinterpret_cast<const char*>(&z), sizeof(int32_t));
            
            // Intensity, return info, classification, etc.
            uint16_t intensity = static_cast<uint16_t>(1000 + i * 100);
            uint8_t returnInfo = 0x11;  // Return 1 of 1
            uint8_t classification = 1; // Unclassified
            int8_t scanAngle = 0;
            uint8_t userData = 0;
            uint16_t pointSourceID = 0;
            
            file.write(reinterpret_cast<const char*>(&intensity), sizeof(uint16_t));
            file.write(reinterpret_cast<const char*>(&returnInfo), sizeof(uint8_t));
            file.write(reinterpret_cast<const char*>(&classification), sizeof(uint8_t));
            file.write(reinterpret_cast<const char*>(&scanAngle), sizeof(int8_t));
            file.write(reinterpret_cast<const char*>(&userData), sizeof(uint8_t));
            file.write(reinterpret_cast<const char*>(&pointSourceID), sizeof(uint16_t));
            
            // GPS time (Format 3 has GPS time)
            double gpsTime = 123456.789 + i;
            file.write(reinterpret_cast<const char*>(&gpsTime), sizeof(double));
            
            // RGB (Format 3 has RGB)
            uint16_t r = static_cast<uint16_t>((255 - i * 50) << 8);
            uint16_t g = static_cast<uint16_t>((128 + i * 60) << 8);
            uint16_t b = static_cast<uint16_t>((100 + i * 40) << 8);
            file.write(reinterpret_cast<const char*>(&r), sizeof(uint16_t));
            file.write(reinterpret_cast<const char*>(&g), sizeof(uint16_t));
            file.write(reinterpret_cast<const char*>(&b), sizeof(uint16_t));
        }
        
        file.close();
    }
};

TEST_CASE_METHOD(LASTestFixture, "LASProcessor Header Validation", "[LASProcessor][Header]") {
    SECTION("Valid LAS header is recognized") {
        LASProcessor::LASHeader header{};
        header.fileSignature = {'L', 'A', 'S', 'F'};
        header.versionMajor = 1;
        header.versionMinor = 3;
        
        REQUIRE(header.isValid());
        REQUIRE(header.getVersion() == "1.3");
    }
    
    SECTION("Invalid file signature is rejected") {
        LASProcessor::LASHeader header{};
        header.fileSignature = {'L', 'A', 'S', 'X'};  // Invalid signature
        header.versionMajor = 1;
        header.versionMinor = 3;
        
        REQUIRE_FALSE(header.isValid());
    }
    
    SECTION("Unsupported version is rejected") {
        LASProcessor::LASHeader header{};
        header.fileSignature = {'L', 'A', 'S', 'F'};
        header.versionMajor = 1;
        header.versionMinor = 1;  // LAS 1.1 not supported
        
        REQUIRE_FALSE(header.isValid());
    }
}

TEST_CASE_METHOD(LASTestFixture, "LASProcessor Point Format Detection", "[LASProcessor][PointFormat]") {
    LASProcessor::LASHeader header{};
    
    SECTION("RGB detection works correctly") {
        header.pointDataRecordFormat = LASProcessor::PointFormat::FORMAT_2;
        REQUIRE(header.hasRGB());
        REQUIRE_FALSE(header.hasGPSTime());
        
        header.pointDataRecordFormat = LASProcessor::PointFormat::FORMAT_3;
        REQUIRE(header.hasRGB());
        REQUIRE(header.hasGPSTime());
        
        header.pointDataRecordFormat = LASProcessor::PointFormat::FORMAT_0;
        REQUIRE_FALSE(header.hasRGB());
        REQUIRE_FALSE(header.hasGPSTime());
    }
    
    SECTION("GPS time detection works correctly") {
        header.pointDataRecordFormat = LASProcessor::PointFormat::FORMAT_1;
        REQUIRE(header.hasGPSTime());
        REQUIRE_FALSE(header.hasRGB());
        
        header.pointDataRecordFormat = LASProcessor::PointFormat::FORMAT_6;
        REQUIRE(header.hasGPSTime());
        REQUIRE_FALSE(header.hasRGB());
    }
}

TEST_CASE_METHOD(LASTestFixture, "LASProcessor File I/O Operations", "[LASProcessor][FileIO]") {
    LASProcessor loader;
    fs::path testFile = testDir / "test_load.las";
    
    SECTION("Loading valid LAS file succeeds") {
        createTestLASFile(testFile, 3);
        
        auto [header, pointCloud] = loader.loadLAS(testFile);
        
        REQUIRE(header.isValid());
        REQUIRE(header.getVersion() == "1.3");
        REQUIRE(header.getTotalPointCount() == 3);
        REQUIRE(pointCloud.size() == 3);
        REQUIRE(pointCloud.width == 3);
        REQUIRE(pointCloud.height == 1);
        
        // Check point coordinates
        REQUIRE_THAT(pointCloud.points[0].position.x, WithinAbs(0.0f, 0.01f));
        REQUIRE_THAT(pointCloud.points[1].position.x, WithinAbs(1.0f, 0.01f));
        REQUIRE_THAT(pointCloud.points[2].position.x, WithinAbs(2.0f, 0.01f));
    }
    
    SECTION("Loading non-existent file fails gracefully") {
        fs::path nonExistentFile = testDir / "does_not_exist.las";
        
        auto [header, pointCloud] = loader.loadLAS(nonExistentFile);
        
        REQUIRE_FALSE(header.isValid());
        REQUIRE(pointCloud.empty());
    }
    
    SECTION("Round-trip save and load preserves data") {
        // Create test point cloud
        PointCloudXYZRGB originalCloud;
        originalCloud.points.push_back(PointXYZRGB(Point3D(1.0f, 2.0f, 3.0f), RGB(255, 128, 64)));
        originalCloud.points.push_back(PointXYZRGB(Point3D(4.0f, 5.0f, 6.0f), RGB(100, 200, 150)));
        originalCloud.width = 2;
        originalCloud.height = 1;
        
        // Create header and save
        auto saveHeader = LASProcessor::createLASHeader(originalCloud, LASProcessor::PointFormat::FORMAT_3);
        fs::path saveFile = testDir / "test_save.las";
        
        REQUIRE(loader.saveLAS(saveFile, saveHeader, originalCloud));
        REQUIRE(fs::exists(saveFile));
        
        // Load back and compare
        auto [loadedHeader, loadedCloud] = loader.loadLAS(saveFile);
        
        REQUIRE(loadedHeader.isValid());
        REQUIRE(loadedCloud.size() == originalCloud.size());
        
        // Check coordinates (within scale factor precision)
        REQUIRE_THAT(loadedCloud.points[0].position.x, WithinAbs(1.0f, 0.02f));
        REQUIRE_THAT(loadedCloud.points[0].position.y, WithinAbs(2.0f, 0.02f));
        REQUIRE_THAT(loadedCloud.points[0].position.z, WithinAbs(3.0f, 0.02f));
        
        REQUIRE_THAT(loadedCloud.points[1].position.x, WithinAbs(4.0f, 0.02f));
        REQUIRE_THAT(loadedCloud.points[1].position.y, WithinAbs(5.0f, 0.02f));
        REQUIRE_THAT(loadedCloud.points[1].position.z, WithinAbs(6.0f, 0.02f));
        
        // Colors should be preserved (with 16->8 bit conversion tolerance)
        REQUIRE(loadedCloud.points[0].color.r >= 250);
        REQUIRE(loadedCloud.points[0].color.g >= 125);
    }
}

TEST_CASE_METHOD(LASTestFixture, "LASProcessor Header Creation", "[LASProcessor][HeaderCreation]") {
    SECTION("Header creation for empty point cloud") {
        PointCloudXYZRGB emptyCloud;
        auto header = LASProcessor::createLASHeader(emptyCloud);
        
        REQUIRE(header.isValid());
        REQUIRE(header.versionMajor == 1);
        REQUIRE(header.versionMinor == 3);
        REQUIRE(header.getTotalPointCount() == 0);
        REQUIRE(header.pointDataRecordFormat == LASProcessor::PointFormat::FORMAT_3);
    }
    
    SECTION("Header creation with bounding box calculation") {
        PointCloudXYZRGB cloud;
        cloud.points.push_back(PointXYZRGB(Point3D(-1.0f, -2.0f, -3.0f), RGB(255, 255, 255)));
        cloud.points.push_back(PointXYZRGB(Point3D(10.0f, 20.0f, 30.0f), RGB(0, 0, 0)));
        
        auto header = LASProcessor::createLASHeader(cloud);
        
        REQUIRE(header.isValid());
        REQUIRE(header.getTotalPointCount() == 2);
        REQUIRE_THAT(header.minX, WithinAbs(-1.0, 0.001));
        REQUIRE_THAT(header.maxX, WithinAbs(10.0, 0.001));
        REQUIRE_THAT(header.minY, WithinAbs(-2.0, 0.001));
        REQUIRE_THAT(header.maxY, WithinAbs(20.0, 0.001));
        REQUIRE_THAT(header.minZ, WithinAbs(-3.0, 0.001));
        REQUIRE_THAT(header.maxZ, WithinAbs(30.0, 0.001));
    }
    
    SECTION("Different point formats can be specified") {
        PointCloudXYZRGB cloud;
        cloud.points.push_back(PointXYZRGB(Point3D(1.0f, 2.0f, 3.0f), RGB(255, 128, 64)));
        
        auto headerFormat0 = LASProcessor::createLASHeader(cloud, LASProcessor::PointFormat::FORMAT_0);
        auto headerFormat3 = LASProcessor::createLASHeader(cloud, LASProcessor::PointFormat::FORMAT_3);
        
        REQUIRE(headerFormat0.pointDataRecordFormat == LASProcessor::PointFormat::FORMAT_0);
        REQUIRE(headerFormat3.pointDataRecordFormat == LASProcessor::PointFormat::FORMAT_3);
        REQUIRE_FALSE(headerFormat0.hasRGB());
        REQUIRE_FALSE(headerFormat0.hasGPSTime());
        REQUIRE(headerFormat3.hasRGB());
        REQUIRE(headerFormat3.hasGPSTime());
    }
}

TEST_CASE_METHOD(LASTestFixture, "LASProcessor Point Data Processing", "[LASProcessor][PointData]") {
    LASProcessor loader;
    
    SECTION("Point coordinates are correctly scaled") {
        fs::path testFile = testDir / "test_scaling.las";
        createTestLASFile(testFile, 3);
        
        auto [header, pointCloud] = loader.loadLAS(testFile);
        
        REQUIRE(pointCloud.size() == 3);
        
        // Points should be at 0.0, 1.0, 2.0 based on our test data generation
        REQUIRE_THAT(pointCloud.points[0].position.x, WithinAbs(0.0f, 0.01f));
        REQUIRE_THAT(pointCloud.points[0].position.y, WithinAbs(0.0f, 0.01f));
        REQUIRE_THAT(pointCloud.points[0].position.z, WithinAbs(0.0f, 0.01f));
        
        REQUIRE_THAT(pointCloud.points[2].position.x, WithinAbs(2.0f, 0.01f));
        REQUIRE_THAT(pointCloud.points[2].position.y, WithinAbs(2.0f, 0.01f));
        REQUIRE_THAT(pointCloud.points[2].position.z, WithinAbs(2.0f, 0.01f));
    }
    
    SECTION("RGB colors are correctly converted") {
        fs::path testFile = testDir / "test_colors.las";
        createTestLASFile(testFile, 2);
        
        auto [header, pointCloud] = loader.loadLAS(testFile);
        
        REQUIRE(pointCloud.size() == 2);
        REQUIRE(header.hasRGB());
        
        // Check that colors were read (exact values depend on 16->8 bit conversion)
        REQUIRE(pointCloud.points[0].color.r > 0);
        REQUIRE(pointCloud.points[0].color.g > 0);
        REQUIRE(pointCloud.points[0].color.b > 0);
    }
}

TEST_CASE("LASProcessor Integration with Point Cloud Types", "[LASProcessor][Integration]") {
    SECTION("LAS points convert correctly to PointXYZRGB") {
        LASProcessor::LASPoint lasPoint;
        lasPoint.position = Point3D(1.5f, 2.5f, 3.5f);
        lasPoint.intensity = 1500;
        lasPoint.color = RGB(200, 150, 100);
        lasPoint.setReturnInfo(2, 3, true, false);
        
        // Verify return info encoding
        REQUIRE(lasPoint.getReturnNumber() == 2);
        REQUIRE(lasPoint.getNumberOfReturns() == 3);
        REQUIRE(lasPoint.getScanDirection() == true);
        REQUIRE(lasPoint.getEdgeOfFlightLine() == false);
        
        // Test conversion to PointXYZRGB
        PointXYZRGB point(lasPoint.position, lasPoint.color);
        REQUIRE_THAT(point.position.x, WithinAbs(1.5f, 0.001f));
        REQUIRE_THAT(point.position.y, WithinAbs(2.5f, 0.001f));
        REQUIRE_THAT(point.position.z, WithinAbs(3.5f, 0.001f));
        REQUIRE(point.color.r == 200);
        REQUIRE(point.color.g == 150);
        REQUIRE(point.color.b == 100);
    }
}

TEST_CASE("LASProcessor Point Record Length Calculation", "[LASProcessor][PointRecordLength]") {
    using PointFormat = LASProcessor::PointFormat;
    
    REQUIRE(LASProcessor::getPointRecordLength(PointFormat::FORMAT_0) == 20);
    REQUIRE(LASProcessor::getPointRecordLength(PointFormat::FORMAT_1) == 28);
    REQUIRE(LASProcessor::getPointRecordLength(PointFormat::FORMAT_2) == 26);
    REQUIRE(LASProcessor::getPointRecordLength(PointFormat::FORMAT_3) == 34);
    REQUIRE(LASProcessor::getPointRecordLength(PointFormat::FORMAT_6) == 30);
    REQUIRE(LASProcessor::getPointRecordLength(PointFormat::FORMAT_7) == 36);
    REQUIRE(LASProcessor::getPointRecordLength(PointFormat::FORMAT_8) == 38);
}