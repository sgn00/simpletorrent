#pragma once
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

#define LOG_TRACE(...) Logger::instance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) Logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::instance().info(__VA_ARGS__)
#define LOG_WARN(...) Logger::instance().warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::instance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) Logger::instance().critical(__VA_ARGS__)
#define LOG_FLUSH() Logger::instance().flush()

namespace simpletorrent {
class Logger {
 public:
  static void initialize(const std::string &filename);
  static spdlog::logger &instance();

 private:
  Logger() = default;
  static std::shared_ptr<spdlog::logger> instance_;
};

}  // namespace simpletorrent