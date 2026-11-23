#include "cache_file.hpp"

#include <gtest/gtest.h>

#include "cache.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

hyper::fs::path path_from_utf8(const std::string& value) {
    return hyper::fs::path(
        std::u8string(
            reinterpret_cast<const char8_t*>(value.data()),
            value.size()
        )
    );
}

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto stamp =
            std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path()
            / ("hypergit-cache-test-" + std::to_string(stamp));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    const std::filesystem::path& path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

hyper::CacheEntry sample_entry(std::string name) {
    hyper::CacheEntry entry{};
    entry.modified_time_ns = 2;
    entry.file_size = 3;
    entry.path = path_from_utf8(name);

    for (std::size_t i = 0; i < entry.obj_id.size(); ++i) {
        entry.obj_id[i] = static_cast<std::byte>(i);
    }

    return entry;
}

hyper::CacheIndex sample_index() {
    hyper::CacheIndex index;
    index.header.signature = hyper::FormatSignature;
    index.header.version = hyper::CurVersion;
    index.entries.push_back(sample_entry("src/你好.txt"));
    index.entries.push_back(sample_entry("README.md"));
    return index;
}

} // namespace

TEST(CacheFileTest, WritesAndReadsCacheFile) {
    TemporaryDirectory directory;
    const auto file = directory.path() / "cache file.bin";
    const auto expected = sample_index();

    ASSERT_TRUE(hyper::cache::write_cache_file(file, expected));

    const auto actual = hyper::cache::read_cache_file(file);

    ASSERT_TRUE(actual.has_value());
    EXPECT_EQ(actual->header.signature, hyper::FormatSignature);
    EXPECT_EQ(actual->header.version, hyper::CurVersion);
    EXPECT_EQ(actual->header.entry_count, 2U);
    ASSERT_EQ(actual->entries.size(), 2U);
    EXPECT_EQ(actual->entries[0].path, path_from_utf8("src/你好.txt"));
    EXPECT_EQ(actual->entries[1].path, path_from_utf8("README.md"));
}

TEST(CacheFileTest, FailsWhenFileDoesNotExist) {
    TemporaryDirectory directory;
    const auto file = directory.path() / "missing.bin";

    EXPECT_FALSE(hyper::cache::read_cache_file(file).has_value());
}

TEST(CacheFileTest, FailsWhenFileIsEmpty) {
    TemporaryDirectory directory;
    const auto file = directory.path() / "empty.bin";

    std::ofstream output(file, std::ios::binary);
    ASSERT_TRUE(output.is_open());
    output.close();

    EXPECT_FALSE(hyper::cache::read_cache_file(file).has_value());
}

TEST(CacheFileTest, FailsWhenFileIsTruncated) {
    TemporaryDirectory directory;
    const auto file = directory.path() / "truncated.bin";

    ASSERT_TRUE(hyper::cache::write_cache_file(file, sample_index()));

    std::error_code error;
    auto size = std::filesystem::file_size(file, error);
    ASSERT_FALSE(error);
    ASSERT_GT(size, 0U);
    std::filesystem::resize_file(file, size - 1, error);
    ASSERT_FALSE(error);

    EXPECT_FALSE(hyper::cache::read_cache_file(file).has_value());
}

TEST(CacheFileTest, FailsWhenFileContentsAreModified) {
    TemporaryDirectory directory;
    const auto file = directory.path() / "modified.bin";

    ASSERT_TRUE(hyper::cache::write_cache_file(file, sample_index()));

    std::fstream stream(file, std::ios::binary | std::ios::in | std::ios::out);
    ASSERT_TRUE(stream.is_open());
    stream.seekp(0, std::ios::end);
    const auto end = stream.tellp();
    ASSERT_GT(end, std::streampos{0});
    stream.seekg(end - std::streamoff{1});

    char byte{};
    stream.get(byte);
    stream.seekp(end - std::streamoff{1});
    byte ^= static_cast<char>(0x01);
    stream.put(byte);
    stream.flush();
    stream.close();

    EXPECT_FALSE(hyper::cache::read_cache_file(file).has_value());
}
