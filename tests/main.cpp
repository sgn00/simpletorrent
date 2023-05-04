#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"
#include "simpletorrent/Logger.h"

using namespace simpletorrent;

int main(int argc, char* argv[]) {
  logger::initialize("test.log");
  auto& instance = logger::instance();
  instance.set_level(spdlog::level::level_enum::off);

  return Catch::Session().run(argc, argv);
}