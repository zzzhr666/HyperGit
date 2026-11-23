#pragma once
#include <cstdint>
#include <span>
#include <vector>
#include <cstddef>
#include <string_view>


namespace hyper {
    class BinaryWriter {
    public:
        BinaryWriter() = default;
        ~BinaryWriter() = default;

        void write_uint32(std::uint32_t value);

        void write_uint64(std::uint64_t value);

        void write_int64(std::int64_t value);

        void write_bytes(std::span<const std::byte> bytes);

        void write_string(std::string_view value);

        [[nodiscard]] const std::byte* data() const noexcept {
            return buffer_.data();
        }

        [[nodiscard]] std::span<const std::byte> bytes() const noexcept {
            return buffer_;
        }

        [[nodiscard]] std::size_t size() const noexcept {
            return buffer_.size();
        }

        void clear() noexcept {
            buffer_.clear();
        }


    private:
        std::vector<std::byte> buffer_;
    };
}
