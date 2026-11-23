cmake_minimum_required(VERSION 3.20)
project(HyperGit)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)


add_executable(HyperGit main.cpp
        binary_writer.cpp
        binary_reader.cpp
        cache_codec.cpp
        sha256.cpp
        cache_file.cpp
        cache_file.hpp
        object_id.cpp
        blob.cpp
        object_store.cpp
)
target_link_libraries(HyperGit PRIVATE OpenSSL::Crypto)


enable_testing()

find_package(GTest REQUIRED)


add_executable(HyperGitTests
        binary_writer.cpp
        binary_reader.cpp
        cache_codec.cpp
        cache_file.cpp
        sha256.cpp
        object_id.cpp
        object_store.cpp
        blob.cpp
        tests/binary_writer_test.cpp
        tests/binary_reader_test.cpp
        tests/cache_codec_test.cpp
        tests/cache_file_test.cpp
        tests/blob_test.cpp
        tests/object_store_test.cpp
        tests/sha256_test.cpp
)

target_include_directories(HyperGitTests
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(HyperGitTests
        PRIVATE
        GTest::gtest_main
        OpenSSL::Crypto
)

include(GoogleTest)
gtest_discover_tests(HyperGitTests)
