
/**
 * @brief Unit tests for PointCloudTypes using Catch2 and realistic test scenarios
 */

#include <catch2/catch_all.hpp>
#include "PointCloudTypes.hpp"

#include <cmath>
#include <limits>
#include <numbers>
#include <algorithm>

using namespace std;
using namespace scanforge;
using Catch::Approx;

TEST_CASE("Point3D construction", "[PointCloudTypes]") {
    GIVEN("a default constructed Point3D") {
        Point3D point;
        (void)point;
        THEN("it does not crash and is default initialized") {
            REQUIRE(true);
        }
    }
    GIVEN("a parameterized Point3D") {
        Point3D point(1.0f, 2.0f, 3.0f);
        THEN("the fields are set correctly") {
            REQUIRE(point.x == 1.0f);
            REQUIRE(point.y == 2.0f);
            REQUIRE(point.z == 3.0f);
        }
    }
    GIVEN("a Point3D with extreme values") {
        Point3D point(numeric_limits<float>::max(), numeric_limits<float>::lowest(), 0.0f);
        THEN("the fields are set to the extremes") {
            REQUIRE(point.x == numeric_limits<float>::max());
            REQUIRE(point.y == numeric_limits<float>::lowest());
            REQUIRE(point.z == 0.0f);
        }
    }
}

TEST_CASE("Point3D arithmetic operations", "[PointCloudTypes]") {
    Point3D point1(1.0f, 2.0f, 3.0f);
    Point3D point2(4.0f, 5.0f, 6.0f);
    GIVEN("two Point3D objects") {
        WHEN("adding them") {
            Point3D sum = point1 + point2;
            THEN("the result is the component-wise sum") {
                REQUIRE(sum.x == 5.0f);
                REQUIRE(sum.y == 7.0f);
                REQUIRE(sum.z == 9.0f);
            }
        }
        WHEN("subtracting them") {
            Point3D diff = point2 - point1;
            THEN("the result is the component-wise difference") {
                REQUIRE(diff.x == 3.0f);
                REQUIRE(diff.y == 3.0f);
                REQUIRE(diff.z == 3.0f);
            }
        }
        WHEN("multiplying by a positive scalar") {
            Point3D scaled = point1 * 2.0f;
            THEN("the result is scaled accordingly") {
                REQUIRE(scaled.x == 2.0f);
                REQUIRE(scaled.y == 4.0f);
                REQUIRE(scaled.z == 6.0f);
            }
        }
        WHEN("multiplying by zero") {
            Point3D scaled = point1 * 0.0f;
            THEN("the result is the zero vector") {
                REQUIRE(scaled.x == 0.0f);
                REQUIRE(scaled.y == 0.0f);
                REQUIRE(scaled.z == 0.0f);
            }
        }
        WHEN("multiplying by a negative scalar") {
            Point3D scaled = point1 * -1.0f;
            THEN("the result is the negated vector") {
                REQUIRE(scaled.x == -1.0f);
                REQUIRE(scaled.y == -2.0f);
                REQUIRE(scaled.z == -3.0f);
            }
        }
    }
}

TEST_CASE("Point3D dot product", "[PointCloudTypes]") {
    SECTION("Basic dot product") {
        Point3D point1(1.0f, 2.0f, 3.0f);
        Point3D point2(4.0f, 5.0f, 6.0f);
        
        float dot = point1.dot(point2);
        REQUIRE(dot == 32.0f); // 1*4 + 2*5 + 3*6 = 4 + 10 + 18 = 32
    }
    
    SECTION("Orthogonal vectors") {
        Point3D point1(1.0f, 0.0f, 0.0f);
        Point3D point2(0.0f, 1.0f, 0.0f);
        
        float dot = point1.dot(point2);
        REQUIRE(dot == 0.0f);
    }
    
    SECTION("Same vector") {
        Point3D point(3.0f, 4.0f, 0.0f);
        float dot = point.dot(point);
        REQUIRE(dot == 25.0f); // 3² + 4² = 9 + 16 = 25
    }
}

TEST_CASE("Point3D magnitude calculations", "[PointCloudTypes]") {
    SECTION("Unit vector magnitude") {
        Point3D unitX(1.0f, 0.0f, 0.0f);
        REQUIRE(unitX.magnitude() == Approx(1.0f));
        
        Point3D unitY(0.0f, 1.0f, 0.0f);
        REQUIRE(unitY.magnitude() == Approx(1.0f));
        
        Point3D unitZ(0.0f, 0.0f, 1.0f);
        REQUIRE(unitZ.magnitude() == Approx(1.0f));
    }
    
    SECTION("3-4-5 triangle magnitude") {
        Point3D point(3.0f, 4.0f, 0.0f);
        REQUIRE(point.magnitude() == Approx(5.0f));
    }
    
    SECTION("Zero vector magnitude") {
        Point3D zero(0.0f, 0.0f, 0.0f);
        REQUIRE(zero.magnitude() == Approx(0.0f));
    }
    
    SECTION("3D Pythagorean magnitude") {
        Point3D point(1.0f, 2.0f, 2.0f);
        float expected = sqrt(1.0f + 4.0f + 4.0f); // sqrt(9) = 3
        REQUIRE(point.magnitude() == Approx(expected));
    }
}

TEST_CASE("Point3D normalization", "[PointCloudTypes]") {
    SECTION("Normalize regular vector") {
        Point3D point(3.0f, 4.0f, 0.0f);
        Point3D normalized = point.normalize();
        
        REQUIRE(normalized.x == Approx(0.6f));  // 3/5
        REQUIRE(normalized.y == Approx(0.8f));  // 4/5
        REQUIRE(normalized.z == Approx(0.0f));
        REQUIRE(normalized.magnitude() == Approx(1.0f));
    }
    
    SECTION("Normalize zero vector") {
        Point3D zero(0.0f, 0.0f, 0.0f);
        Point3D normalized = zero.normalize();
        
        REQUIRE(normalized.x == 0.0f);
        REQUIRE(normalized.y == 0.0f);
        REQUIRE(normalized.z == 0.0f);
    }
    
    SECTION("Normalize unit vector") {
        Point3D unit(1.0f, 0.0f, 0.0f);
        Point3D normalized = unit.normalize();
        
        REQUIRE(normalized.x == Approx(1.0f));
        REQUIRE(normalized.y == Approx(0.0f));
        REQUIRE(normalized.z == Approx(0.0f));
    }
}

TEST_CASE("RGB color structure", "[PointCloudTypes]") {
    SECTION("RGB construction") {
        RGB color(255, 128, 64);
        REQUIRE(color.r == 255);
        REQUIRE(color.g == 128);
        REQUIRE(color.b == 64);
    }
    
    SECTION("RGB from packed value") {
        uint32_t packed = 0xFF8040; // Red=255, Green=128, Blue=64
        RGB color(packed);
        
        REQUIRE(color.r == 255);
        REQUIRE(color.g == 128);
        REQUIRE(color.b == 64);
    }
    
    SECTION("RGB to packed value") {
        RGB color(255, 128, 64);
        uint32_t packed = color.toPacked();
        
        REQUIRE(packed == 0xFF8040);
    }
    
    SECTION("RGB round-trip conversion") {
        uint32_t originalPacked = 0x123456;
        RGB color(originalPacked);
        uint32_t convertedPacked = color.toPacked();
        
        REQUIRE(convertedPacked == originalPacked);
    }
}

TEST_CASE("PointXYZRGB structure", "[PointCloudTypes]") {
    SECTION("Construction with separate components") {
        PointXYZRGB point(1.0f, 2.0f, 3.0f, 255, 128, 64);
        
        REQUIRE(point.position.x == 1.0f);
        REQUIRE(point.position.y == 2.0f);
        REQUIRE(point.position.z == 3.0f);
        REQUIRE(point.color.r == 255);
        REQUIRE(point.color.g == 128);
        REQUIRE(point.color.b == 64);
    }
    
    SECTION("Construction with Point3D and RGB") {
        Point3D pos(1.0f, 2.0f, 3.0f);
        RGB color(255, 128, 64);
        PointXYZRGB point(pos, color);
        
        REQUIRE(point.position.x == pos.x);
        REQUIRE(point.position.y == pos.y);
        REQUIRE(point.position.z == pos.z);
        REQUIRE(point.color.r == color.r);
        REQUIRE(point.color.g == color.g);
        REQUIRE(point.color.b == color.b);
    }
}

TEST_CASE("PointCloud basic operations", "[PointCloudTypes]") {
    SECTION("Empty point cloud") {
        PointCloudXYZ cloud;
        
        REQUIRE(cloud.empty());
        REQUIRE(cloud.size() == 0);
        REQUIRE(cloud.width == 0);
        REQUIRE(cloud.height == 0);
        REQUIRE(cloud.is_dense == true);
    }
    
    SECTION("Add points to cloud") {
        PointCloudXYZ cloud;
        
        cloud.push_back(Point3D(1.0f, 2.0f, 3.0f));
        cloud.push_back(Point3D(4.0f, 5.0f, 6.0f));
        
        REQUIRE(cloud.size() == 2);
        REQUIRE(cloud[0].x == 1.0f);
        REQUIRE(cloud[1].z == 6.0f);
    }
    
    SECTION("Clear point cloud") {
        PointCloudXYZ cloud;
        cloud.push_back(Point3D(1.0f, 2.0f, 3.0f));
        
        cloud.clear();
        
        REQUIRE(cloud.empty());
        REQUIRE(cloud.size() == 0);
        REQUIRE(cloud.width == 0);
        REQUIRE(cloud.height == 0);
    }
    
    SECTION("Point cloud with reserved size") {
        PointCloudXYZ cloud(100);
        
        REQUIRE(cloud.empty());
        REQUIRE(cloud.size() == 0);
    }
}

TEST_CASE("PointCloud bounding box calculation", "[PointCloudTypes]") {
    SECTION("Empty cloud bounding box") {
        PointCloudXYZ cloud;
        auto [minPt, maxPt] = cloud.getBoundingBox();
        
        REQUIRE(minPt.x == 0.0f);
        REQUIRE(minPt.y == 0.0f);
        REQUIRE(minPt.z == 0.0f);
        REQUIRE(maxPt.x == 0.0f);
        REQUIRE(maxPt.y == 0.0f);
        REQUIRE(maxPt.z == 0.0f);
    }
    
    SECTION("Single point bounding box") {
        PointCloudXYZ cloud;
        cloud.push_back(Point3D(1.0f, 2.0f, 3.0f));
        
        auto [minPt, maxPt] = cloud.getBoundingBox();
        
        REQUIRE(minPt.x == 1.0f);
        REQUIRE(minPt.y == 2.0f);
        REQUIRE(minPt.z == 3.0f);
        REQUIRE(maxPt.x == 1.0f);
        REQUIRE(maxPt.y == 2.0f);
        REQUIRE(maxPt.z == 3.0f);
    }
    
    SECTION("Multiple points bounding box") {
        PointCloudXYZ cloud;
        cloud.push_back(Point3D(1.0f, 5.0f, 3.0f));
        cloud.push_back(Point3D(4.0f, 2.0f, 6.0f));
        cloud.push_back(Point3D(0.0f, 3.0f, 1.0f));
        
        auto [minPt, maxPt] = cloud.getBoundingBox();
        
        REQUIRE(minPt.x == 0.0f);
        REQUIRE(minPt.y == 2.0f);
        REQUIRE(minPt.z == 1.0f);
        REQUIRE(maxPt.x == 4.0f);
        REQUIRE(maxPt.y == 5.0f);
        REQUIRE(maxPt.z == 6.0f);
    }
    
    SECTION("Colored point cloud bounding box") {
        PointCloudXYZRGB cloud;
        cloud.push_back(PointXYZRGB(1.0f, 5.0f, 3.0f, 255, 0, 0));
        cloud.push_back(PointXYZRGB(4.0f, 2.0f, 6.0f, 0, 255, 0));
        cloud.push_back(PointXYZRGB(0.0f, 3.0f, 1.0f, 0, 0, 255));
        
        auto [minPt, maxPt] = cloud.getBoundingBox();
        
        REQUIRE(minPt.x == 0.0f);
        REQUIRE(minPt.y == 2.0f);
        REQUIRE(minPt.z == 1.0f);
        REQUIRE(maxPt.x == 4.0f);
        REQUIRE(maxPt.y == 5.0f);
        REQUIRE(maxPt.z == 6.0f);
    }
}

TEST_CASE("PointCloud iterator support", "[PointCloudTypes]") {
    SECTION("Range-based for loop") {
        PointCloudXYZ cloud;
        cloud.push_back(Point3D(1.0f, 2.0f, 3.0f));
        cloud.push_back(Point3D(4.0f, 5.0f, 6.0f));
        
        float sumX = 0.0f;
        for (const auto& point : cloud) {
            sumX += point.x;
        }
        
        REQUIRE(sumX == 5.0f); // 1.0 + 4.0
    }
    
    SECTION("STL algorithm compatibility") {
        PointCloudXYZ cloud;
        cloud.push_back(Point3D(3.0f, 1.0f, 2.0f));
        cloud.push_back(Point3D(1.0f, 3.0f, 1.0f));
        cloud.push_back(Point3D(2.0f, 2.0f, 3.0f));
        
        // Sort by x coordinate
        sort(cloud.begin(), cloud.end(), 
                 [](const Point3D& a, const Point3D& b) { return a.x < b.x; });
        
        REQUIRE(cloud[0].x == 1.0f);
        REQUIRE(cloud[1].x == 2.0f);
        REQUIRE(cloud[2].x == 3.0f);
    }
}

TEST_CASE("PointCloud realistic scenarios", "[PointCloudTypes]") {
    SECTION("Simulate laser scan data") {
        PointCloudXYZ cloud;
        
        // Simulate a simple laser scan pattern (horizontal sweep)
        for (int i = 0; i < 360; ++i) {
            float angle = static_cast<float>(i) * numbers::pi_v<float> / 180.0f; // Convert to radians
            float distance = 10.0f; // 10 meter range
            
            Point3D point;
            point.x = distance * cos(angle);
            point.y = distance * sin(angle);
            point.z = 0.0f; // Ground level
            
            cloud.push_back(point);
        }
        
        REQUIRE(cloud.size() == 360);
        
        auto [minPt, maxPt] = cloud.getBoundingBox();
        
        // Should form a circle with radius ~10
        REQUIRE(minPt.x == Approx(-10.0f).margin(0.1f));
        REQUIRE(maxPt.x == Approx(10.0f).margin(0.1f));
        REQUIRE(minPt.y == Approx(-10.0f).margin(0.1f));
        REQUIRE(maxPt.y == Approx(10.0f).margin(0.1f));
        REQUIRE(minPt.z == Approx(0.0f));
        REQUIRE(maxPt.z == Approx(0.0f));
    }
    
    SECTION("Simulate RGB-D camera data") {
        PointCloudXYZRGB cloud;
        
        // Simulate a 64x48 depth image with colors
        constexpr int width = 64;
        constexpr int height = 48;
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                // Simulate depth (closer in center, farther at edges)
                float centerX = static_cast<float>(width) / 2.0f;
                float centerY = static_cast<float>(height) / 2.0f;
                float xFloat = static_cast<float>(x);
                float yFloat = static_cast<float>(y);
                float distFromCenter = sqrt((xFloat - centerX) * (xFloat - centerX) + 
                                               (yFloat - centerY) * (yFloat - centerY));
                float depth = 1.0f + distFromCenter * 0.1f; // 1-5 meters
                
                // Convert to 3D coordinates (simplified camera model)
                Point3D pos;
                pos.x = (xFloat - centerX) * depth * 0.01f; // Scale factor
                pos.y = (yFloat - centerY) * depth * 0.01f;
                pos.z = depth;
                
                // Generate color based on position
                RGB color;
                color.r = static_cast<uint8_t>((x * 255) / width);
                color.g = static_cast<uint8_t>((y * 255) / height);
                color.b = static_cast<uint8_t>(128);
                
                cloud.push_back(PointXYZRGB(pos, color));
            }
        }
        
        REQUIRE(cloud.size() == width * height);
        REQUIRE(cloud.size() == 3072); // 64 * 48
        
        // Verify color distribution
        bool hasVariedColors = false;
        for (size_t i = 1; i < cloud.size(); ++i) {
            if (cloud[i].color.r != cloud[0].color.r || 
                cloud[i].color.g != cloud[0].color.g) {
                hasVariedColors = true;
                break;
            }
        }
        REQUIRE(hasVariedColors);
    }
}
