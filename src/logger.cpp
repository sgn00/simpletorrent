#include "simpletorrent/Logger.h"

namespace simpletorrent {
std::shared_ptr<spdlog::logger> Logger::instance_ = nullptr;

void Logger::initialize(const std::string &filename) {
  if (!instance_) {
    instance_ = spdlog::basic_logger_mt("file_logger", filename);
  }
  spdlog::flush_every(std::chrono::seconds(3));
}

spdlog::logger &Logger::instance() {
  if (!instance_) {
    throw std::runtime_error("Logger not initialized");
  }
  return *instance_;
}
}  // namespace simpletorrent