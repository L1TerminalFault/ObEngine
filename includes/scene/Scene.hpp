#pragma once
#include <entt/entt.hpp>

namespace ob {
class Scene {
public:
  Scene() = default;
  ~Scene() = default;

  entt::entity createEntity() { return m_registry.create(); }
  void destroyEntity(entt::entity entity) { m_registry.destroy(entity); }

  entt::registry &registry() { return m_registry; }
  const entt::registry &registry() const { return m_registry; }

private:
  entt::registry m_registry;
};
} // namespace ob
