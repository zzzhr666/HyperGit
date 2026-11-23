#pragma once
#include <array>
#include <vector>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>

#include "object_id.hpp"

namespace hyper {
    constexpr std::array<std::byte,5> BlobHead {
        {std::byte{'b'},std::byte{'l'},std::byte{'o'},std::byte{'b'},std::byte{' '}},
    };
    std::vector<std::byte>encode_blob(std::span<const std::byte> content);

    std::optional<std::vector<std::byte>> decode_blob(std::span<const std::byte> content);

    std::optional<ObjectId> calculate_blob_id(std::span<const std::byte> content);

    std::optional<ObjectId> write_blob(const std::filesystem::path& objects_root, std::span<const std::byte> content);

    std::optional<std::vector<std::byte>> read_blob(const std::filesystem::path& objects_root, const ObjectId& object_id);

}
