#include "cache_codec.hpp"

#include "binary_reader.hpp"
#include "binary_writer.hpp"

#include <algorithm>
#include <limits>

#include "sha256.hpp"

namespace {
    std::optional<std::string> encode_relative_path(const hyper::fs::path& path) {
        if (path.empty() || path.has_root_name() || path.has_root_directory()) {
            return std::nullopt;
        }

        for (const auto& component : path) {
            if (component == "." || component == "..") {
                return std::nullopt;
            }
        }

        try {
            const auto utf8 = path.generic_u8string();
            return std::string(reinterpret_cast<const char*>(utf8.data()), utf8.size());
        } catch (const hyper::fs::filesystem_error&) {
            return std::nullopt;
        }
    }

    std::optional<hyper::fs::path> decode_relative_path(const std::string& value) {
        if (value.empty()) {
            return std::nullopt;
        }

        try {
            auto path = hyper::fs::path(std::u8string(reinterpret_cast<const char8_t*>(value.data()), value.size()));
            if (path.empty() || path.has_root_name() || path.has_root_directory()) {
                return std::nullopt;
            }

            for (const auto& component : path) {
                if (component == "." || component == "..") {
                    return std::nullopt;
                }
            }

            return path;
        } catch (const hyper::fs::filesystem_error&) {
            return std::nullopt;
        }
    }

    void encode_header_without_checksum(hyper::BinaryWriter& writer, const hyper::CacheHeader& header) {
        writer.write_bytes(header.signature);
        writer.write_uint32(header.version);
        writer.write_uint32(header.entry_count);
    }
} // namespace


void hyper::cache::encode_header(BinaryWriter& writer, const CacheHeader& header) {
    encode_header_without_checksum(writer, header);
    writer.write_bytes(header.index_checksum);
}

std::optional<hyper::CacheHeader> hyper::cache::decode_header(BinaryReader& reader) {
    const auto signature = reader.read_bytes(4);
    if (!signature.has_value()) {
        return std::nullopt;
    }

    const auto version = reader.read_uint32();
    if (!version.has_value()) {
        return std::nullopt;
    }

    const auto entry_count = reader.read_uint32();
    if (!entry_count.has_value()) {
        return std::nullopt;
    }

    const auto checksum = reader.read_bytes(32);
    if (!checksum.has_value()) {
        return std::nullopt;
    }

    CacheHeader header;
    std::ranges::copy(signature.value(), header.signature.begin());
    header.version = version.value();
    header.entry_count = entry_count.value();
    std::ranges::copy(checksum.value(), header.index_checksum.begin());
    return header;
}

bool hyper::cache::encode_entry(BinaryWriter& writer, const CacheEntry& entry) {
    const auto path = encode_relative_path(entry.path);
    if (!path.has_value()) {
        return false;
    }

    writer.write_int64(entry.modified_time_ns);
    writer.write_uint64(entry.file_size);
    writer.write_bytes(entry.obj_id);
    writer.write_string(path.value());
    return true;
}

std::optional<hyper::CacheEntry> hyper::cache::decode_entry(BinaryReader& reader) {
    const auto modified_time_ns = reader.read_int64();
    if (!modified_time_ns.has_value()) {
        return std::nullopt;
    }

    const auto file_size = reader.read_uint64();
    if (!file_size.has_value()) {
        return std::nullopt;
    }

    const auto object_identifier = reader.read_bytes(32);
    if (!object_identifier.has_value()) {
        return std::nullopt;
    }

    const auto name = reader.read_string();
    if (!name.has_value() || name.value().empty()) {
        return std::nullopt;
    }

    const auto path = decode_relative_path(name.value());
    if (!path.has_value()) {
        return std::nullopt;
    }

    CacheEntry entry{
        .modified_time_ns = modified_time_ns.value(),
        .file_size = file_size.value(),
        .path = path.value(),
    };
    std::ranges::copy(object_identifier.value(), entry.obj_id.begin());

    return entry;
}

std::vector<std::byte> hyper::cache::encode_cache(const CacheIndex& cache_index) {
    const auto& [source_header, entries] = cache_index;
    auto header = source_header;
    if (entries.size() > std::numeric_limits<std::uint32_t>::max()) {
        return {};
    }

    header.entry_count = static_cast<std::uint32_t>(entries.size());

    BinaryWriter entry_writer;
    for (const auto& entry : entries) {
        if (!encode_entry(entry_writer, entry)) {
            return {};
        }
    }

    BinaryWriter checksum_writer;
    encode_header_without_checksum(checksum_writer, header);
    checksum_writer.write_bytes(entry_writer.bytes());

    const auto checksum = sha256::calculate_sha256(checksum_writer.bytes());
    if (!checksum.has_value()) {
        return {};
    }

    header.index_checksum = checksum.value();

    BinaryWriter result_writer;
    encode_header(result_writer, header);
    result_writer.write_bytes(entry_writer.bytes());

    const auto result = result_writer.bytes();
    return {result.begin(), result.end()};
}

std::optional<hyper::CacheIndex> hyper::cache::decode_cache(const std::span<const std::byte> data) {
    auto reader = BinaryReader(data);
    auto header_opt = decode_header(reader);
    if (!header_opt.has_value()) {
        return std::nullopt;
    }
    const auto& header = header_opt.value();
    if (header.signature != FormatSignature || header.version != CurVersion) {
        return std::nullopt;
    }
    std::vector<CacheEntry> entries;
    entries.reserve(header.entry_count);
    for (std::size_t i = 0; i < header.entry_count; ++i) {
        auto entry_opt = decode_entry(reader);
        if (!entry_opt.has_value()) {
            return std::nullopt;
        }
        entries.emplace_back(entry_opt.value());
    }

    if (reader.position() != data.size()) {
        return std::nullopt;
    }

    constexpr std::size_t checksum_offset = 12;
    constexpr std::size_t checksum_size = 32;
    if (data.size() < checksum_offset + checksum_size) {
        return std::nullopt;
    }

    BinaryWriter checksum_writer;
    checksum_writer.write_bytes(data.subspan(0, checksum_offset));
    checksum_writer.write_bytes(data.subspan(checksum_offset + checksum_size, data.size() - checksum_offset - checksum_size));

    if (const auto checksum = sha256::calculate_sha256(checksum_writer.bytes()); !checksum.has_value() || checksum.value() != header.index_checksum) {
        return std::nullopt;
    }
    return std::make_optional<CacheIndex>(header, entries);
}
