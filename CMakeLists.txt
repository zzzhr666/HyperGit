cmake_minimum_required(VERSION 3.20)
project(HyperGit)

# ---------------------------------------------------------------------------
# Project-wide settings
# ---------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

find_package(OpenSSL REQUIRED)
find_package(GTest REQUIRED)

find_path(ZSTD_INCLUDE_DIR
    NAMES zstd.h
    REQUIRED
)

find_library(ZSTD_LIBRARY
    NAMES zstd libzstd
    REQUIRED
)

add_library(Zstd::Zstd UNKNOWN IMPORTED)
set_target_properties(Zstd::Zstd PROPERTIES
    IMPORTED_LOCATION "${ZSTD_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIR}"
)

# ---------------------------------------------------------------------------
# Production sources
# ---------------------------------------------------------------------------

set(HYPERGIT_SOURCES
    binary_writer.cpp
    binary_reader.cpp
    cache_codec.cpp
    cache_file.cpp
    sha256.cpp
    object_id.cpp
    blob.cpp
    object_store.cpp
    repository.cpp
    compression.cpp
)

add_executable(HyperGit
    main.cpp
    ${HYPERGIT_SOURCES}
)

target_link_libraries(HyperGit
    PRIVATE
    OpenSSL::Crypto
    Zstd::Zstd
)

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

enable_testing()

add_executable(HyperGitTests
    ${HYPERGIT_SOURCES}
    tests/binary_writer_test.cpp
    tests/binary_reader_test.cpp
    tests/cache_codec_test.cpp
    tests/cache_file_test.cpp
    tests/blob_test.cpp
    tests/compression_test.cpp
    tests/object_store_test.cpp
    tests/repository_test.cpp
    tests/sha256_test.cpp
)

target_include_directories(HyperGitTests
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(HyperGitTests
    PRIVATE
    GTest::gtest_main
    OpenSSL::Crypto
    Zstd::Zstd
)

include(GoogleTest)
gtest_discover_tests(HyperGitTests)
