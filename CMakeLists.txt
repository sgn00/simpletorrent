cmake_minimum_required(VERSION 3.16)

project(simpletorrent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# set(CMAKE_CXX_FLAGS "-O3")
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -rdynamic -g")
# set(CMAKE_CXX_FLAGS_DEBUG "-O3")

find_package(Threads REQUIRED)

include(FetchContent)

FetchContent_Declare(asio GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
                          GIT_TAG f693a3eb7fe72a5f19b975289afc4f437d373d9c)

FetchContent_MakeAvailable(asio)

FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 0817715923c9705e68994eb52ef9df3f6845beba)
FetchContent_MakeAvailable(cpr)

FetchContent_Declare(readerwriterqueue GIT_REPOSITORY https://github.com/cameron314/readerwriterqueue
                                        GIT_TAG 8e7627d18c2108aca178888d88514179899a044f)
FetchContent_MakeAvailable(readerwriterqueue)

FetchContent_Declare(indicators GIT_REPOSITORY https://github.com/p-ranav/indicators
                                  GIT_TAG v2.3)
FetchContent_MakeAvailable(indicators)

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.1.0
)
FetchContent_MakeAvailable(spdlog)

# Build library
file(GLOB_RECURSE SIMPLETORRENT_SOURCES "src/*.cpp")
# Exclude main.cpp from the list of sources for the library
list(FILTER SIMPLETORRENT_SOURCES EXCLUDE REGEX "src/main.cpp")
add_library(simpletorrent_lib ${SIMPLETORRENT_SOURCES})
target_include_directories(simpletorrent_lib PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/external/include ${asio_SOURCE_DIR}/asio/include)
target_link_libraries(simpletorrent_lib PUBLIC Threads::Threads cpr::cpr readerwriterqueue spdlog::spdlog indicators)

# Main executable
add_executable(simpletorrent src/main.cpp)
target_link_libraries(simpletorrent PRIVATE simpletorrent_lib)

add_subdirectory(tests)

enable_testing()