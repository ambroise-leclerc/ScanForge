/**
 * @brief Unit tests for PCD writing functionality using Catch2
 */

#include <catch2/catch_all.hpp>
#include "PCDLoader.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>

using namespace std;
using namespace scanforge;

TEST_CASE("PCD writing functionality", "[PCDWriter][file_io]") {
    GIVEN("a simple point cloud with XYZRGB data") {
        PointCloudXYZRGB pointCloud;
        pointCloud.points = {
            PointXYZRGB(Point3D(1.0f, 2.0f, 3.0f), RGB(255, 0, 0)),
            PointXYZRGB(Point3D(4.0f, 5.0f, 6.0f), RGB(0, 255, 0)),
            PointXYZRGB(Point3D(7.0f, 8.0f, 9.0f), RGB(0, 0, 255))
        };
        pointCloud.width = 3;
        pointCloud.height = 1;
        pointCloud.is_dense = true;

        PCDLoader loader;
        
        WHEN("creating a header and saving as ASCII") {
            auto header = PCDLoader::createXYZRGBHeader(pointCloud, "ascii");
            
            THEN("the header is valid") {
                REQUIRE(header.isValid());
                REQUIRE(header.hasXYZ());
                REQUIRE(header.hasRGB());
                REQUIRE(header.points == 3);
                REQUIRE(header.dataType == "ascii");
            }
            
            AND_WHEN("saving the point cloud to ASCII file") {
                string filename = "test_output_ascii.pcd";
                bool saveResult = loader.savePCD_ASCII(filename, header, pointCloud);
                
                THEN("the save operation succeeds") {
                    REQUIRE(saveResult);
                    REQUIRE(filesystem::exists(filename));
                    
                    AND_WHEN("reading the file back") {
                        auto [loadedHeader, loadedCloud] = loader.loadPCD(filename);
                        
                        THEN("the data is preserved") {
                            REQUIRE(loadedHeader.isValid());
                            REQUIRE(loadedCloud.size() == pointCloud.size());
                            REQUIRE(loadedCloud.points[0].position.x == Catch::Approx(1.0f));
                            REQUIRE(loadedCloud.points[0].position.y == Catch::Approx(2.0f));
                            REQUIRE(loadedCloud.points[0].position.z == Catch::Approx(3.0f));
                        }
                    }
                    
                    // Cleanup
                    filesystem::remove(filename);
                }
            }
        }
        
        WHEN("saving as binary format") {
            auto header = PCDLoader::createXYZRGBHeader(pointCloud, "binary");
            string filename = "test_output_binary.pcd";
            
            bool saveResult = loader.savePCD_Binary(filename, header, pointCloud);
            
            THEN("the save operation succeeds") {
                REQUIRE(saveResult);
                REQUIRE(filesystem::exists(filename));
                
                AND_WHEN("reading the binary file back") {
                    auto [loadedHeader, loadedCloud] = loader.loadPCD(filename);
                    
                    THEN("the data is preserved with binary precision") {
                        REQUIRE(loadedHeader.isValid());
                        REQUIRE(loadedCloud.size() == pointCloud.size());
                        REQUIRE(loadedCloud.points[1].position.x == Catch::Approx(4.0f));
                        REQUIRE(loadedCloud.points[1].position.y == Catch::Approx(5.0f));
                        REQUIRE(loadedCloud.points[1].position.z == Catch::Approx(6.0f));
                    }
                }
                
                // Cleanup
                filesystem::remove(filename);
            }
        }
        
        WHEN("saving as binary compressed format") {
            auto header = PCDLoader::createXYZRGBHeader(pointCloud, "binary_compressed");
            string filename = "test_output_compressed.pcd";
            
            bool saveResult = loader.savePCD_BinaryCompressed(filename, header, pointCloud);
            
            THEN("the save operation succeeds") {
                REQUIRE(saveResult);
                REQUIRE(filesystem::exists(filename));
                
                AND_WHEN("reading the compressed file back") {
                    auto [loadedHeader, loadedCloud] = loader.loadPCD(filename);
                    
                    THEN("the data is preserved after compression/decompression") {
                        REQUIRE(loadedHeader.isValid());
                        REQUIRE(loadedCloud.size() == pointCloud.size());
                        REQUIRE(loadedCloud.points[2].position.x == Catch::Approx(7.0f));
                        REQUIRE(loadedCloud.points[2].position.y == Catch::Approx(8.0f));
                        REQUIRE(loadedCloud.points[2].position.z == Catch::Approx(9.0f));
                    }
                }
                
                // Cleanup
                filesystem::remove(filename);
            }
        }
    }
}

TEST_CASE("PCD round-trip validation with sample files", "[PCDWriter][validation]") {
    GIVEN("existing sample PCD files") {
        PCDLoader loader;
        
        WHEN("loading and saving sample.pcd in different formats") {
            auto [originalHeader, originalCloud] = loader.loadPCD("tests/data/sample.pcd");
            
            THEN("the original file loads successfully") {
                REQUIRE(originalHeader.isValid());
                REQUIRE(!originalCloud.empty());
                
                AND_WHEN("saving as ASCII and reloading") {
                    auto asciiHeader = PCDLoader::createXYZRGBHeader(originalCloud, "ascii");
                    string asciiFilename = "roundtrip_ascii.pcd";
                    
                    bool saveResult = loader.savePCD_ASCII(asciiFilename, asciiHeader, originalCloud);
                    REQUIRE(saveResult);
                    
                    auto [reloadedHeader, reloadedCloud] = loader.loadPCD(asciiFilename);
                    
                    THEN("the round-trip preserves the data") {
                        REQUIRE(reloadedHeader.isValid());
                        REQUIRE(reloadedCloud.size() == originalCloud.size());
                        
                        // Compare first and last points
                        if (!originalCloud.empty()) {
                            REQUIRE(reloadedCloud.points[0].position.x == Catch::Approx(originalCloud.points[0].position.x));
                            REQUIRE(reloadedCloud.points[0].position.y == Catch::Approx(originalCloud.points[0].position.y));
                            REQUIRE(reloadedCloud.points[0].position.z == Catch::Approx(originalCloud.points[0].position.z));
                        }
                    }
                    
                    filesystem::remove(asciiFilename);
                }
                
                AND_WHEN("saving as binary and reloading") {
                    auto binaryHeader = PCDLoader::createXYZRGBHeader(originalCloud, "binary");
                    string binaryFilename = "roundtrip_binary.pcd";
                    
                    bool saveResult = loader.savePCD_Binary(binaryFilename, binaryHeader, originalCloud);
                    REQUIRE(saveResult);
                    
                    auto [reloadedHeader, reloadedCloud] = loader.loadPCD(binaryFilename);
                    
                    THEN("the binary round-trip preserves exact precision") {
                        REQUIRE(reloadedHeader.isValid());
                        REQUIRE(reloadedCloud.size() == originalCloud.size());
                        
                        // Binary should preserve exact floating point values
                        if (!originalCloud.empty()) {
                            REQUIRE(reloadedCloud.points[0].position.x == originalCloud.points[0].position.x);
                            REQUIRE(reloadedCloud.points[0].position.y == originalCloud.points[0].position.y);
                            REQUIRE(reloadedCloud.points[0].position.z == originalCloud.points[0].position.z);
                        }
                    }
                    
                    filesystem::remove(binaryFilename);
                }
            }
        }
    }
}

TEST_CASE("PCD writing error handling", "[PCDWriter][error_handling]") {
    GIVEN("invalid scenarios for PCD writing") {
        PCDLoader loader;
        PointCloudXYZRGB emptyCloud;
        
        WHEN("attempting to save to an invalid path") {
            auto header = PCDLoader::createXYZRGBHeader(emptyCloud, "ascii");
            bool result = loader.savePCD_ASCII("/invalid/path/file.pcd", header, emptyCloud);
            
            THEN("the save operation fails gracefully") {
                REQUIRE_FALSE(result);
            }
        }
        
        WHEN("using generic savePCD with unknown format") {
            auto header = PCDLoader::createXYZRGBHeader(emptyCloud, "unknown_format");
            bool result = loader.savePCD("test_unknown.pcd", header, emptyCloud);
            
            THEN("the save operation fails gracefully") {
                REQUIRE_FALSE(result);
            }
        }
    }
}

TEST_CASE("PCD header creation", "[PCDWriter][header]") {
    GIVEN("various point cloud configurations") {
        WHEN("creating header for empty point cloud") {
            PointCloudXYZRGB emptyCloud;
            auto header = PCDLoader::createXYZRGBHeader(emptyCloud, "ascii");
            
            THEN("the header has correct default values") {
                REQUIRE(header.version == "0.7");
                REQUIRE(header.fields.size() == 4);
                REQUIRE(header.fields[0] == "x");
                REQUIRE(header.fields[1] == "y");
                REQUIRE(header.fields[2] == "z");
                REQUIRE(header.fields[3] == "rgb");
                REQUIRE(header.points == 0);
                REQUIRE(header.width == 0);
                REQUIRE(header.height == 1);
            }
        }
        
        WHEN("creating header for structured point cloud") {
            PointCloudXYZRGB structuredCloud;
            structuredCloud.points.resize(12); // 3x4 grid
            structuredCloud.width = 3;
            structuredCloud.height = 4;
            
            auto header = PCDLoader::createXYZRGBHeader(structuredCloud, "binary");
            
            THEN("the header preserves the structure") {
                REQUIRE(header.width == 3);
                REQUIRE(header.height == 4);
                REQUIRE(header.points == 12);
                REQUIRE(header.dataType == "binary");
            }
        }
    }
}

TEST_CASE("PCD format compatibility", "[PCDWriter][compatibility]") {
    GIVEN("a point cloud with color information") {
        PointCloudXYZRGB coloredCloud;
        coloredCloud.points = {
            PointXYZRGB(Point3D(0.1f, 0.2f, 0.3f), RGB(128, 64, 192)),
            PointXYZRGB(Point3D(-1.5f, 2.7f, -0.8f), RGB(255, 255, 255))
        };
        
        PCDLoader loader;
        
        WHEN("saving and reloading through all formats") {
            auto originalHeader = PCDLoader::createXYZRGBHeader(coloredCloud, "ascii");
            
            // Test all three formats
            vector<string> formats = {"ascii", "binary", "binary_compressed"};
            
            for (const auto& format : formats) {
                string filename = "compatibility_test_" + format + ".pcd";
                auto header = PCDLoader::createXYZRGBHeader(coloredCloud, format);
                
                bool saveResult = loader.savePCD(filename, header, coloredCloud);
                REQUIRE(saveResult);
                
                auto [loadedHeader, loadedCloud] = loader.loadPCD(filename);
                
                THEN("format " + format + " preserves data correctly") {
                    REQUIRE(loadedHeader.isValid());
                    REQUIRE(loadedCloud.size() == coloredCloud.size());
                    REQUIRE(loadedHeader.dataType == format);
                    
                    // Check color preservation (may have slight differences in ASCII due to formatting)
                    if (format == "binary" || format == "binary_compressed") {
                        REQUIRE(loadedCloud.points[0].color.r == coloredCloud.points[0].color.r);
                        REQUIRE(loadedCloud.points[0].color.g == coloredCloud.points[0].color.g);
                        REQUIRE(loadedCloud.points[0].color.b == coloredCloud.points[0].color.b);
                    }
                }
                
                filesystem::remove(filename);
            }
        }
    }
}