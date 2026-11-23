#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <vector>


namespace hyper {
    namespace fs = std::filesystem;

    constexpr std::array<std::byte, 4> FormatSignature = {{std::byte{'H'}, std::byte{'P'}, std::byte{'G'}, std::byte{'T'}}};
    constexpr std::uint32_t CurVersion = 1;

    struct CacheHeader {
        std::array<std::byte, 4> signature{};
        std::uint32_t version{};
        std::uint32_t entry_count{};
        std::array<std::byte, 32> index_checksum{};
    };

    struct CacheEntry {
        std::int64_t modified_time_ns{};
        std::uint64_t file_size{};
        std::array<std::byte, 32> obj_id{};
        fs::path path;
    };

    struct CacheIndex {
        CacheHeader header;
        std::vector<CacheEntry> entries;
    };


}
