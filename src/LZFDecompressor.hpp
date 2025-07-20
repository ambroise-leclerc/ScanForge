#pragma once

#include <vector>
#include <cstdint>
#include <iterator>
#include <concepts>
#include <numeric>

namespace scanforge {

/**
 * @brief LZF decompressor for PCD binary_compressed format.
 * Based on Marc Lehmann's LZF compression algorithm.
 * Reference: http://oldhome.schmorp.de/marc/liblzf.html
 */
class LZFDecompressor {
public:
    /**
     * @brief Decompress LZF compressed data using raw pointers.
     * @param compressed Pointer to the start of the compressed data buffer.
     * @param compressedSize Size of the compressed data.
     * @param uncompressed Pointer to the start of the output buffer for uncompressed data.
     * @param uncompressedSize The capacity of the output buffer.
     * @return The number of bytes decompressed, or 0 on failure.
     */
    static size_t decompress(const uint8_t* compressed, size_t compressedSize,
                           uint8_t* uncompressed, size_t uncompressedSize) {
        return decompress_generic(compressed, compressed + compressedSize,
                                  uncompressed, uncompressed + uncompressedSize);
    }

    /**
     * @brief Decompress LZF data from a vector into a new vector.
     * @param compressed A vector containing the compressed data.
     * @param expectedSize The expected size of the uncompressed data.
     * @return A vector with the decompressed data, or an empty vector on failure or size mismatch.
     */
    static std::vector<uint8_t> decompress(const std::vector<uint8_t>& compressed, size_t expectedSize) {
        std::vector<uint8_t> uncompressed(expectedSize);
        size_t decompressedSize = decompress_generic(compressed.begin(), compressed.end(),
                                                     uncompressed.begin(), uncompressed.end());

        if (decompressedSize != expectedSize) {
            return {}; // Decompression failed or size mismatch
        }

        return uncompressed;
    }

private:
    /**
     * @brief Generic C++23-style LZF decompression implementation.
     *
     * This function uses iterators and concepts to provide a generic implementation
     * of the LZF decompression algorithm. It's designed to work with any
     * compatible input and output iterators.
     *
     * @tparam In An input iterator type.
     * @tparam InEnd A sentinel for the input iterator.
     * @tparam Out A random access iterator type for the output.
     * @param ip The beginning of the input range.
     * @param in_end The end of the input range.
     * @param op The beginning of the output range.
     * @param out_end The end of the output range.
     * @return The number of bytes written to the output range, or 0 on error.
     */
    template <std::input_iterator In, std::sentinel_for<In> InEnd, std::random_access_iterator Out>
    requires std::convertible_to<std::iter_value_t<In>, uint8_t> &&
             std::is_assignable_v<decltype(*std::declval<Out&>()), uint8_t>
    static size_t decompress_generic(In ip, InEnd in_end, Out op, Out out_end) {
        const Out out_start = op;

        while (ip != in_end && op != out_end) {
            const uint8_t ctrl = *ip++;

            if (ctrl < 32) { // Literal run: ctrl + 1 bytes
                const size_t run_len = ctrl + 1;

                if (static_cast<size_t>(std::distance(op, out_end)) < run_len ||
                    static_cast<size_t>(std::distance(ip, in_end)) < run_len) {
                    return 0; // Error: not enough data or space
                }

                for (size_t i = 0; i < run_len; ++i) {
                    *op++ = *ip++;
                }
            } else { // Back reference
                uint32_t len = ctrl >> 5;

                if (ip == in_end) return 0; // Error: truncated input

                if (len == 7) { // Extended length
                    len += *ip++;
                    if (ip == in_end) return 0; // Error: truncated input
                }

                const size_t offset = static_cast<size_t>(((ctrl & 0x1f) << 8) + *ip++ + 1);

                if (static_cast<size_t>(std::distance(out_start, op)) < offset) {
                    return 0; // Error: invalid back reference
                }
                Out ref = op - static_cast<typename std::iterator_traits<Out>::difference_type>(offset);

                const size_t copy_len = len + 2;
                if (static_cast<size_t>(std::distance(op, out_end)) < copy_len) {
                    return 0; // Error: not enough space for back reference
                }

                for (size_t i = 0; i < copy_len; ++i) {
                    *op++ = *ref++;
                }
            }
        }
        return static_cast<size_t>(std::distance(out_start, op));
    }
};

} // namespace scanforge