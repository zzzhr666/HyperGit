#include "binary_reader.hpp"
#include "binary_writer.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace {

std::array<std::byte, 12> sample_bytes() {
    return {
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04},
        std::byte{0x05}, std::byte{0x06}, std::byte{0x07}, std::byte{0x08},
        std::byte{0x09}, std::byte{0x0A}, std::byte{0x0B}, std::byte{0x0C}
    };
}

} // namespace

TEST(BinaryReaderTest, ReadsUint32InBigEndianOrder) {
    const auto input = sample_bytes();
    hyper::BinaryReader reader(input);

    const auto result = reader.read_uint32();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 0x01020304U);
}

TEST(BinaryReaderTest, ReadsUint64InBigEndianOrder) {
    const auto input = sample_bytes();
    hyper::BinaryReader reader(input);

    const auto result = reader.read_uint64();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), 0x0102030405060708ULL);
}

TEST(BinaryReaderTest, ReadsValuesSequentially) {
    const auto input = sample_bytes();
    hyper::BinaryReader reader(input);

    const auto first = reader.read_uint32();
    const auto second = reader.read_uint64();

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first.value(), 0x01020304U);
    EXPECT_EQ(second.value(), 0x05060708090A0B0CULL);
}

TEST(BinaryReaderTest, ReadsSignedInt64Values) {
    hyper::BinaryWriter writer;
    writer.write_int64(-1);
    writer.write_int64(std::numeric_limits<std::int64_t>::min());
    writer.write_int64(std::numeric_limits<std::int64_t>::max());

    hyper::BinaryReader reader(
        std::span<const std::byte>(writer.data(), writer.size())
    );

    const auto first = reader.read_int64();
    const auto second = reader.read_int64();
    const auto third = reader.read_int64();

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    ASSERT_TRUE(third.has_value());
    EXPECT_EQ(first.value(), -1);
    EXPECT_EQ(second.value(), std::numeric_limits<std::int64_t>::min());
    EXPECT_EQ(third.value(), std::numeric_limits<std::int64_t>::max());
}

TEST(BinaryReaderTest, ReadsRawBytesAndAdvancesPosition) {
    const auto input = sample_bytes();
    hyper::BinaryReader reader(input);

    const auto result = reader.read_bytes(4);
    ASSERT_TRUE(result.has_value());
    const std::vector expected{
        std::byte{0x01}, std::byte{0x02},
        std::byte{0x03}, std::byte{0x04}
    };
    EXPECT_EQ(result.value(), expected);

    const auto next = reader.read_bytes(2);
    ASSERT_TRUE(next.has_value());
    const std::vector next_expected{
        std::byte{0x05}, std::byte{0x06}
    };
    EXPECT_EQ(next.value(), next_expected);
}

TEST(BinaryReaderTest, ReadsZeroRawBytesAsAnEngagedEmptyOptional) {
    hyper::BinaryReader reader(std::span<const std::byte>{});

    const auto result = reader.read_bytes(0);

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST(BinaryReaderTest, ReadsLengthPrefixedString) {
    constexpr std::array input{
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x03},
        std::byte{'a'}, std::byte{'b'}, std::byte{'c'}
    };
    hyper::BinaryReader reader(input);

    const auto result = reader.read_string();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "abc");
}

TEST(BinaryReaderTest, ReadsLengthPrefixedUtf8String) {
    constexpr std::array input{
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x06},
        std::byte{0xE4}, std::byte{0xBD}, std::byte{0xA0},
        std::byte{0xE5}, std::byte{0xA5}, std::byte{0xBD}
    };
    hyper::BinaryReader reader(input);

    const auto result = reader.read_string();

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "你好");
}

TEST(BinaryReaderTest, ReadsEmptyStringAsAnEngagedEmptyOptional) {
    constexpr std::array input{
        std::byte{0x00}, std::byte{0x00},
        std::byte{0x00}, std::byte{0x00}
    };
    hyper::BinaryReader reader(input);

    const auto result = reader.read_string();

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result.value().empty());
}

TEST(BinaryReaderTest, ReadsStringsSequentially) {
    constexpr std::array input{
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x03},
        std::byte{'a'}, std::byte{'b'}, std::byte{'c'},
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x02},
        std::byte{'d'}, std::byte{'e'}
    };
    hyper::BinaryReader reader(input);

    const auto first = reader.read_string();
    const auto second = reader.read_string();

    ASSERT_TRUE(first.has_value());
    ASSERT_TRUE(second.has_value());
    EXPECT_EQ(first.value(), "abc");
    EXPECT_EQ(second.value(), "de");
}

TEST(BinaryReaderTest, FailsWhenUint32IsTruncated) {
    constexpr std::array input{
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}
    };
    hyper::BinaryReader reader(input);

    EXPECT_FALSE(reader.read_uint32().has_value());
}

TEST(BinaryReaderTest, FailsWhenUint64IsTruncated) {
    constexpr std::array input{
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03},
        std::byte{0x04}, std::byte{0x05}, std::byte{0x06},
        std::byte{0x07}
    };
    hyper::BinaryReader reader(input);

    EXPECT_FALSE(reader.read_uint64().has_value());
}

TEST(BinaryReaderTest, FailsWhenRawBytesAreTruncated) {
    constexpr std::array input{
        std::byte{0x01}, std::byte{0x02}, std::byte{0x03}
    };
    hyper::BinaryReader reader(input);

    EXPECT_FALSE(reader.read_bytes(4).has_value());
}

TEST(BinaryReaderTest, FailsWhenStringIsTruncatedWithoutAdvancing) {
    constexpr std::array input{
        std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x04},
        std::byte{'a'}, std::byte{'b'}, std::byte{'c'}
    };
    hyper::BinaryReader reader(input);

    EXPECT_FALSE(reader.read_string().has_value());

    const auto remaining = reader.read_bytes(input.size());
    ASSERT_TRUE(remaining.has_value());
    EXPECT_EQ(remaining.value(), std::vector(input.begin(), input.end()));
}
