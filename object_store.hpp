#pragma once
#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

#include "object_id.hpp"

namespace hyper {


    namespace fs = std::filesystem;

    fs::path object_store_path(const fs::path& objects_root, const ObjectId& object_id);

    std::optional<ObjectId> write_object(const fs::path& objects_root, std::span<const std::byte> encoded_object);

    std::optional<std::vector<std::byte>> read_object(const fs::path& objects_root, const ObjectId& object_id);
}
