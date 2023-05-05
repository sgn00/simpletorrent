#include "simpletorrent/Logger.h"

namespace simpletorrent {

namespace {
std::shared_ptr<spdlog::logger> instance_ = nullptr;
}

namespace logger {
void initialize(const std::string &filename) {
  if (!instance_) {
    instance_ = spdlog::basic_logger_mt("simpletorrent", filename);
  }
  spdlog::flush_every(std::chrono::seconds(3));
}

spdlog::logger &instance() {
  if (!instance_) {
    throw std::runtime_error("Logger not initialized");
  }
  return *instance_;
}
}  // namespace logger

}  // namespace simpletorrent