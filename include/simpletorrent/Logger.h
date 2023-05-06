#pragma once
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#define LOG_TRACE(...) logger::instance().trace(__VA_ARGS__)
#define LOG_DEBUG(...) logger::instance().debug(__VA_ARGS__)
#define LOG_INFO(...) logger::instance().info(__VA_ARGS__)
#define LOG_WARN(...) logger::instance().warn(__VA_ARGS__)
#define LOG_ERROR(...) logger::instance().error(__VA_ARGS__)
#define LOG_CRITICAL(...) logger::instance().critical(__VA_ARGS__)
#define LOG_FLUSH() logger::instance().flush()

namespace simpletorrent {

namespace logger {
void initialize(const std::string &filename);
spdlog::logger &instance();
}  // namespace logger

}  // namespace simpletorrent