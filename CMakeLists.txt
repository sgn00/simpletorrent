cmake_minimum_required(VERSION 3.16)

project(simpletorrent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)

file(GLOB_RECURSE SIMPLETORRENT_SOURCES "src/*.cpp" EXCLUDE "src/main.cpp")

add_library(simpletorrent_lib ${SIMPLETORRENT_SOURCES})

target_include_directories(simpletorrent_lib PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/external/include)

target_link_libraries(simpletorrent_lib PRIVATE Threads::Threads)

add_executable(simpletorrent src/main.cpp)

target_include_directories(simpletorrent PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/external/include)

target_link_libraries(simpletorrent PRIVATE simpletorrent_lib Threads::Threads)

add_subdirectory(tests)

enable_testing()