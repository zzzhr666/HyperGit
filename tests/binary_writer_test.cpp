#include "binary_writer.hpp"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <string_view>
#include <vector>

namespace {

std::vector<std::byte> bytes_of(const hyper::BinaryWriter& writer) {
    return {
        writer.data(),
        writer.data() + writer.size()
    };
}

} // namespace

TEST(BinaryWriterTest, WritesUint32InBigEndianOrder) {
    hyper::BinaryWriter writer;

    writer.write_uint32(0x01020304);

    const std::vector expected{
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03},
        std::byte{0x04}
    };

    EXPECT_EQ(bytes_of(writer), expected);
}

TEST(BinaryWriterTest, WritesUint64InBigEndianOrder) {
    hyper::BinaryWriter writer;

    writer.write_uint64(0x0102030405060708);

    const std::vector expected{
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03},
        std::byte{0x04},
        std::byte{0x05},
        std::byte{0x06},
        std::byte{0x07},
        std::byte{0x08}
    };

    EXPECT_EQ(bytes_of(writer), expected);
}

TEST(BinaryWriterTest, AppendsMultipleValuesInWriteOrder) {
    hyper::BinaryWriter writer;

    writer.write_uint32(0x01020304);
    writer.write_uint64(0x05060708090A0B0C);

    const std::vector expected{
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03},
        std::byte{0x04},
        std::byte{0x05},
        std::byte{0x06},
        std::byte{0x07},
        std::byte{0x08},
        std::byte{0x09},
        std::byte{0x0A},
        std::byte{0x0B},
        std::byte{0x0C}
    };

    EXPECT_EQ(bytes_of(writer), expected);
}

TEST(BinaryWriterTest, WritesRawBytesInOrder) {
    hyper::BinaryWriter writer;
    constexpr std::array input{
        std::byte{0xAA},
        std::byte{0xBB},
        std::byte{0xCC}
    };

    writer.write_bytes(input);

    const std::vector expected{
        std::byte{0xAA},
        std::byte{0xBB},
        std::byte{0xCC}
    };

    EXPECT_EQ(bytes_of(writer), expected);
}

TEST(BinaryWriterTest, AppendsRawBytesWithoutAddingLength) {
    hyper::BinaryWriter writer;
    constexpr std::array input{
        std::byte{0xAA},
        std::byte{0xBB}
    };

    writer.write_uint32(0x01020304);
    writer.write_bytes(input);
    writer.write_uint32(0x05060708);

    const std::vector expected{
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03},
        std::byte{0x04},
        std::byte{0xAA},
        std::byte{0xBB},
        std::byte{0x05},
        std::byte{0x06},
        std::byte{0x07},
        std::byte{0x08}
    };

    EXPECT_EQ(bytes_of(writer), expected);
}

TEST(BinaryWriterTest, WritingEmptyBytesDoesNotChangeBuffer) {
    hyper::BinaryWriter writer;
    constexpr std::array<std::byte, 0> input{};

    writer.write_uint32(0x01020304);
    const auto before = bytes_of(writer);

    writer.write_bytes(input);

    EXPECT_EQ(bytes_of(writer), before);
}

TEST(BinaryWriterTest, WritesStringWithByteLengthPrefix) {
    hyper::BinaryWriter writer;

    writer.write_string("abc");

    const std::vector expected{
        std::byte{0x00},
        std::byte{0x00},
        std::byte{0x00},
        std::byte{0x03},
        std::byte{'a'},
        std::byte{'b'},
        std::byte{'c'}
    };

    EXPECT_EQ(bytes_of(writer), expected);
}

TEST(BinaryWriterTest, WritesEmptyStringWithZeroLengthPrefix) {
    hyper::BinaryWriter writer;

    writer.write_string("");

    const std::vector expected{
        std::byte{0x00},
        std::byte{0x00},
        std::byte{0x00},
        std::byte{0x00}
    };

    EXPECT_EQ(bytes_of(writer), expected);
}

TEST(BinaryWriterTest, WritesUtf8StringWithByteLengthPrefix) {
    hyper::BinaryWriter writer;
    constexpr std::string_view input = "你好";

    writer.write_string(input);

    const std::vector expected{
        std::byte{0x00},
        std::byte{0x00},
        std::byte{0x00},
        std::byte{0x06},
        std::byte{0xE4},
        std::byte{0xBD},
        std::byte{0xA0},
        std::byte{0xE5},
        std::byte{0xA5},
        std::byte{0xBD}
    };

    EXPECT_EQ(bytes_of(writer), expected);
}

TEST(BinaryWriterTest, AppendsStringAfterExistingBytes) {
    hyper::BinaryWriter writer;

    writer.write_uint32(0x01020304);
    writer.write_string("abc");

    const std::vector expected{
        std::byte{0x01},
        std::byte{0x02},
        std::byte{0x03},
        std::byte{0x04},
        std::byte{0x00},
        std::byte{0x00},
        std::byte{0x00},
        std::byte{0x03},
        std::byte{'a'},
        std::byte{'b'},
        std::byte{'c'}
    };

    EXPECT_EQ(bytes_of(writer), expected);
}

TEST(BinaryWriterTest, ClearRemovesAllWrittenBytes) {
    hyper::BinaryWriter writer;
    writer.write_uint32(0x01020304);

    writer.clear();

    EXPECT_EQ(writer.size(), 0);
}
