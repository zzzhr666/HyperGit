#pragma once
#include <filesystem>
#include <optional>




namespace hyper {
    class CacheIndex;

    namespace cache {
        namespace fs = std::filesystem;
        bool write_cache_file(const fs::path& file_path, const CacheIndex& index);

        std::optional<CacheIndex> read_cache_file(const fs::path& file_path);
    }
}
