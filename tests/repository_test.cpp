#include "repository.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto unique_suffix =
            std::chrono::steady_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() /
            ("hypergit-repository-test-" + std::to_string(unique_suffix));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    [[nodiscard]] const std::filesystem::path& path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

} // namespace

TEST(RepositoryTest, CreatesRepositoryAndObjectsDirectories) {
    TemporaryDirectory temporary_directory;

    ASSERT_TRUE(hyper::init_repository(temporary_directory.path()));

    EXPECT_TRUE(std::filesystem::is_directory(
        temporary_directory.path() / ".hypergit"
    ));
    EXPECT_TRUE(std::filesystem::is_directory(
        temporary_directory.path() / ".hypergit" / "objects"
    ));
}

TEST(RepositoryTest, CreatesMissingWorkTree) {
    TemporaryDirectory temporary_directory;
    const auto work_tree = temporary_directory.path() / "project";

    ASSERT_TRUE(hyper::init_repository(work_tree));

    EXPECT_TRUE(std::filesystem::is_directory(work_tree));
    EXPECT_TRUE(std::filesystem::is_directory(work_tree / ".hypergit" / "objects"));
}

TEST(RepositoryTest, IsIdempotent) {
    TemporaryDirectory temporary_directory;

    ASSERT_TRUE(hyper::init_repository(temporary_directory.path()));
    ASSERT_TRUE(hyper::init_repository(temporary_directory.path()));

    EXPECT_TRUE(std::filesystem::is_directory(
        temporary_directory.path() / ".hypergit" / "objects"
    ));
}

TEST(RepositoryTest, RejectsWorkTreeThatIsAFile) {
    TemporaryDirectory temporary_directory;
    const auto work_tree = temporary_directory.path() / "work-tree-file";

    std::ofstream file(work_tree);
    ASSERT_TRUE(file.is_open());
    file << "not a directory";
    file.close();

    EXPECT_FALSE(hyper::init_repository(work_tree));
    EXPECT_FALSE(std::filesystem::exists(work_tree / ".hypergit"));
}

TEST(RepositoryTest, RejectsRepositoryDirectoryThatIsAFile) {
    TemporaryDirectory temporary_directory;
    const auto repository_directory = temporary_directory.path() / ".hypergit";

    std::ofstream file(repository_directory);
    ASSERT_TRUE(file.is_open());
    file << "not a directory";
    file.close();

    EXPECT_FALSE(hyper::init_repository(temporary_directory.path()));
    EXPECT_FALSE(std::filesystem::exists(repository_directory / "objects"));
}

TEST(RepositoryTest, RejectsObjectsDirectoryThatIsAFile) {
    TemporaryDirectory temporary_directory;
    const auto objects_directory =
        temporary_directory.path() / ".hypergit" / "objects";

    std::filesystem::create_directories(objects_directory.parent_path());
    std::ofstream file(objects_directory);
    ASSERT_TRUE(file.is_open());
    file << "not a directory";
    file.close();

    EXPECT_FALSE(hyper::init_repository(temporary_directory.path()));
}
