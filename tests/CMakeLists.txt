file(GLOB_RECURSE TEST_SOURCES "*.cpp")

add_executable(simpletorrent_tests ${TEST_SOURCES})

target_link_libraries(simpletorrent_tests PRIVATE simpletorrent_lib)

target_include_directories(simpletorrent_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/external/include")

add_test(NAME simpletorrent_tests COMMAND simpletorrent_tests)