#include "object_store.hpp"

#include <fstream>
#include <limits>

#include "object_id.hpp"
#include "sha256.hpp"

std::filesystem::path hyper::object_store_path(const fs::path& objects_root, const ObjectId& object_id) {
    const auto hex_bytes = object_id.to_hex();
    return objects_root / hex_bytes.substr(0, 2) / hex_bytes.substr(2);
}

std::optional<hyper::ObjectId> hyper::write_object(const fs::path& objects_root, const std::span<const std::byte> encoded_object) {
    auto sha256 = sha256::calculate_sha256(encoded_object);
    if (!sha256.has_value()) {
        return std::nullopt;
    }
    auto obj_id = ObjectId::from_digest(sha256.value());
    auto file_path = object_store_path(objects_root, obj_id);
    std::error_code ec;
    if (fs::create_directories(file_path.parent_path(), ec); ec) {
        return std::nullopt;
    }
    std::ofstream object_store_file(file_path, std::ios::binary);
    if (!object_store_file.is_open()) {
        return std::nullopt;
    }
    object_store_file.write(reinterpret_cast<const char*>(encoded_object.data()), static_cast<std::streamsize>(encoded_object.size()));
    if (!object_store_file) {
        return std::nullopt;
    }
    return obj_id;
}

std::optional<std::vector<std::byte>> hyper::read_object(const fs::path& objects_root, const ObjectId& object_id) {
    const auto file_path = object_store_path(objects_root, object_id);
    std::error_code ec;
    if (!fs::exists(file_path, ec) || ec) {
        return std::nullopt;
    }
    if (!fs::is_regular_file(file_path, ec) || ec) {
        return std::nullopt;
    }
    std::ifstream object_store_file(file_path, std::ios::binary);
    if (!object_store_file.is_open()) {
        return std::nullopt;
    }
    const auto file_size = fs::file_size(file_path, ec);
    if (ec || file_size > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
        return std::nullopt;
    }

    std::vector<std::byte> encoded_object(file_size);
    if (file_size != 0) {
        object_store_file.read(
            reinterpret_cast<char*>(encoded_object.data()),
            static_cast<std::streamsize>(file_size)
        );
    }
    if (object_store_file.bad() ||
        (file_size != 0 && object_store_file.gcount() != static_cast<std::streamsize>(file_size))) {
        return std::nullopt;
    }
    auto sha256 = sha256::calculate_sha256(encoded_object);
    if (!sha256.has_value()) {
        return std::nullopt;
    }
    if (ObjectId::from_digest(sha256.value()) != object_id) {
        return std::nullopt;
    }
    return encoded_object;
}
