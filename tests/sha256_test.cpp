#include "sha256.hpp"

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::vector<std::byte> bytes_of(const std::string_view value) {
    const auto* begin = reinterpret_cast<const std::byte*>(value.data());
    return {begin, begin + value.size()};
}

} // namespace

TEST(Sha256Test, CalculatesSha256ForEmptyInput) {
    const std::span<const std::byte> input{};

    const auto result = hyper::sha256::calculate_sha256(input);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(
        hyper::sha256::bytes_to_hex(*result),
        "e3b0c44298fc1c149afbf4c8996fb924"
        "27ae41e4649b934ca495991b7852b855"
    );
}

TEST(Sha256Test, CalculatesSha256ForAsciiInput) {
    const auto input = bytes_of("abc");

    const auto result = hyper::sha256::calculate_sha256(input);

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(
        hyper::sha256::bytes_to_hex(*result),
        "ba7816bf8f01cfea414140de5dae2223"
        "b00361a396177a9cb410ff61f20015ad"
    );
}

TEST(Sha256Test, ConvertsValidHexStringToBytes) {
    const auto result = hyper::sha256::hex_to_bytes(
        "000102030405060708090a0b0c0d0e0f"
        "101112131415161718191a1b1c1d1e1f"
    );

    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(hyper::sha256::bytes_to_hex(result.value()),
        "000102030405060708090a0b0c0d0e0f"
        "101112131415161718191a1b1c1d1e1f");
}

TEST(Sha256Test, RejectsHexStringWithInvalidLength) {
    const auto result = hyper::sha256::hex_to_bytes("00");

    EXPECT_FALSE(result.has_value());
}

TEST(Sha256Test, RejectsHexStringWithInvalidCharacter) {
    const auto result = hyper::sha256::hex_to_bytes(
        "000102030405060708090a0b0c0d0e0f"
        "101112131415161718191a1b1c1d1eXf"
    );

    EXPECT_FALSE(result.has_value());
}
