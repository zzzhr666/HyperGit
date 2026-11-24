#include "compression.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <string_view>
#include <vector>

namespace {

std::vector<std::byte> bytes_of(const std::string_view value) {
    const auto* begin = reinterpret_cast<const std::byte*>(value.data());
    return {begin, begin + value.size()};
}

} // namespace

TEST(CompressionTest, CompressesAndDecompressesEmptyInput) {
    std::vector<std::byte> input;

    auto compressed = hyper::compression::compress(input);

    ASSERT_TRUE(compressed.has_value());
    auto decompressed = hyper::compression::decompress(*compressed);

    ASSERT_TRUE(decompressed.has_value());
    EXPECT_EQ(*decompressed, input);
}

TEST(CompressionTest, CompressesAndDecompressesText) {
    auto input = bytes_of("hello, HyperGit");

    auto compressed = hyper::compression::compress(input);

    ASSERT_TRUE(compressed.has_value());
    auto decompressed = hyper::compression::decompress(*compressed);

    ASSERT_TRUE(decompressed.has_value());
    EXPECT_EQ(*decompressed, input);
}

TEST(CompressionTest, CompressesAndDecompressesBinaryData) {
    std::vector<std::byte> input;
    for (int repetition = 0; repetition < 64; ++repetition) {
        input.insert(input.end(), {
            std::byte{0x00},
            std::byte{0x01},
            std::byte{0x7F},
            std::byte{0x80},
            std::byte{0xFF}
        });
    }

    auto compressed = hyper::compression::compress(input);

    ASSERT_TRUE(compressed.has_value());
    auto decompressed = hyper::compression::decompress(*compressed);

    ASSERT_TRUE(decompressed.has_value());
    EXPECT_EQ(*decompressed, input);
}

TEST(CompressionTest, DoesNotModifyInput) {
    auto input = bytes_of("input must remain unchanged");
    const auto original = input;

    auto compressed = hyper::compression::compress(input);

    ASSERT_TRUE(compressed.has_value());
    EXPECT_EQ(input, original);
}

TEST(CompressionTest, RejectsInvalidCompressedData) {
    auto invalid_data = bytes_of("this is not zstd data");

    const auto decompressed = hyper::compression::decompress(invalid_data);

    EXPECT_FALSE(decompressed.has_value());
}

TEST(CompressionTest, RejectsTruncatedCompressedData) {
    auto input = bytes_of("data that will be compressed");
    auto compressed = hyper::compression::compress(input);

    ASSERT_TRUE(compressed.has_value());
    ASSERT_FALSE(compressed->empty());
    compressed->resize(compressed->size() - 1);

    const auto decompressed = hyper::compression::decompress(*compressed);

    EXPECT_FALSE(decompressed.has_value());
}
