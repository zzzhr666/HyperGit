#pragma once
#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>

namespace hyper::sha256 {
    std::optional<std::array<std::byte, 32>> calculate_sha256(std::span<const std::byte> data);
    std::string bytes_to_hex(const std::array<std::byte, 32>& bytes);
    std::optional<std::array<std::byte, 32>> hex_to_bytes(const std::string& data);
}
