#include "windowing/glfw_window.hpp"
#include "rhi/renderer.hpp"
#include "windowing/window.hpp"
#include <GLFW/glfw3.h>
#include <cstdint>
#include <expected>

namespace ob {

GlfwWindow::GlfwWindow() {}
GlfwWindow::~GlfwWindow() { shutdown(); }
std::expected<void, std::string> GlfwWindow::init(WindowConfig &windowConfig) {
  m_config = windowConfig;
  if (!glfwInit()) {
    return std::unexpected("Failed to initialize GLFW");
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
  glfwWindowHint(GLFW_FOCUS_ON_SHOW, GLFW_TRUE);

  m_window = glfwCreateWindow(m_config.width, m_config.height,
                              m_config.title.c_str(), nullptr, nullptr);

  if (!m_window) {
    return std::unexpected("unabled to create glfw window");
  }
  // show window explicitly
  glfwShowWindow(m_window);

  uint32_t glfwExtensionsCount = 0;
  const char **glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
  if (glfwExtensions) {
    m_extensions.assign(glfwExtensions, glfwExtensions + glfwExtensionsCount);
  }
  return {};
}

void GlfwWindow::poll_events() { glfwPollEvents(); }
bool GlfwWindow::should_close() const {
  return glfwWindowShouldClose(m_window);
}
void GlfwWindow::shutdown() {
  if (m_window) {
    glfwDestroyWindow(m_window);
    m_window = nullptr;
    glfwTerminate();
  }
}

NativeWindowHandle GlfwWindow::get_native_handle() const {
  NativeWindowHandle handle{};
  handle.window = static_cast<void *>(m_window);
  handle.required_instance_extensions = m_extensions;
  return handle;
}
} // namespace ob
