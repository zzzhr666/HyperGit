#pragma once
#include <optional>
#include <span>
#include <vector>

namespace hyper::compression {
    std::optional<std::vector<std::byte>> compress(std::span<const std::byte> data);

    std::optional<std::vector<std::byte>> decompress(std::span<const std::byte> data);
}
