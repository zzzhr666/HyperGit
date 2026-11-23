#include "sha256.hpp"

#include <cstdint>
#include <openssl/evp.h>

namespace {
    constexpr char hex[] = "0123456789abcdef";

    int hex_digit_to_value(const unsigned char c) {
        if (c >= '0' && c <= '9') {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        return -1;
    }
}

std::optional<std::array<std::byte, 32>> hyper::sha256::calculate_sha256(const std::span<const std::byte> data) {
    std::array<std::byte, 32> result{};
    unsigned int len{};

    if (EVP_Digest(data.data(), data.size(),
                   reinterpret_cast<unsigned char*>(result.data()),
                   &len, EVP_sha256(), nullptr) != 1) {
        return std::nullopt;
    }

    if (len != result.size()) {
        return std::nullopt;
    }

    return result;
}

std::string hyper::sha256::bytes_to_hex(const std::array<std::byte, 32>& bytes) {
    std::string result{};
    result.reserve(64);
    for (const auto byte : bytes) {
        const auto value = std::to_integer<std::uint8_t>(byte);
        result.push_back(hex[value >> 4]);
        result.push_back(hex[value & 0xf]);
    }
    return result;
}

std::optional<std::array<std::byte, 32>> hyper::sha256::hex_to_bytes(const std::string& data) {
    if (data.size() != 64) {
        return std::nullopt;
    }

    std::array<std::byte, 32> result{};

    for (std::size_t i = 0; i < result.size(); ++i) {
        const auto high = hex_digit_to_value(data[2 * i]);
        const auto low = hex_digit_to_value(data[2 * i + 1]);

        if (high < 0 || low < 0) {
            return std::nullopt;
        }

        result[i] = static_cast<std::byte>((high << 4) | low);
    }

    return result;
}
