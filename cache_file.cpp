#include "cache_file.hpp"

#include <fstream>
#include <limits>
#include <system_error>
#include <vector>


#include "cache_codec.hpp"

bool hyper::cache::write_cache_file(const fs::path& file_path, const CacheIndex& index) {
    const auto bytes = encode_cache(index);
    if (bytes.empty()) {
        return false;
    }

    std::ofstream out_file(file_path, std::ios::binary | std::ios::trunc);
    if (!out_file.is_open()) {
        return false;
    }
    out_file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!out_file) {
        return false;
    }

    out_file.flush();
    if (!out_file) {
        return false;
    }

    out_file.close();
    return static_cast<bool>(out_file);
}

std::optional<hyper::CacheIndex> hyper::cache::read_cache_file(const fs::path& file_path) {
    std::error_code error;

    if (!fs::exists(file_path, error) || error) {
        return std::nullopt;
    }

    if (!fs::is_regular_file(file_path, error) || error) {
        return std::nullopt;
    }

    const auto file_size = fs::file_size(file_path, error);
    if (error || file_size > std::numeric_limits<std::size_t>::max()) {
        return std::nullopt;
    }

    if (file_size > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
        return std::nullopt;
    }

    auto in_file = std::ifstream(file_path, std::ios::binary);
    if (!in_file.is_open()) {
        return std::nullopt;
    }

    std::vector<std::byte> bytes(file_size);
    in_file.read(
        reinterpret_cast<char*>(bytes.data()),
        static_cast<std::streamsize>(file_size)
    );
    if (!in_file ||
        in_file.gcount() != static_cast<std::streamsize>(file_size)) {
        return std::nullopt;
    }

    return decode_cache(bytes);
}
