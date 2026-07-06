#pragma once
#include "rhi/renderer.hpp"
#include "scene/Scene.hpp"
#include "windowing/window.hpp"
#include <chrono>
#include <memory>

namespace ob {
class Engine {
public:
  Engine();
  ~Engine();
  std::expected<void, std::string> init();
  void run();

private:
  void update(float deltaTime);
  void render();

private:
  std::chrono::time_point<std::chrono::steady_clock> m_last_frame_time;
  bool isRunning;
  unsigned int frameCount;

  std::unique_ptr<IWindow> m_engine_window;
  std::unique_ptr<IRenderer> m_renderer;
  std::unique_ptr<Scene> m_active_scene;
};
} // namespace ob
