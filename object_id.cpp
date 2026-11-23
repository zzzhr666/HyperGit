#include "object_id.hpp"

#include <algorithm>

#include "sha256.hpp"

std::optional<hyper::ObjectId> hyper::ObjectId::from_hex(const std::string_view hex_summary) {
    const auto bytes = sha256::hex_to_bytes(hex_summary);
    if (!bytes.has_value()) {
        return std::nullopt;
    }
    return ObjectId{bytes.value()};
}

hyper::ObjectId hyper::ObjectId::from_digest(const std::span<std::byte, 32> digest) {
    std::array<std::byte, 32> bytes{};
    std::ranges::copy(digest, bytes.begin());
    return ObjectId{bytes};
}

std::string hyper::ObjectId::to_hex() const {
    return sha256::bytes_to_hex(summary_);
}

hyper::ObjectId::ObjectId(const std::array<std::byte, 32>& bytes) : summary_(bytes) {}
