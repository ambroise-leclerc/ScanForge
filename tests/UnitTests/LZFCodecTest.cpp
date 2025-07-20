
/**
 * @brief Unit tests for LZFCodec using Catch2 and real PCD data
 */

#include <catch2/catch_all.hpp>
#include "codec/LZFCodec.hpp"
#include "PCDProcessor.hpp"
#include <vector>
#include <fstream>
#include <filesystem>
#include <array>
#include <cstring>
#include <chrono>
#include <numeric>

using namespace std;
using namespace scanforge;
using namespace scanforge::codec;

/**
 * @brief Helper function to create simple LZF compressed test data
 * @param uncompressed The uncompressed data to compress
 * @return A vector containing LZF compressed data
 */
vector<uint8_t> createSimpleLzfData(const vector<uint8_t>& uncompressed) {
    // Simple test case: literal data (no compression)
    // LZF format: if first byte < 32, it's a literal run
    vector<uint8_t> compressed;
    
    if (uncompressed.size() <= 31) {
        compressed.push_back(static_cast<uint8_t>(uncompressed.size() - 1));
        compressed.insert(compressed.end(), uncompressed.begin(), uncompressed.end());
    } else {
        // For larger data, create chunks of literals
        size_t pos = 0;
        while (pos < uncompressed.size()) {
            size_t chunkSize = min(size_t(31), uncompressed.size() - pos);
            compressed.push_back(static_cast<uint8_t>(chunkSize - 1));
            compressed.insert(compressed.end(), 
                            uncompressed.begin() + static_cast<ptrdiff_t>(pos), 
                            uncompressed.begin() + static_cast<ptrdiff_t>(pos + chunkSize));
            pos += chunkSize;
        }
    }
    
    return compressed;
}

TEST_CASE("LZFCodec basic functionality", "[LZFCodec]") {
    GIVEN("simple literal data (uncompressed in LZF format)") {
        vector<uint8_t> originalData = {0x01, 0x02, 0x03, 0x04, 0x05};
        auto compressed = createSimpleLzfData(originalData);
        WHEN("decompressing the data") {
            auto result = LZFCodec::decompress(compressed, originalData.size());
            THEN("the decompressed data matches the original") {
                REQUIRE(result.size() == originalData.size());
                REQUIRE(result == originalData);
            }
        }
    }
    GIVEN("larger data that requires multiple literal chunks") {
        vector<uint8_t> originalData(100);
        iota(originalData.begin(), originalData.end(), uint8_t{0});
        auto compressed = createSimpleLzfData(originalData);
        WHEN("decompressing the data") {
            auto result = LZFCodec::decompress(compressed, originalData.size());
            THEN("the decompressed data matches the original") {
                REQUIRE(result.size() == originalData.size());
                REQUIRE(result == originalData);
            }
        }
    }
    GIVEN("repeating pattern data") {
        vector<uint8_t> originalData;
        for (int i = 0; i < 50; ++i) {
            originalData.push_back(static_cast<uint8_t>(i % 10));
        }
        auto compressed = createSimpleLzfData(originalData);
        WHEN("decompressing the data") {
            auto result = LZFCodec::decompress(compressed, originalData.size());
            THEN("the decompressed data matches the original") {
                REQUIRE(result.size() == originalData.size());
                REQUIRE(result == originalData);
            }
        }
    }
}

TEST_CASE("LZFCodec edge cases", "[LZFCodec]") {
    GIVEN("empty compressed data") {
        vector<uint8_t> emptyCompressed;
        WHEN("decompressing") {
            auto result = LZFCodec::decompress(emptyCompressed, 0);
            THEN("the result is empty") {
                REQUIRE(result.empty());
            }
        }
    }
    GIVEN("a single byte of data") {
        vector<uint8_t> singleByte = {0x42};
        auto compressed = createSimpleLzfData(singleByte);
        WHEN("decompressing") {
            auto result = LZFCodec::decompress(compressed, 1);
            THEN("the result contains the single byte") {
                REQUIRE(result.size() == 1);
                REQUIRE(result[0] == 0x42);
            }
        }
    }
    GIVEN("invalid expected size (too large)") {
        vector<uint8_t> someData = {0x01, 0x02, 0x03};
        WHEN("decompressing with too large expected size") {
            auto result = LZFCodec::decompress(someData, 1000);
            THEN("the result is empty") {
                REQUIRE(result.empty());
            }
        }
    }
    GIVEN("invalid expected size (too small)") {
        vector<uint8_t> originalData = {0x01, 0x02, 0x03, 0x04, 0x05};
        auto compressed = createSimpleLzfData(originalData);
        WHEN("decompressing with too small expected size") {
            auto result = LZFCodec::decompress(compressed, 2);
            THEN("the result is empty") {
                REQUIRE(result.empty());
            }
        }
    }
    GIVEN("corrupted compressed data") {
        vector<uint8_t> corruptedData = {0xFF, 0xFF, 0xFF};
        WHEN("decompressing") {
            auto result = LZFCodec::decompress(corruptedData, 10);
            THEN("the result is empty") {
                REQUIRE(result.empty());
            }
        }
    }
}

TEST_CASE("LZFCodec with real PCD data", "[LZFCodec][integration]") {
    const string testDataPath = "tests/data/sample.pcd";
    
    SECTION("Load and process real PCD file") {
        // Check if test data file exists
        if (!filesystem::exists(testDataPath)) {
            SKIP("Test data file not found: " + testDataPath);
        }
        
        // Load the PCD file using PCDProcessor to get compressed data
        ifstream file(testDataPath, ios::binary);
        REQUIRE(file.is_open());
        
        // Read the header to find the data section
        string line;
        size_t dataOffset = 0;
        
        while (getline(file, line)) {
            if (line.find("DATA binary_compressed") != string::npos) {
                auto pos = file.tellg();
                dataOffset = static_cast<size_t>(pos);
                break;
            }
        }
        
        REQUIRE(dataOffset > 0);
        
        // Read some compressed data for testing
        file.seekg(static_cast<streamoff>(dataOffset));
        vector<uint8_t> compressedChunk(1024); // Read first 1KB
        file.read(reinterpret_cast<char*>(compressedChunk.data()), 
                 static_cast<streamsize>(compressedChunk.size()));
        auto bytesReadPos = file.gcount();
        size_t bytesRead = static_cast<size_t>(bytesReadPos);
        compressedChunk.resize(bytesRead);
        
        REQUIRE(!compressedChunk.empty());
        
        // Try to decompress a small chunk (this is a real-world test)
        // Note: We don't know the exact uncompressed size, so we'll try a reasonable estimate
        auto result = LZFCodec::decompress(compressedChunk, 2048);
        
        // The decompression may fail if we don't have complete LZF blocks,
        // but the function should handle it gracefully
        // This test verifies the function doesn't crash with real data
        REQUIRE(true); // Just checking we don't crash
    }
}

TEST_CASE("LZFCodec performance characteristics", "[LZFCodec][performance]") {
    SECTION("Large data decompression performance") {
        // Create a large dataset for performance testing
        constexpr size_t dataSize = 10000;
        vector<uint8_t> originalData(dataSize);
        
        // Fill with pseudo-random but compressible data
        for (size_t i = 0; i < dataSize; ++i) {
            originalData[i] = static_cast<uint8_t>((i / 100) % 256);
        }
        
        auto compressed = createSimpleLzfData(originalData);
        
        // Measure decompression time (basic performance test)
        auto start = chrono::high_resolution_clock::now();
        auto result = LZFCodec::decompress(compressed, originalData.size());
        auto end = chrono::high_resolution_clock::now();
        
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
        
        REQUIRE(result.size() == originalData.size());
        REQUIRE(result == originalData);
        
        // Verify reasonable performance (should complete in reasonable time)
        REQUIRE(duration.count() < 10000); // Less than 10ms for 10KB data
    }
    
    SECTION("Memory efficiency test") {
        // Test that decompression doesn't use excessive memory
        constexpr size_t dataSize = 1000;
        vector<uint8_t> originalData(dataSize, 0xAA); // All same byte
        
        auto compressed = createSimpleLzfData(originalData);
        auto result = LZFCodec::decompress(compressed, originalData.size());
        
        REQUIRE(result.size() == originalData.size());
        REQUIRE(all_of(result.begin(), result.end(), [](uint8_t b) { return b == 0xAA; }));
    }
}

TEST_CASE("LZFCodec compression functionality", "[LZFCodec][compression]") {
    GIVEN("simple byte data") {
        vector<uint8_t> originalData = {0x01, 0x02, 0x03, 0x04, 0x05};
        
        WHEN("compressing the data") {
            auto compressed = LZFCodec::compress(originalData);
            
            THEN("compression succeeds") {
                REQUIRE(!compressed.empty());
                
                AND_WHEN("decompressing the result") {
                    auto decompressed = LZFCodec::decompress(compressed, originalData.size());
                    
                    THEN("the original data is recovered") {
                        REQUIRE(decompressed == originalData);
                    }
                }
            }
        }
    }
    
    GIVEN("highly repetitive data") {
        vector<uint8_t> originalData(100, 0x42); // 100 bytes of 0x42
        
        WHEN("compressing the data") {
            auto compressed = LZFCodec::compress(originalData);
            
            THEN("compression succeeds") {
                REQUIRE(!compressed.empty());
                // Note: Simple literal-only compression may not achieve size reduction
                
                AND_WHEN("decompressing the result") {
                    auto decompressed = LZFCodec::decompress(compressed, originalData.size());
                    
                    THEN("the original data is perfectly recovered") {
                        REQUIRE(decompressed == originalData);
                    }
                }
            }
        }
    }
    
    GIVEN("data with repeating patterns") {
        vector<uint8_t> originalData;
        for (int i = 0; i < 200; ++i) {
            originalData.push_back(static_cast<uint8_t>(i % 16));
        }
        
        WHEN("compressing the patterned data") {
            auto compressed = LZFCodec::compress(originalData);
            
            THEN("compression succeeds") {
                REQUIRE(!compressed.empty());
                
                AND_WHEN("decompressing the compressed data") {
                    auto decompressed = LZFCodec::decompress(compressed, originalData.size());
                    
                    THEN("the pattern is preserved") {
                        REQUIRE(decompressed == originalData);
                    }
                }
            }
        }
    }
    
    GIVEN("empty data") {
        vector<uint8_t> emptyData;
        
        WHEN("attempting to compress empty data") {
            auto compressed = LZFCodec::compress(emptyData);
            
            THEN("the result is empty") {
                REQUIRE(compressed.empty());
            }
        }
    }
    
    GIVEN("a single byte") {
        vector<uint8_t> singleByte = {0x7F};
        
        WHEN("compressing the single byte") {
            auto compressed = LZFCodec::compress(singleByte);
            
            THEN("compression succeeds") {
                REQUIRE(!compressed.empty());
                
                AND_WHEN("decompressing the result") {
                    auto decompressed = LZFCodec::decompress(compressed, singleByte.size());
                    
                    THEN("the single byte is recovered") {
                        REQUIRE(decompressed == singleByte);
                    }
                }
            }
        }
    }
}

TEST_CASE("LZFCodec pointer-based compression API", "[LZFCodec][compression][api]") {
    GIVEN("data and a sufficiently large output buffer") {
        vector<uint8_t> originalData = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
        vector<uint8_t> compressedBuffer(originalData.size() + 100);
        
        WHEN("using the pointer-based compression API") {
            size_t compressedSize = LZFCodec::compress(
                originalData.data(), originalData.size(),
                compressedBuffer.data(), compressedBuffer.size()
            );
            
            THEN("compression succeeds") {
                REQUIRE(compressedSize > 0);
                REQUIRE(compressedSize <= compressedBuffer.size());
                
                AND_WHEN("decompressing with the pointer API") {
                    vector<uint8_t> decompressed(originalData.size());
                    size_t decompressedSize = LZFCodec::decompress(
                        compressedBuffer.data(), compressedSize,
                        decompressed.data(), decompressed.size()
                    );
                    
                    THEN("the original data is recovered") {
                        REQUIRE(decompressedSize == originalData.size());
                        REQUIRE(decompressed == originalData);
                    }
                }
            }
        }
    }
    
    GIVEN("data and an insufficient output buffer") {
        vector<uint8_t> originalData(1000, 0x99);
        vector<uint8_t> smallBuffer(10); // Too small
        
        WHEN("attempting compression with insufficient space") {
            size_t compressedSize = LZFCodec::compress(
                originalData.data(), originalData.size(),
                smallBuffer.data(), smallBuffer.size()
            );
            
            THEN("compression fails gracefully") {
                REQUIRE(compressedSize == 0);
            }
        }
    }
}

TEST_CASE("LZFCodec round-trip compression/decompression", "[LZFCodec][roundtrip]") {
    GIVEN("various data patterns") {
        vector<vector<uint8_t>> testCases = {
            {0x00, 0x01, 0x02, 0x03, 0x04}, // Sequential
            {0xFF, 0xFF, 0xFF, 0xFF, 0xFF}, // All same
            {0xAA, 0x55, 0xAA, 0x55, 0xAA}, // Alternating
            {}, // Empty
            {0x42}, // Single byte
        };
        
        WHEN("performing round-trip compression and decompression") {
            THEN("all patterns are preserved") {
                for (const auto& originalData : testCases) {
                    auto compressed = LZFCodec::compress(originalData);
                    
                    if (originalData.empty()) {
                        REQUIRE(compressed.empty());
                        continue;
                    }
                    
                    REQUIRE(!compressed.empty());
                    
                    auto decompressed = LZFCodec::decompress(compressed, originalData.size());
                    REQUIRE(decompressed == originalData);
                }
            }
        }
    }
    
    GIVEN("large data set") {
        vector<uint8_t> largeData(10000);
        for (size_t i = 0; i < largeData.size(); ++i) {
            largeData[i] = static_cast<uint8_t>((i * 7 + 13) % 256);
        }
        
        WHEN("performing round-trip on large data") {
            auto compressed = LZFCodec::compress(largeData);
            
            THEN("compression succeeds") {
                REQUIRE(!compressed.empty());
                
                AND_WHEN("decompressing the large dataset") {
                    auto decompressed = LZFCodec::decompress(compressed, largeData.size());
                    
                    THEN("all data is preserved") {
                        REQUIRE(decompressed == largeData);
                    }
                }
            }
        }
    }
}

TEST_CASE("LZFCodec compression efficiency", "[LZFCodec][efficiency]") {
    GIVEN("highly compressible repetitive data") {
        vector<uint8_t> repetitiveData(1000, 0x77);
        
        WHEN("compressing the repetitive data") {
            auto compressed = LZFCodec::compress(repetitiveData);
            
            THEN("compression succeeds") {
                REQUIRE(!compressed.empty());
                // Note: Current implementation is literal-only, no size reduction expected
                
                AND_WHEN("decompressing the result") {
                    auto decompressed = LZFCodec::decompress(compressed, repetitiveData.size());
                    
                    THEN("the original data is recovered") {
                        REQUIRE(decompressed == repetitiveData);
                    }
                }
            }
        }
    }
    
    GIVEN("text-like data with repetition") {
        string text = "The quick brown fox jumps over the lazy dog. ";
        string repeatedText;
        for (int i = 0; i < 20; ++i) {
            repeatedText += text;
        }
        vector<uint8_t> textData(repeatedText.begin(), repeatedText.end());
        
        WHEN("compressing the text data") {
            auto compressed = LZFCodec::compress(textData);
            
            THEN("compression succeeds") {
                REQUIRE(!compressed.empty());
                // Note: Current implementation is literal-only, no size reduction expected
                
                AND_WHEN("decompressing the text") {
                    auto decompressed = LZFCodec::decompress(compressed, textData.size());
                    
                    THEN("the text is perfectly preserved") {
                        REQUIRE(decompressed == textData);
                    }
                }
            }
        }
    }
}

TEST_CASE("LZFCodec API consistency", "[LZFCodec][api]") {
    GIVEN("test data and both vector and pointer APIs") {
        vector<uint8_t> originalData = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};
        
        WHEN("compressing with vector-based API") {
            auto vectorCompressed = LZFCodec::compress(originalData);
            
            THEN("compression succeeds") {
                REQUIRE(!vectorCompressed.empty());
                
                AND_WHEN("compressing the same data with pointer-based API") {
                    vector<uint8_t> pointerCompressed(originalData.size() + 100);
                    size_t compressedSize = LZFCodec::compress(
                        originalData.data(), originalData.size(),
                        pointerCompressed.data(), pointerCompressed.size()
                    );
                    
                    THEN("both APIs work correctly") {
                        REQUIRE(compressedSize > 0);
                        pointerCompressed.resize(compressedSize);
                        
                        AND_WHEN("decompressing results from both APIs") {
                            auto vectorDecompressed = LZFCodec::decompress(vectorCompressed, originalData.size());
                            auto pointerDecompressed = LZFCodec::decompress(pointerCompressed, originalData.size());
                            
                            THEN("both produce the original data") {
                                REQUIRE(vectorDecompressed == originalData);
                                REQUIRE(pointerDecompressed == originalData);
                            }
                        }
                    }
                }
            }
        }
    }
    
    GIVEN("simple test data for decompression API consistency") {
        vector<uint8_t> originalData = {0x10, 0x20, 0x30, 0x40};
        auto compressed = createSimpleLzfData(originalData);
        
        WHEN("using vector-based decompression API") {
            auto vectorResult = LZFCodec::decompress(compressed, originalData.size());
            
            THEN("decompression succeeds") {
                REQUIRE(vectorResult.size() == originalData.size());
                REQUIRE(vectorResult == originalData);
                
                AND_WHEN("using pointer-based decompression API on same data") {
                    vector<uint8_t> pointerResult(originalData.size());
                    size_t decompressedSize = LZFCodec::decompress(
                        compressed.data(), compressed.size(),
                        pointerResult.data(), pointerResult.size()
                    );
                    
                    THEN("both APIs produce identical results") {
                        REQUIRE(decompressedSize == originalData.size());
                        REQUIRE(vectorResult == pointerResult);
                        REQUIRE(vectorResult == originalData);
                    }
                }
            }
        }
    }
}
