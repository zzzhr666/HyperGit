#pragma once
#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace hyper {
    class ObjectId {
    public:
        static std::optional<ObjectId> from_hex(std::string_view hex_summary);

        static ObjectId from_digest(std::span<std::byte,32> digest);

        [[nodiscard]] std::string to_hex() const;

        bool operator==(const ObjectId& other) const = default;
    private:
        explicit ObjectId(const std::array<std::byte,32>& bytes);
        std::array<std::byte,32> summary_{};
    };
}

