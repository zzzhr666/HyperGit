#pragma once
#include <filesystem>

namespace hyper {
    bool init_repository(const std::filesystem::path& work_tree);
}
