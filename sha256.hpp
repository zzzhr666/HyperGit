#pragma once
#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>

namespace hyper::sha256 {
    std::optional<std::array<std::byte, 32>> calculate_sha256(std::span<const std::byte> data);

    std::string bytes_to_hex(std::span<const std::byte, 32> bytes);

    std::optional<std::array<std::byte, 32>> hex_to_bytes(std::string_view data);
}
