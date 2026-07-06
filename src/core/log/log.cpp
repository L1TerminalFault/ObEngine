#include "log/log.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>

namespace ob {
std::shared_ptr<spdlog::logger> Log::s_coreLogger;

void Log::init() {
  auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

  // [Timestamp] [Logger Name] [Log Level] actual message
  consoleSink->set_pattern("%^[%T] %n: %v%$");

  s_coreLogger = std::make_shared<spdlog::logger>("OBLIQUE", consoleSink);
  spdlog::register_logger(s_coreLogger);

  s_coreLogger->set_level(spdlog::level::trace);
}
} // namespace ob
