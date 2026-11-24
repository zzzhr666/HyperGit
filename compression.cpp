#include "compression.hpp"

#include <zstd.h>

std::optional<std::vector<std::byte>> hyper::compression::compress(const std::span<const std::byte> data) {
    const auto bound = ZSTD_compressBound(data.size());
    std::vector<std::byte> buffer(bound);
    const auto result = ZSTD_compress(buffer.data(), buffer.size(), data.data(), data.size(),ZSTD_CLEVEL_DEFAULT);
    if (ZSTD_isError(result)) {
        return std::nullopt;
    }
    buffer.resize(result);
    return buffer;
}

std::optional<std::vector<std::byte>> hyper::compression::decompress(const std::span<const std::byte> data) {
    const auto content_size = ZSTD_getFrameContentSize(data.data(), data.size());
    if (content_size == ZSTD_CONTENTSIZE_ERROR || content_size == ZSTD_CONTENTSIZE_UNKNOWN) {
        return std::nullopt;
    }
    std::vector<std::byte> buffer(content_size);
    if (const auto result = ZSTD_decompress(buffer.data(), buffer.size(), data.data(), data.size()); ZSTD_isError(result) || result != content_size) {
        return std::nullopt;
    }
    return buffer;
}
