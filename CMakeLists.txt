cmake_minimum_required(VERSION 3.16)

project(simpletorrent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB_RECURSE SIMPLETORRENT_SOURCES "src/*.cpp")

add_executable(simpletorrent ${SIMPLETORRENT_SOURCES})

add_library(simpletorrent_lib ${SIMPLETORRENT_SOURCES})

target_include_directories(simpletorrent PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_include_directories(simpletorrent_lib PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_subdirectory(tests)

enable_testing()