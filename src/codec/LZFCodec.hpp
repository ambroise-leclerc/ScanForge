#pragma once

#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>

namespace scanforge::codec {

/**
 * @brief Modern LZF codec for PCD binary_compressed format.
 * Based on Marc Lehmann's LZF compression algorithm.
 * Reference: http://oldhome.schmorp.de/marc/liblzf.html
 */
class LZFCodec {
   public:
    /**
     * @brief Decompress LZF compressed data.
     * @param compressed Span of compressed data
     * @param output Span of output buffer
     * @return Number of bytes decompressed, or 0 on failure
     */
    static size_t decompress(std::span<const uint8_t> compressed, std::span<uint8_t> output) {
        auto ip = compressed.begin();
        auto op = output.begin();
        const auto in_end = compressed.end();
        const auto out_end = output.end();
        const auto out_start = op;

        while (ip != in_end && op != out_end) {
            const uint8_t ctrl = *ip++;

            if (ctrl < 32) {  // Literal run: ctrl + 1 bytes
                const size_t run_len = ctrl + 1;

                if (std::distance(op, out_end) < static_cast<ptrdiff_t>(run_len) || std::distance(ip, in_end) < static_cast<ptrdiff_t>(run_len)) {
                    return 0;  // Error: not enough data or space
                }

                std::copy_n(ip, run_len, op);
                ip += static_cast<ptrdiff_t>(run_len);
                op += static_cast<ptrdiff_t>(run_len);
            } else {  // Back reference
                uint32_t len = ctrl >> 5;

                if (ip == in_end)
                    return 0;  // Error: truncated input

                if (len == 7) {  // Extended length
                    len += *ip++;
                    if (ip == in_end)
                        return 0;  // Error: truncated input
                }

                const size_t offset = static_cast<size_t>(((ctrl & 0x1f) << 8) + *ip++ + 1);

                if (std::distance(out_start, op) < static_cast<ptrdiff_t>(offset)) {
                    return 0;  // Error: invalid back reference
                }

                auto ref = op - static_cast<ptrdiff_t>(offset);
                const size_t copy_len = len + 2;

                if (std::distance(op, out_end) < static_cast<ptrdiff_t>(copy_len)) {
                    return 0;  // Error: not enough space for back reference
                }

                // Copy with potential overlap
                for (size_t i = 0; i < copy_len; ++i) {
                    *op++ = *ref++;
                }
            }
        }
        return static_cast<size_t>(std::distance(out_start, op));
    }

    /**
     * @brief Decompress LZF data from a vector.
     * @param compressed Vector containing compressed data
     * @param expectedSize Expected size of uncompressed data
     * @return Vector with decompressed data, or empty vector on failure
     */
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed, size_t expectedSize) {
        std::vector<uint8_t> uncompressed(expectedSize);
        size_t decompressedSize = decompress(compressed, uncompressed);

        if (decompressedSize != expectedSize) {
            return {};  // Decompression failed or size mismatch
        }

        return uncompressed;
    }

    /**
     * @brief Compress data using LZF algorithm.
     * @param input Span of input data
     * @param output Span of output buffer
     * @return Number of bytes compressed, or 0 on failure
     */
    static size_t compress(std::span<const uint8_t> input, std::span<uint8_t> output) {
        auto ip = input.begin();
        auto op = output.begin();
        const auto in_end = input.end();
        const auto out_end = output.end();
        const auto out_start = op;

        // Simple literal-only compression for reliability
        while (ip != in_end) {
            // Collect literals into runs (max 31 per run)
            const auto literal_start = ip;
            size_t literal_count = 0;

            while (ip != in_end && literal_count < 31) {
                ++ip;
                ++literal_count;
            }

            // Check if we have enough output space
            if (std::distance(op, out_end) < static_cast<ptrdiff_t>(literal_count + 1)) {
                return 0;  // Not enough space
            }

            // Output literal run header
            *op++ = static_cast<uint8_t>(literal_count - 1);

            // Copy literal bytes
            op = std::copy(literal_start, ip, op);
        }

        return static_cast<size_t>(std::distance(out_start, op));
    }

    /**
     * @brief Compress data from a vector.
     * @param uncompressed Vector containing uncompressed data
     * @return Vector with compressed data, or empty vector on failure
     */
    static std::vector<uint8_t> compress(const std::vector<uint8_t>& uncompressed) {
        // Allocate buffer with extra space for worst-case scenario
        size_t maxCompressedSize = uncompressed.size() + (uncompressed.size() >> 3) + 16;
        std::vector<uint8_t> compressed(maxCompressedSize);

        size_t compressedSize = compress(uncompressed, compressed);

        if (compressedSize == 0) {
            return {};  // Compression failed
        }

        compressed.resize(compressedSize);
        return compressed;
    }

    // Legacy pointer-based interface for compatibility
    static size_t decompress(const uint8_t* compressed, size_t compressedSize, uint8_t* uncompressed, size_t uncompressedSize) {
        return decompress(std::span{compressed, compressedSize}, std::span{uncompressed, uncompressedSize});
    }

    static size_t compress(const uint8_t* uncompressed, size_t uncompressedSize, uint8_t* compressed, size_t compressedCapacity) {
        return compress(std::span{uncompressed, uncompressedSize}, std::span{compressed, compressedCapacity});
    }
};

}  // namespace scanforge::codec
