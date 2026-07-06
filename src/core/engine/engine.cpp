#include "engine/engine.hpp"
#include "log/log.hpp"
#include "rhi/vulkan_renderer.hpp"
#include "scene/Scene.hpp"
#include "windowing/glfw_window.hpp"
#include "windowing/window.hpp"
#include <memory>

namespace ob {

Engine::Engine() : isRunning(false), frameCount(0) { ob::Log::init(); }

Engine::~Engine() {
  if (m_renderer) {
    m_renderer->waitDeviceIdle();
  }
  if (m_active_scene) {
    auto view = m_active_scene->registry().view<MeshComponent>();
    for (auto entity : view) {
      auto &mesh = view.get<MeshComponent>(entity);
      if (mesh.handle != 0) {
        m_renderer->destroyMesh(mesh.handle);
        mesh.handle = 0;
      }
    }
  }
  if (m_renderer) {
    m_renderer->shutdown();
    m_renderer.reset();
  }
  m_engine_window->shutdown();
  OB_CORE_INFO("Engine structures successfully broken down via RAII.");
}

std::expected<void, std::string> Engine::init() {
  WindowConfig config{};
  config.height = 900;
  config.width = 1200;
  config.title = "Oblique Engine";

  m_engine_window = std::make_unique<GlfwWindow>();

  auto windowRes = m_engine_window->init(config);
  if (!windowRes) {
    return std::unexpected("Window System Initialization Failed: " +
                           windowRes.error());
  }

  ob::NativeWindowHandle nativeHandle = m_engine_window->get_native_handle();

  ob::RendererConfig rendererConfig{};
  rendererConfig.width = config.width;
  rendererConfig.height = config.height;
  rendererConfig.vsync = true;
#ifdef NDEBUG
  rendererConfig.validation = false;
#else
  rendererConfig.validation = true;
#endif

  OB_CORE_INFO("Initializing Renderer context...");
  m_renderer = std::make_unique<VulkanRenderer>();

  auto rendererRes = m_renderer->init(nativeHandle, rendererConfig);
  if (!rendererRes) {
    return std::unexpected("Renderer Initialization Failed: " +
                           rendererRes.error());
  }
  m_active_scene = std::make_unique<Scene>();

  // Set up a simple 2D triangle
  std::vector<Vertex> triVertices = {
      {{0.0f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.5f, 0.0f}}, // Top (Red)
      {{0.5f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
      {{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}};
  std::vector<uint32_t> triIndices = {0, 2, 1};

  MeshHandle triHandle = m_renderer->uploadMesh(triVertices, triIndices);
  if (triHandle == 0) {
    return std::unexpected("Failed to populate 2D triangle assets.");
  }

  auto triEntity = m_active_scene->createEntity();
  m_active_scene->registry().emplace<TagComponent>(triEntity, "2D Triangle");

  m_active_scene->registry().emplace<TransformComponent>(
      triEntity, glm::vec3(0.0f, 0.0f, 0.0f), // Translation
      glm::vec3(0.0f, 0.0f, 0.0f),            // Rotation
      glm::vec3(1.0f, 1.0f, 1.0f)             // Scale
  );

  m_active_scene->registry().emplace<MeshComponent>(triEntity, triHandle);

  m_last_frame_time = std::chrono::steady_clock::now();
  OB_CORE_INFO("Engine core and graphics subsystems successfully loaded.");
  return {};
}

void Engine::run() {
  isRunning = true;

  while (isRunning) {
    m_engine_window->poll_events();

    if (m_engine_window->should_close()) {
      isRunning = false;
      break;
    }

    auto current_time = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsed = current_time - m_last_frame_time;
    float deltaTime = elapsed.count();
    m_last_frame_time = current_time;

    if (deltaTime > 0.1f)
      deltaTime = 0.1f;

    update(deltaTime);
    render();
    frameCount++;
  }
}

void Engine::update(float deltaTime) {
  auto view =
      m_active_scene->registry().view<TransformComponent, MeshComponent>();
  for (auto entity : view) {
    auto &transform = view.get<TransformComponent>(entity);
    // Flat linear spin on Z-axis for 2D rotation
    transform.rotation.z += 0.5f * deltaTime;
  }
}

void Engine::render() {
  std::vector<RenderItem> renderQueue;
  auto view =
      m_active_scene->registry().view<TransformComponent, MeshComponent>();

  for (auto entity : view) {
    const auto &transform = view.get<TransformComponent>(entity);
    const auto &mesh = view.get<MeshComponent>(entity);

    renderQueue.push_back(
        {.handle = mesh.handle, .transform = transform.getTransform()});
  }

  m_renderer->present(renderQueue);
}

} // namespace ob
