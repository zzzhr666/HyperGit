#pragma once
#include "cache.hpp"

#include <optional>
#include <vector>
#include <span>

namespace hyper {
    class BinaryReader;
    class BinaryWriter;
}


namespace hyper::cache {
    void encode_header(BinaryWriter& writer, const CacheHeader& header);

    [[nodiscard]] std::optional<CacheHeader> decode_header(BinaryReader& reader);

        bool encode_entry(BinaryWriter& writer, const CacheEntry& entry);

    [[nodiscard]] std::optional<CacheEntry> decode_entry(BinaryReader& reader);

    [[nodiscard]] std::vector<std::byte> encode_cache(const CacheIndex& cache_index);

    [[nodiscard]] std::optional<CacheIndex> decode_cache(std::span<const std::byte> data);
}
