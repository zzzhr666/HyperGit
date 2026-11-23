#include "cache_codec.hpp"

#include <gtest/gtest.h>

#include "binary_reader.hpp"
#include "binary_writer.hpp"
#include "cache.hpp"
#include "sha256.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace {

hyper::fs::path path_from_utf8(const std::string& value) {
    return hyper::fs::path(
        std::u8string(
            reinterpret_cast<const char8_t*>(value.data()),
            value.size()
        )
    );
}

hyper::CacheHeader sample_header() {
    hyper::CacheHeader header;
    header.signature = hyper::FormatSignature;
    header.version = 1;
    header.entry_count = 2;

    for (std::size_t i = 0; i < header.index_checksum.size(); ++i) {
        header.index_checksum[i] = static_cast<std::byte>(i);
    }

    return header;
}

std::vector<std::byte> bytes_of(const hyper::BinaryWriter& writer) {
    return {
        writer.data(),
        writer.data() + writer.size()
    };
}

hyper::CacheEntry sample_entry(std::string name = "src/你好.txt") {
    hyper::CacheEntry entry{};
    entry.modified_time_ns = -123456789012345678;
    entry.file_size = 0x0102030405060708ULL;
    entry.path = path_from_utf8(name);

    for (std::size_t i = 0; i < entry.obj_id.size(); ++i) {
        entry.obj_id[i] = static_cast<std::byte>(i);
    }

    return entry;
}

} // namespace

TEST(CacheCodecTest, WritesHeaderInDefinedOrder) {
    hyper::BinaryWriter writer;

    hyper::cache::encode_header(writer, sample_header());

    const auto actual = bytes_of(writer);
    ASSERT_EQ(actual.size(), 44U);

    EXPECT_EQ(actual[0], std::byte{'H'});
    EXPECT_EQ(actual[1], std::byte{'P'});
    EXPECT_EQ(actual[2], std::byte{'G'});
    EXPECT_EQ(actual[3], std::byte{'T'});

    EXPECT_EQ(actual[4], std::byte{0x00});
    EXPECT_EQ(actual[5], std::byte{0x00});
    EXPECT_EQ(actual[6], std::byte{0x00});
    EXPECT_EQ(actual[7], std::byte{0x01});

    EXPECT_EQ(actual[8], std::byte{0x00});
    EXPECT_EQ(actual[9], std::byte{0x00});
    EXPECT_EQ(actual[10], std::byte{0x00});
    EXPECT_EQ(actual[11], std::byte{0x02});

    for (std::size_t i = 0; i < 32; ++i) {
        EXPECT_EQ(actual[12 + i], static_cast<std::byte>(i));
    }
}

TEST(CacheCodecTest, ReadsHeaderWrittenByWriter) {
    hyper::BinaryWriter writer;
    const auto expected = sample_header();

    hyper::cache::encode_header(writer, expected);

    hyper::BinaryReader reader(
        std::span<const std::byte>(writer.data(), writer.size())
    );
    const auto actual = hyper::cache::decode_header(reader);

    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(actual.value().signature, expected.signature);
    EXPECT_EQ(actual.value().version, expected.version);
    EXPECT_EQ(actual.value().entry_count, expected.entry_count);
    EXPECT_EQ(actual.value().index_checksum, expected.index_checksum);
}

TEST(CacheCodecTest, FailsWhenHeaderIsTruncated) {
    hyper::BinaryWriter writer;
    hyper::cache::encode_header(writer, sample_header());

    auto bytes = bytes_of(writer);
    bytes.pop_back();

    hyper::BinaryReader reader(bytes);
    EXPECT_FALSE(hyper::cache::decode_header(reader).has_value());
}

TEST(CacheCodecTest, WritesAndReadsEntryWithoutChangingItsFields) {
    hyper::BinaryWriter writer;
    const auto expected = sample_entry();

    ASSERT_TRUE(hyper::cache::encode_entry(writer, expected));

    hyper::BinaryReader reader(
        std::span<const std::byte>(writer.data(), writer.size())
    );
    const auto actual = hyper::cache::decode_entry(reader);

    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(actual.value().modified_time_ns, expected.modified_time_ns);
    EXPECT_EQ(actual.value().file_size, expected.file_size);
    EXPECT_EQ(actual.value().obj_id, expected.obj_id);
    EXPECT_EQ(actual.value().path, expected.path);
}

TEST(CacheCodecTest, FailsWhenEntryHasEmptyName) {
    hyper::BinaryWriter writer;
    const auto expected = sample_entry("");

    ASSERT_FALSE(hyper::cache::encode_entry(writer, expected));

    hyper::BinaryReader reader(
        std::span<const std::byte>(writer.data(), writer.size())
    );
    EXPECT_FALSE(hyper::cache::decode_entry(reader).has_value());
}

TEST(CacheCodecTest, RejectsAbsoluteEntryPath) {
    hyper::CacheEntry entry = sample_entry();
    entry.path = hyper::fs::path("/absolute/path.txt");

    hyper::BinaryWriter writer;

    EXPECT_FALSE(hyper::cache::encode_entry(writer, entry));
}

TEST(CacheCodecTest, RejectsEntryPathThatEscapesRepository) {
    hyper::CacheEntry entry = sample_entry();
    entry.path = hyper::fs::path("../outside.txt");

    hyper::BinaryWriter writer;

    EXPECT_FALSE(hyper::cache::encode_entry(writer, entry));
}

TEST(CacheCodecTest, FailsWhenEntryIsTruncated) {
    hyper::BinaryWriter writer;
    ASSERT_TRUE(hyper::cache::encode_entry(writer, sample_entry()));

    auto bytes = bytes_of(writer);
    ASSERT_FALSE(bytes.empty());
    bytes.pop_back();

    hyper::BinaryReader reader(bytes);
    EXPECT_FALSE(hyper::cache::decode_entry(reader).has_value());
}

TEST(CacheCodecTest, WritesCacheWithEntryCountAndChecksum) {
    hyper::CacheIndex cache_index;
    cache_index.header.signature = hyper::FormatSignature;
    cache_index.header.version = 1;
    cache_index.entries.push_back(sample_entry());

    const auto bytes = hyper::cache::encode_cache(cache_index);
    ASSERT_FALSE(bytes.empty());
    ASSERT_GE(bytes.size(), 44U);

    hyper::BinaryReader reader(bytes);
    const auto header = hyper::cache::decode_header(reader);

    ASSERT_TRUE(header.has_value());
    EXPECT_EQ(header->signature, hyper::FormatSignature);
    EXPECT_EQ(header->version, 1U);
    EXPECT_EQ(header->entry_count, 1U);

    const auto entry = hyper::cache::decode_entry(reader);
    ASSERT_TRUE(entry.has_value());
    EXPECT_EQ(entry->path, path_from_utf8("src/你好.txt"));

    std::vector<std::byte> checksum_input;
    checksum_input.insert(checksum_input.end(), bytes.begin(), bytes.begin() + 12);
    checksum_input.insert(checksum_input.end(), bytes.begin() + 44, bytes.end());

    const auto expected_checksum = hyper::sha256::calculate_sha256(checksum_input);
    ASSERT_TRUE(expected_checksum.has_value());
    EXPECT_EQ(header->index_checksum, expected_checksum.value());
}

TEST(CacheCodecTest, ReadsCacheWrittenByWriter) {
    hyper::CacheIndex expected;
    expected.header.signature = hyper::FormatSignature;
    expected.header.version = 1;
    expected.entries.push_back(sample_entry());
    expected.entries.push_back(sample_entry("README.md"));

    const auto bytes = hyper::cache::encode_cache(expected);
    const auto actual = hyper::cache::decode_cache(bytes);

    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(actual->header.signature, hyper::FormatSignature);
    EXPECT_EQ(actual->header.version, 1U);
    EXPECT_EQ(actual->header.entry_count, 2U);
    ASSERT_EQ(actual->entries.size(), 2U);
    EXPECT_EQ(actual->entries[0].path, path_from_utf8("src/你好.txt"));
    EXPECT_EQ(actual->entries[1].path, path_from_utf8("README.md"));
}

TEST(CacheCodecTest, RejectsCacheWithModifiedEntryBytes) {
    hyper::CacheIndex cache_index;
    cache_index.header.signature = hyper::FormatSignature;
    cache_index.header.version = 1;
    cache_index.entries.push_back(sample_entry());

    auto bytes = hyper::cache::encode_cache(cache_index);
    ASSERT_GT(bytes.size(), 44U);
    bytes.back() ^= std::byte{0x01};

    EXPECT_FALSE(hyper::cache::decode_cache(bytes).has_value());
}

TEST(CacheCodecTest, RejectsCacheWithTrailingBytes) {
    hyper::CacheIndex cache_index;
    cache_index.header.signature = hyper::FormatSignature;
    cache_index.header.version = 1;
    cache_index.entries.push_back(sample_entry());

    auto bytes = hyper::cache::encode_cache(cache_index);
    bytes.push_back(std::byte{0xFF});

    EXPECT_FALSE(hyper::cache::decode_cache(bytes).has_value());
}
