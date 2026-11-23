#include "blob.hpp"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "object_store.hpp"

namespace {

class TemporaryObjectStore {
public:
    TemporaryObjectStore() {
        const auto unique_suffix =
            std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
            ("hypergit-blob-test-" + std::to_string(unique_suffix));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryObjectStore() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    const std::filesystem::path& path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

std::vector<std::byte> bytes_of(const std::string_view value) {
    const auto* begin = reinterpret_cast<const std::byte*>(value.data());
    return {begin, begin + value.size()};
}

std::vector<std::byte> expected_blob(
    const std::string_view header,
    const std::vector<std::byte>& content
) {
    auto result = bytes_of(header);
    result.insert(result.end(), std::byte{'\0'});
    result.insert(result.end(), content.begin(), content.end());
    return result;
}

} // namespace

TEST(BlobTest, EncodesEmptyBlob) {
    const std::vector<std::byte> content;

    const auto actual = hyper::encode_blob(content);
    const auto expected = expected_blob("blob 0", content);

    EXPECT_EQ(actual, expected);
}

TEST(BlobTest, EncodesTextBlob) {
    const auto content = bytes_of("hello");

    const auto actual = hyper::encode_blob(content);
    const auto expected = expected_blob("blob 5", content);

    EXPECT_EQ(actual, expected);
}

TEST(BlobTest, EncodesBinaryContentIncludingNullBytes) {
    const std::array content{
        std::byte{0x00},
        std::byte{0x01},
        std::byte{0x00},
        std::byte{0xFF}
    };

    const auto actual = hyper::encode_blob(content);
    const std::vector expected{
        std::byte{'b'}, std::byte{'l'}, std::byte{'o'}, std::byte{'b'},
        std::byte{' '}, std::byte{'4'}, std::byte{'\0'},
        std::byte{0x00}, std::byte{0x01}, std::byte{0x00}, std::byte{0xFF}
    };

    EXPECT_EQ(actual, expected);
}

TEST(BlobTest, CountsUtf8ContentByBytes) {
    const auto content = bytes_of("你好");

    const auto actual = hyper::encode_blob(content);
    const auto expected = expected_blob("blob 6", content);

    EXPECT_EQ(actual, expected);
}

TEST(BlobTest, EncodesTwoDigitContentLength) {
    const auto content = bytes_of("0123456789");

    const auto actual = hyper::encode_blob(content);
    const auto expected = expected_blob("blob 10", content);

    EXPECT_EQ(actual, expected);
}

TEST(BlobTest, CalculatesObjectIdForEmptyBlob) {
    const std::vector<std::byte> content;

    const auto object_id = hyper::calculate_blob_id(content);

    ASSERT_TRUE(object_id.has_value());
    EXPECT_EQ(
        object_id->to_hex(),
        "473a0f4c3be8a93681a267e3b1e9a7dc"
        "da1185436fe141f7749120a303721813"
    );
}

TEST(BlobTest, CalculatesObjectIdForHello) {
    const auto content = bytes_of("hello");

    const auto object_id = hyper::calculate_blob_id(content);

    ASSERT_TRUE(object_id.has_value());
    EXPECT_EQ(
        object_id->to_hex(),
        "8aec4e4876f854f688d0ebfc8f37598f"
        "38e5fd6903cccc850ca36591175aeb60"
    );
}

TEST(BlobTest, CalculatesObjectIdForBinaryContent) {
    const std::array content{
        std::byte{0x00},
        std::byte{0x01},
        std::byte{0x00},
        std::byte{0xFF}
    };

    const auto object_id = hyper::calculate_blob_id(content);

    ASSERT_TRUE(object_id.has_value());
    EXPECT_EQ(
        object_id->to_hex(),
        "59741cf79a42c7acb7b4a7800e11401f"
        "6d6bf05b5f7cfc888e50f37fb7e25c1a"
    );
}

TEST(BlobTest, SameContentProducesSameObjectId) {
    const auto first = bytes_of("same content");
    const auto second = bytes_of("same content");

    const auto first_id = hyper::calculate_blob_id(first);
    const auto second_id = hyper::calculate_blob_id(second);

    ASSERT_TRUE(first_id.has_value());
    ASSERT_TRUE(second_id.has_value());
    EXPECT_EQ(first_id.value(), second_id.value());
}

TEST(BlobTest, DifferentContentProducesDifferentObjectId) {
    const auto first = bytes_of("content a");
    const auto second = bytes_of("content b");

    const auto first_id = hyper::calculate_blob_id(first);
    const auto second_id = hyper::calculate_blob_id(second);

    ASSERT_TRUE(first_id.has_value());
    ASSERT_TRUE(second_id.has_value());
    EXPECT_NE(first_id.value(), second_id.value());
}

TEST(BlobTest, WritesEncodedBlobToObjectStore) {
    TemporaryObjectStore store;
    const auto content = bytes_of("hello");

    const auto written_id = hyper::write_blob(store.path(), content);
    const auto expected_id = hyper::calculate_blob_id(content);

    ASSERT_TRUE(written_id.has_value());
    ASSERT_TRUE(expected_id.has_value());
    EXPECT_EQ(*written_id, *expected_id);

    const auto stored_object = hyper::read_object(store.path(), *written_id);

    ASSERT_TRUE(stored_object.has_value());
    EXPECT_EQ(*stored_object, hyper::encode_blob(content));
}

TEST(BlobTest, WritesBinaryBlobToObjectStore) {
    TemporaryObjectStore store;
    const std::array content{
        std::byte{0x00},
        std::byte{0x01},
        std::byte{0x00},
        std::byte{0xFF}
    };

    const auto written_id = hyper::write_blob(store.path(), content);

    ASSERT_TRUE(written_id.has_value());
    const auto stored_object = hyper::read_object(store.path(), *written_id);

    ASSERT_TRUE(stored_object.has_value());
    EXPECT_EQ(*stored_object, hyper::encode_blob(content));
}

TEST(BlobTest, ReadsTextBlobFromObjectStore) {
    TemporaryObjectStore store;
    const auto content = bytes_of("hello");

    const auto object_id = hyper::write_blob(store.path(), content);
    ASSERT_TRUE(object_id.has_value());

    const auto actual = hyper::read_blob(store.path(), *object_id);

    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(*actual, content);
}

TEST(BlobTest, ReadsEmptyBlobFromObjectStore) {
    TemporaryObjectStore store;
    const std::vector<std::byte> content;

    const auto object_id = hyper::write_blob(store.path(), content);
    ASSERT_TRUE(object_id.has_value());

    const auto actual = hyper::read_blob(store.path(), *object_id);

    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(*actual, content);
}

TEST(BlobTest, ReadsBinaryBlobFromObjectStore) {
    TemporaryObjectStore store;
    const std::array content{
        std::byte{0x00},
        std::byte{0x01},
        std::byte{0x00},
        std::byte{0xFF}
    };

    const auto object_id = hyper::write_blob(store.path(), content);
    ASSERT_TRUE(object_id.has_value());

    const auto actual = hyper::read_blob(store.path(), *object_id);

    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(*actual, std::vector<std::byte>(content.begin(), content.end()));
}

TEST(BlobTest, ReadingMissingBlobFails) {
    TemporaryObjectStore store;
    const auto object_id = hyper::calculate_blob_id(bytes_of("missing"));

    ASSERT_TRUE(object_id.has_value());

    const auto actual = hyper::read_blob(store.path(), *object_id);

    EXPECT_FALSE(actual.has_value());
}

TEST(BlobTest, RejectsObjectWithInvalidBlobEncoding) {
    TemporaryObjectStore store;
    const auto invalid_blob = bytes_of("this is not a blob");

    const auto object_id = hyper::write_object(store.path(), invalid_blob);
    ASSERT_TRUE(object_id.has_value());

    const auto actual = hyper::read_blob(store.path(), *object_id);

    EXPECT_FALSE(actual.has_value());
}
