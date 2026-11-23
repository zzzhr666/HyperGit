#include "object_store.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "object_id.hpp"

namespace {

class TemporaryObjectStore {
public:
    TemporaryObjectStore() {
        const auto unique_suffix =
            std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
            ("hypergit-object-store-" + std::to_string(unique_suffix));
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

hyper::ObjectId test_object_id() {
    constexpr std::string_view hex =
        "473a0f4c3be8a93681a267e3b1e9a7dc"
        "da1185436fe141f7749120a303721813";

    const auto object_id = hyper::ObjectId::from_hex(hex);
    EXPECT_TRUE(object_id.has_value());
    return *object_id;
}

std::vector<std::byte> test_object() {
    return {
        std::byte{'b'}, std::byte{'l'}, std::byte{'o'}, std::byte{'b'},
        std::byte{' '}, std::byte{'5'}, std::byte{'\0'},
        std::byte{'h'}, std::byte{'e'}, std::byte{'l'}, std::byte{'l'}, std::byte{'o'}
    };
}

} // namespace

TEST(ObjectStoreTest, SplitsObjectIdIntoDirectoryAndFileName) {
    const auto object_id = test_object_id();
    const std::filesystem::path objects_root = ".hypergit/objects";

    const auto actual = hyper::object_store_path(objects_root, object_id);
    const auto expected =
        objects_root
        / "47"
        / "3a0f4c3be8a93681a267e3b1e9a7dcda1185436fe141f7749120a303721813";

    EXPECT_EQ(actual, expected);
}

TEST(ObjectStoreTest, WorksWhenObjectsRootDoesNotExist) {
    const auto object_id = test_object_id();
    const std::filesystem::path objects_root =
        "directory-that-does-not-exist/.hypergit/objects";

    const auto actual = hyper::object_store_path(objects_root, object_id);

    EXPECT_EQ(
        actual,
        objects_root
        / "47"
        / "3a0f4c3be8a93681a267e3b1e9a7dcda1185436fe141f7749120a303721813"
    );
}

TEST(ObjectStoreTest, NormalizesRootWithTrailingSeparator) {
    const auto object_id = test_object_id();
    const std::filesystem::path root_without_separator = ".hypergit/objects";
    const std::filesystem::path root_with_separator = ".hypergit/objects/";

    EXPECT_EQ(
        hyper::object_store_path(root_without_separator, object_id),
        hyper::object_store_path(root_with_separator, object_id)
    );
}

TEST(ObjectStoreTest, DifferentObjectIdsProduceDifferentPaths) {
    const auto first = test_object_id();
    const auto second = hyper::ObjectId::from_hex(
        "8aec4e4876f854f688d0ebfc8f37598f"
        "38e5fd6903cccc850ca36591175aeb60"
    );

    ASSERT_TRUE(second.has_value());

    EXPECT_NE(
        hyper::object_store_path(".hypergit/objects", first),
        hyper::object_store_path(".hypergit/objects", *second)
    );
}

TEST(ObjectStoreTest, WritesAndReadsObject) {
    TemporaryObjectStore store;
    const auto object = test_object();

    const auto object_id = hyper::write_object(store.path(), object);

    ASSERT_TRUE(object_id.has_value());
    const auto object_path = hyper::object_store_path(store.path(), *object_id);
    EXPECT_TRUE(std::filesystem::is_regular_file(object_path));

    const auto actual = hyper::read_object(store.path(), *object_id);

    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(*actual, object);
}

TEST(ObjectStoreTest, ReadingMissingObjectFails) {
    TemporaryObjectStore store;

    const auto actual = hyper::read_object(store.path(), test_object_id());

    EXPECT_FALSE(actual.has_value());
}

TEST(ObjectStoreTest, ReadingTamperedObjectFailsChecksumValidation) {
    TemporaryObjectStore store;
    const auto object = test_object();
    const auto object_id = hyper::write_object(store.path(), object);

    ASSERT_TRUE(object_id.has_value());

    const auto object_path = hyper::object_store_path(store.path(), *object_id);
    std::fstream file(object_path, std::ios::binary | std::ios::in | std::ios::out);
    ASSERT_TRUE(file.is_open());
    file.seekp(0);
    const char tampered_byte = 'x';
    file.write(&tampered_byte, 1);
    ASSERT_TRUE(file);
    file.close();

    const auto actual = hyper::read_object(store.path(), *object_id);

    EXPECT_FALSE(actual.has_value());
}
