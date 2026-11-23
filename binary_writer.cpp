#include "binary_writer.hpp"

#include <bit>
#include <cstddef>

void hyper::BinaryWriter::write_uint32(const std::uint32_t value) {
    for (int i = 3; i >= 0; --i) {
        buffer_.push_back(static_cast<std::byte>((value >> (i * 8)) & 0xFF));
    }
}

void hyper::BinaryWriter::write_uint64(const std::uint64_t value) {
    write_uint32(static_cast<std::uint32_t>(value >> 32));
    write_uint32(static_cast<std::uint32_t>(value & 0xFFFFFFFF));
}

void hyper::BinaryWriter::write_int64(const std::int64_t value) {
    write_uint64(std::bit_cast<std::uint64_t>(value));
}

void hyper::BinaryWriter::write_bytes(std::span<const std::byte> bytes) {
    buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
}

void hyper::BinaryWriter::write_string(const std::string_view value) {
    write_uint32(value.size());
    buffer_.insert(buffer_.end(), reinterpret_cast<const std::byte*>(value.data()), reinterpret_cast<const std::byte*>(value.data() + value.size()));
}
