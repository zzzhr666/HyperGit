#include "binary_reader.hpp"

#include <bit>

std::optional<std::uint32_t> hyper::BinaryReader::read_uint32() {
    if (position_ > buffer_.size() || sizeof(std::uint32_t) > buffer_.size() - position_) {
        return std::nullopt;
    }

    std::uint32_t result = 0;
    for (int i = 0; i < 4; ++i) {
        result |= static_cast<std::uint32_t>(buffer_[position_ + i]) << ((3 - i) * 8);
    }
    position_ += sizeof(std::uint32_t);
    return result;
}

std::optional<std::uint64_t> hyper::BinaryReader::read_uint64() {
    if (position_ > buffer_.size() || sizeof(std::uint64_t) > buffer_.size() - position_) {
        return std::nullopt;
    }

    std::uint64_t result = 0;
    for (int i = 0; i < 8; ++i) {
        result |= static_cast<std::uint64_t>(buffer_[position_ + i]) << ((7 - i) * 8);
    }
    position_ += sizeof(std::uint64_t);
    return result;
}

std::optional<std::int64_t> hyper::BinaryReader::read_int64() {
    const auto result = read_uint64();
    if (result.has_value() == false) {
        return std::nullopt;
    }
    return std::bit_cast<std::int64_t>(result.value());
}

std::optional<std::string> hyper::BinaryReader::read_string() {
    const auto start = position_;
    const auto size = read_uint32();
    if (size.has_value() == false) {
        return std::nullopt;
    }

    const auto bytes = read_bytes(size.value());
    if (bytes.has_value() == false) {
        position_ = start;
        return std::nullopt;
    }

    std::string result;
    result.reserve(bytes.value().size());
    for (const auto byte : bytes.value()) {
        result.push_back(static_cast<char>(std::to_integer<unsigned char>(byte)));
    }
    return result;
}

std::optional<std::vector<std::byte>> hyper::BinaryReader::read_bytes(const std::size_t size) {
    if (position_ > buffer_.size() || size > buffer_.size() - position_) {
        return std::nullopt;
    }

    std::vector<std::byte> result(
        buffer_.begin() + static_cast<std::ptrdiff_t>(position_),
        buffer_.begin() + static_cast<std::ptrdiff_t>(position_ + size)
    );
    position_ += size;
    return result;
}
