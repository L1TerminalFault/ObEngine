#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace ob {
class Log {
public:
  static void init();

  static std::shared_ptr<spdlog::logger> &getCoreLogger() {
    return s_coreLogger;
  }

private:
  static std::shared_ptr<spdlog::logger> s_coreLogger;
};
} // namespace ob

#define OB_CORE_TRACE(...) ::ob::Log::getCoreLogger()->trace(__VA_ARGS__)
#define OB_CORE_INFO(...) ::ob::Log::getCoreLogger()->info(__VA_ARGS__)
#define OB_CORE_WARN(...) ::ob::Log::getCoreLogger()->warn(__VA_ARGS__)
#define OB_CORE_ERROR(...) ::ob::Log::getCoreLogger()->error(__VA_ARGS__)
