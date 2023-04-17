cmake_minimum_required(VERSION 3.16)

project(simpletorrent VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)

include(FetchContent)

FetchContent_Declare(asio GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
                          GIT_TAG f693a3eb7fe72a5f19b975289afc4f437d373d9c)

FetchContent_MakeAvailable(asio)

FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 0817715923c9705e68994eb52ef9df3f6845beba)
FetchContent_MakeAvailable(cpr)


file(GLOB_RECURSE SIMPLETORRENT_SOURCES "src/*.cpp" EXCLUDE "src/main.cpp")

add_library(simpletorrent_lib ${SIMPLETORRENT_SOURCES})

message(STATUS "Source files: ${SIMPLETORRENT_SOURCES}")

target_include_directories(simpletorrent_lib PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/external/include ${asio_SOURCE_DIR}/asio/include)

target_link_libraries(simpletorrent_lib PRIVATE Threads::Threads cpr::cpr)

add_executable(simpletorrent src/main.cpp)

target_include_directories(simpletorrent PUBLIC ${CMAKE_SOURCE_DIR}/include ${CMAKE_SOURCE_DIR}/external/include)

target_link_libraries(simpletorrent PRIVATE simpletorrent_lib Threads::Threads)

add_subdirectory(tests)

enable_testing()