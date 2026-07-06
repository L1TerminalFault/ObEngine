#pragma once

#include "rhi/renderer.hpp"
#include "windowing/window.hpp"
#include <expected>
#include <string>
#include <vector>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace ob {

class GlfwWindow : public IWindow {
public:
  GlfwWindow();
  ~GlfwWindow() override;

  std::expected<void, std::string> init(WindowConfig &windowConfig) override;
  void poll_events() override;
  [[nodiscard]] bool should_close() const override;
  void shutdown() override;

  NativeWindowHandle get_native_handle() const override;

private:
  WindowConfig m_config;
  GLFWwindow *m_window = nullptr;
  std::vector<const char *> m_extensions;
};

} // namespace ob
