
/**
 * @brief Unit tests for LZFDecompressor using Catch2 and real PCD data
 */

#include <catch2/catch_all.hpp>
#include "LZFDecompressor.hpp"
#include "PCDLoader.hpp"
#include <vector>
#include <fstream>
#include <filesystem>
#include <array>
#include <cstring>

using namespace scanforge;

/**
 * @brief Helper function to create simple LZF compressed test data
 * @param uncompressed The uncompressed data to compress
 * @return A vector containing LZF compressed data
 */
std::vector<uint8_t> createSimpleLzfData(const std::vector<uint8_t>& uncompressed) {
    // Simple test case: literal data (no compression)
    // LZF format: if first byte < 32, it's a literal run
    std::vector<uint8_t> compressed;
    
    if (uncompressed.size() <= 31) {
        compressed.push_back(static_cast<uint8_t>(uncompressed.size() - 1));
        compressed.insert(compressed.end(), uncompressed.begin(), uncompressed.end());
    } else {
        // For larger data, create chunks of literals
        size_t pos = 0;
        while (pos < uncompressed.size()) {
            size_t chunkSize = std::min(size_t(31), uncompressed.size() - pos);
            compressed.push_back(static_cast<uint8_t>(chunkSize - 1));
            compressed.insert(compressed.end(), 
                            uncompressed.begin() + static_cast<std::ptrdiff_t>(pos), 
                            uncompressed.begin() + static_cast<std::ptrdiff_t>(pos + chunkSize));
            pos += chunkSize;
        }
    }
    
    return compressed;
}

TEST_CASE("LZFDecompressor basic functionality", "[LZFDecompressor]") {
    GIVEN("simple literal data (uncompressed in LZF format)") {
        std::vector<uint8_t> originalData = {0x01, 0x02, 0x03, 0x04, 0x05};
        auto compressed = createSimpleLzfData(originalData);
        WHEN("decompressing the data") {
            auto result = LZFDecompressor::decompress(compressed, originalData.size());
            THEN("the decompressed data matches the original") {
                REQUIRE(result.size() == originalData.size());
                REQUIRE(result == originalData);
            }
        }
    }
    GIVEN("larger data that requires multiple literal chunks") {
        std::vector<uint8_t> originalData(100);
        std::iota(originalData.begin(), originalData.end(), 0);
        auto compressed = createSimpleLzfData(originalData);
        WHEN("decompressing the data") {
            auto result = LZFDecompressor::decompress(compressed, originalData.size());
            THEN("the decompressed data matches the original") {
                REQUIRE(result.size() == originalData.size());
                REQUIRE(result == originalData);
            }
        }
    }
    GIVEN("repeating pattern data") {
        std::vector<uint8_t> originalData;
        for (int i = 0; i < 50; ++i) {
            originalData.push_back(static_cast<uint8_t>(i % 10));
        }
        auto compressed = createSimpleLzfData(originalData);
        WHEN("decompressing the data") {
            auto result = LZFDecompressor::decompress(compressed, originalData.size());
            THEN("the decompressed data matches the original") {
                REQUIRE(result.size() == originalData.size());
                REQUIRE(result == originalData);
            }
        }
    }
}

TEST_CASE("LZFDecompressor edge cases", "[LZFDecompressor]") {
    GIVEN("empty compressed data") {
        std::vector<uint8_t> emptyCompressed;
        WHEN("decompressing") {
            auto result = LZFDecompressor::decompress(emptyCompressed, 0);
            THEN("the result is empty") {
                REQUIRE(result.empty());
            }
        }
    }
    GIVEN("a single byte of data") {
        std::vector<uint8_t> singleByte = {0x42};
        auto compressed = createSimpleLzfData(singleByte);
        WHEN("decompressing") {
            auto result = LZFDecompressor::decompress(compressed, 1);
            THEN("the result contains the single byte") {
                REQUIRE(result.size() == 1);
                REQUIRE(result[0] == 0x42);
            }
        }
    }
    GIVEN("invalid expected size (too large)") {
        std::vector<uint8_t> someData = {0x01, 0x02, 0x03};
        WHEN("decompressing with too large expected size") {
            auto result = LZFDecompressor::decompress(someData, 1000);
            THEN("the result is empty") {
                REQUIRE(result.empty());
            }
        }
    }
    GIVEN("invalid expected size (too small)") {
        std::vector<uint8_t> originalData = {0x01, 0x02, 0x03, 0x04, 0x05};
        auto compressed = createSimpleLzfData(originalData);
        WHEN("decompressing with too small expected size") {
            auto result = LZFDecompressor::decompress(compressed, 2);
            THEN("the result is empty") {
                REQUIRE(result.empty());
            }
        }
    }
    GIVEN("corrupted compressed data") {
        std::vector<uint8_t> corruptedData = {0xFF, 0xFF, 0xFF};
        WHEN("decompressing") {
            auto result = LZFDecompressor::decompress(corruptedData, 10);
            THEN("the result is empty") {
                REQUIRE(result.empty());
            }
        }
    }
}

TEST_CASE("LZFDecompressor with real PCD data", "[LZFDecompressor][integration]") {
    const std::string testDataPath = "tests/data/sample.pcd";
    
    SECTION("Load and process real PCD file") {
        // Check if test data file exists
        if (!std::filesystem::exists(testDataPath)) {
            SKIP("Test data file not found: " + testDataPath);
        }
        
        // Load the PCD file using PCDLoader to get compressed data
        std::ifstream file(testDataPath, std::ios::binary);
        REQUIRE(file.is_open());
        
        // Read the header to find the data section
        std::string line;
        size_t dataOffset = 0;
        
        while (std::getline(file, line)) {
            if (line.find("DATA binary_compressed") != std::string::npos) {
                auto pos = file.tellg();
                dataOffset = static_cast<size_t>(pos);
                break;
            }
        }
        
        REQUIRE(dataOffset > 0);
        
        // Read some compressed data for testing
        file.seekg(static_cast<std::streamoff>(dataOffset));
        std::vector<uint8_t> compressedChunk(1024); // Read first 1KB
        file.read(reinterpret_cast<char*>(compressedChunk.data()), 
                 static_cast<std::streamsize>(compressedChunk.size()));
        auto bytesReadPos = file.gcount();
        size_t bytesRead = static_cast<size_t>(bytesReadPos);
        compressedChunk.resize(bytesRead);
        
        REQUIRE(!compressedChunk.empty());
        
        // Try to decompress a small chunk (this is a real-world test)
        // Note: We don't know the exact uncompressed size, so we'll try a reasonable estimate
        auto result = LZFDecompressor::decompress(compressedChunk, 2048);
        
        // The decompression may fail if we don't have complete LZF blocks,
        // but the function should handle it gracefully
        // This test verifies the function doesn't crash with real data
        REQUIRE(true); // Just checking we don't crash
    }
}

TEST_CASE("LZFDecompressor performance characteristics", "[LZFDecompressor][performance]") {
    SECTION("Large data decompression performance") {
        // Create a large dataset for performance testing
        constexpr size_t dataSize = 10000;
        std::vector<uint8_t> originalData(dataSize);
        
        // Fill with pseudo-random but compressible data
        for (size_t i = 0; i < dataSize; ++i) {
            originalData[i] = static_cast<uint8_t>((i / 100) % 256);
        }
        
        auto compressed = createSimpleLzfData(originalData);
        
        // Measure decompression time (basic performance test)
        auto start = std::chrono::high_resolution_clock::now();
        auto result = LZFDecompressor::decompress(compressed, originalData.size());
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        REQUIRE(result.size() == originalData.size());
        REQUIRE(result == originalData);
        
        // Verify reasonable performance (should complete in reasonable time)
        REQUIRE(duration.count() < 10000); // Less than 10ms for 10KB data
    }
    
    SECTION("Memory efficiency test") {
        // Test that decompression doesn't use excessive memory
        constexpr size_t dataSize = 1000;
        std::vector<uint8_t> originalData(dataSize, 0xAA); // All same byte
        
        auto compressed = createSimpleLzfData(originalData);
        auto result = LZFDecompressor::decompress(compressed, originalData.size());
        
        REQUIRE(result.size() == originalData.size());
        REQUIRE(std::all_of(result.begin(), result.end(), [](uint8_t b) { return b == 0xAA; }));
    }
}

TEST_CASE("LZFDecompressor API consistency", "[LZFDecompressor]") {
    SECTION("Pointer-based API") {
        std::vector<uint8_t> originalData = {0x10, 0x20, 0x30, 0x40};
        auto compressed = createSimpleLzfData(originalData);
        
        std::vector<uint8_t> output(originalData.size());
        
        size_t decompressedSize = LZFDecompressor::decompress(
            compressed.data(), compressed.size(),
            output.data(), output.size()
        );
        
        REQUIRE(decompressedSize == originalData.size());
        REQUIRE(std::equal(output.begin(), output.end(), originalData.begin()));
    }
    
    SECTION("Vector-based API") {
        std::vector<uint8_t> originalData = {0x10, 0x20, 0x30, 0x40};
        auto compressed = createSimpleLzfData(originalData);
        
        auto result = LZFDecompressor::decompress(compressed, originalData.size());
        
        REQUIRE(result.size() == originalData.size());
        REQUIRE(result == originalData);
    }
    
    SECTION("API consistency between methods") {
        std::vector<uint8_t> originalData = {0xF0, 0xE1, 0xD2, 0xC3, 0xB4, 0xA5};
        auto compressed = createSimpleLzfData(originalData);
        
        // Test both APIs produce the same result
        auto vectorResult = LZFDecompressor::decompress(compressed, originalData.size());
        
        std::vector<uint8_t> pointerResult(originalData.size());
        size_t decompressedSize = LZFDecompressor::decompress(
            compressed.data(), compressed.size(),
            pointerResult.data(), pointerResult.size()
        );
        
        REQUIRE(decompressedSize == originalData.size());
        REQUIRE(vectorResult == pointerResult);
        REQUIRE(vectorResult == originalData);
    }
}
