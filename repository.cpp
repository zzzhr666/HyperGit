#include "repository.hpp"

namespace {

const std::filesystem::path DefaultDir = ".hypergit";
const std::filesystem::path ObjectDirName = "objects";

}

bool hyper::init_repository(const std::filesystem::path& work_tree) {
    std::error_code err;
    const auto objects_dir = work_tree / DefaultDir / ObjectDirName;
    std::filesystem::create_directories(objects_dir, err);
    return !err;
}
