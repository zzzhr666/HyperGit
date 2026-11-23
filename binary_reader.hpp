#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace hyper {
    class BinaryReader {
    public:
        explicit BinaryReader(const std::span<const std::byte> buffer) : buffer_(buffer) {};

        [[nodiscard]] std::optional<std::uint32_t> read_uint32();

        [[nodiscard]] std::optional<std::uint64_t> read_uint64();

        [[nodiscard]] std::optional<std::int64_t> read_int64();

        [[nodiscard]] std::optional<std::string> read_string();

        [[nodiscard]] std::optional<std::vector<std::byte>> read_bytes(std::size_t size);

        [[nodiscard]] std::size_t position() const noexcept {
            return position_;
        }

    private:
        std::size_t position_{};
        std::span<const std::byte> buffer_{};
    };
}
