#include "blob.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <string>
#include <vector>

#include "object_store.hpp"
#include "sha256.hpp"


std::vector<std::byte> hyper::encode_blob(std::span<const std::byte> content) {
    const auto content_size = content.size();
    const auto content_size_str = std::to_string(content_size);
    const std::size_t total_size = BlobHead.size() + content_size_str.size() + 1 + content_size;
    std::vector<std::byte> blob(total_size);
    auto it = std::ranges::copy(BlobHead, blob.begin()).out;
    it = std::ranges::transform(content_size_str, it, [](char c) {
        return static_cast<std::byte>(c);
    }).out;
    *it++ = std::byte{'\0'};
    std::ranges::copy(content, it);
    return blob;
}

std::optional<std::vector<std::byte>> hyper::decode_blob(std::span<const std::byte> content) {
    const auto null_it = std::ranges::find(content, std::byte{'\0'});
    if (null_it == content.end()) {
        return std::nullopt;
    }
    const auto header_len = static_cast<std::size_t>(null_it - content.begin());
    if (header_len <= BlobHead.size()) {
        return std::nullopt;
    }
    auto header = std::span(content.data(), header_len);
    for (std::size_t i = 0; i < BlobHead.size(); i++) {
        if (header[i] != BlobHead[i]) {
            return std::nullopt;
        }
    }
    header = std::span(header.begin() + BlobHead.size(), header.end());
    std::size_t bytes_size{};
    if (auto [ptr,ec] = std::from_chars(reinterpret_cast<const char*>(header.data()), reinterpret_cast<const char*>(header.data() + header.size()), bytes_size); ec != std::errc{}
        || ptr != reinterpret_cast<const char*>(header.data() + header.size())) {
        return std::nullopt;
    }
    auto actual_content = content.subspan(header_len + 1);
    if (actual_content.size() != bytes_size) {
        return std::nullopt;
    }
    return std::vector(actual_content.begin(), actual_content.end());
}

std::optional<hyper::ObjectId> hyper::calculate_blob_id(std::span<const std::byte> content) {
    auto blob_content = encode_blob(content);
    auto blob_sha256 = sha256::calculate_sha256(blob_content);
    if (!blob_sha256.has_value()) {
        return std::nullopt;
    }
    return ObjectId::from_digest(blob_sha256.value());
}

std::optional<hyper::ObjectId> hyper::write_blob(const std::filesystem::path& objects_root, const std::span<const std::byte> content) {
    return write_object(objects_root, encode_blob(content));
}

std::optional<std::vector<std::byte>> hyper::read_blob(const std::filesystem::path& objects_root, const ObjectId& object_id) {
    auto blob_content = read_object(objects_root, object_id);
    if (!blob_content.has_value()) {
        return std::nullopt;
    }
    const auto blob_content_view = std::span(blob_content.value());
    return decode_blob(blob_content_view);
}
