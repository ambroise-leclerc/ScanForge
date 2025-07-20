#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

namespace scanforge {

/**
 * @brief 3D Point structure
 */
struct Point3D {
    float x, y, z;

    Point3D() = default;
    Point3D(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Point3D operator+(const Point3D& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Point3D operator-(const Point3D& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Point3D operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }
    float dot(const Point3D& other) const { return x * other.x + y * other.y + z * other.z; }
    float magnitude() const { return std::sqrt(x * x + y * y + z * z); }

    Point3D normalize() const {
        float mag = magnitude();
        return mag > 0 ? *this * (1.0f / mag) : Point3D{};
    }
};

/**
 * @brief RGB Color structure
 */
struct RGB {
    uint8_t r, g, b;

    RGB() = default;
    RGB(uint8_t r_, uint8_t g_, uint8_t b_) : r(r_), g(g_), b(b_) {}

    // Convert from packed RGB
    explicit RGB(uint32_t packed) {
        r = (packed >> 16) & 0xFF;
        g = (packed >> 8) & 0xFF;
        b = packed & 0xFF;
    }

    // Convert to packed RGB
    uint32_t toPacked() const { return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b); }
};

/**
 * @brief Point with color
 */
struct PointXYZRGB {
    Point3D position;
    RGB color;

    PointXYZRGB() = default;
    PointXYZRGB(const Point3D& pos, const RGB& col) : position(pos), color(col) {}
    PointXYZRGB(float x, float y, float z, uint8_t r, uint8_t g, uint8_t b) : position(x, y, z), color(r, g, b) {}
};

/**
 * @brief Point cloud data structure
 */
template <typename PointT>
class PointCloud {
   public:
    std::vector<PointT> points;
    uint32_t width = 0;
    uint32_t height = 0;
    bool is_dense = true;

    PointCloud() = default;
    explicit PointCloud(size_t reserve_size) { points.reserve(reserve_size); }

    void clear() {
        points.clear();
        width = 0;
        height = 0;
        is_dense = true;
    }

    size_t size() const { return points.size(); }
    bool empty() const { return points.empty(); }
    void push_back(const PointT& point) { points.push_back(point); }
    PointT& operator[](size_t idx) { return points[idx]; }
    const PointT& operator[](size_t idx) const { return points[idx]; }
    auto begin() { return points.begin(); }
    auto end() { return points.end(); }
    auto begin() const { return points.begin(); }
    auto end() const { return points.end(); }

    // Calculate bounding box
    std::pair<Point3D, Point3D> getBoundingBox() const {
        if (points.empty()) {
            return {
                {0, 0, 0},
                {0, 0, 0}
            };
        }

        Point3D min_pt = getPosition(points[0]);
        Point3D max_pt = min_pt;

        for (const auto& point : points) {
            Point3D pos = getPosition(point);
            min_pt.x = std::min(min_pt.x, pos.x);
            min_pt.y = std::min(min_pt.y, pos.y);
            min_pt.z = std::min(min_pt.z, pos.z);
            max_pt.x = std::max(max_pt.x, pos.x);
            max_pt.y = std::max(max_pt.y, pos.y);
            max_pt.z = std::max(max_pt.z, pos.z);
        }

        return {min_pt, max_pt};
    }

   private:
    // Helper function to extract position from different point types
    Point3D getPosition(const Point3D& point) const { return point; }

    Point3D getPosition(const PointXYZRGB& point) const { return point.position; }
};

// Type aliases for common point cloud types
using PointCloudXYZ = PointCloud<Point3D>;
using PointCloudXYZRGB = PointCloud<PointXYZRGB>;

}  // namespace scanforge
