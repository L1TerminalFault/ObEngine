#include "log/log.hpp"
#include "rhi/vulkan_renderer.hpp"
#include <cstring>

namespace ob {
// Append to the bottom of Render.cpp

std::expected<void, std::string>
VulkanRenderer::uploadBuffer(const void *data, VkDeviceSize size,
                             VkBufferUsageFlags usage, VkBuffer &outBuffer,
                             VmaAllocation &outAllocation) {
  VkBufferCreateInfo stagingBufferInfo{};
  stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingBufferInfo.size = size;
  stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo stagingAllocInfo{};
  stagingAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;
  stagingAllocInfo.flags =
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
      VMA_ALLOCATION_CREATE_MAPPED_BIT;

  VkBuffer stagingBuffer;
  VmaAllocation stagingAllocation;
  VmaAllocationInfo stagingAllocResult;

  if (vmaCreateBuffer(m_allocator, &stagingBufferInfo, &stagingAllocInfo,
                      &stagingBuffer, &stagingAllocation,
                      &stagingAllocResult) != VK_SUCCESS) {
    return std::unexpected(
        "Vulkan Allocation Error: Failed to allocate staging buffer via VMA.");
  }

  std::memcpy(stagingAllocResult.pMappedData, data, size);

  VkBufferCreateInfo deviceBufferInfo{};
  deviceBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  deviceBufferInfo.size = size;
  deviceBufferInfo.usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  deviceBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  VmaAllocationCreateInfo deviceAllocInfo{};
  deviceAllocInfo.usage = VMA_MEMORY_USAGE_AUTO;

  if (vmaCreateBuffer(m_allocator, &deviceBufferInfo, &deviceAllocInfo,
                      &outBuffer, &outAllocation, nullptr) != VK_SUCCESS) {
    vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);
    return std::unexpected("Vulkan Allocation Error: Failed to allocate device "
                           "local buffer via VMA.");
  }

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = m_commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer transferCmd;
  vkAllocateCommandBuffers(m_device, &allocInfo, &transferCmd);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(transferCmd, &beginInfo);

  VkBufferCopy copyRegion{};
  copyRegion.size = size;
  vkCmdCopyBuffer(transferCmd, stagingBuffer, outBuffer, 1, &copyRegion);

  vkEndCommandBuffer(transferCmd);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &transferCmd;

  vkQueueSubmit(m_graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(m_graphics_queue);

  vkFreeCommandBuffers(m_device, m_commandPool, 1, &transferCmd);
  vmaDestroyBuffer(m_allocator, stagingBuffer, stagingAllocation);

  return {};
}

MeshHandle VulkanRenderer::uploadMesh(std::span<const Vertex> vertices,
                                      std::span<const uint32_t> indices) {
  VulkanMeshBackend mesh{};
  mesh.vertexCount = static_cast<uint32_t>(vertices.size());
  mesh.indexCount = static_cast<uint32_t>(indices.size());

  auto vboRes = uploadBuffer(vertices.data(), sizeof(Vertex) * vertices.size(),
                             VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                             mesh.vertexBuffer, mesh.vertexAllocation);
  if (!vboRes) {
    OB_CORE_ERROR("Mesh Upload Error: Vertex storage creation failed.");
    return 0;
  }

  if (!indices.empty()) {
    auto iboRes =
        uploadBuffer(indices.data(), sizeof(uint32_t) * indices.size(),
                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT, mesh.indexBuffer,
                     mesh.indexAllocation);
    if (!iboRes) {
      vmaDestroyBuffer(m_allocator, mesh.vertexBuffer, mesh.vertexAllocation);
      OB_CORE_ERROR("Mesh Upload Error: Index storage creation failed.");
      return 0;
    }
  }

  MeshHandle handle = m_next_mesh_handle++;
  m_meshes[handle] = mesh;
  return handle;
}

void VulkanRenderer::destroyMesh(MeshHandle handle) {
  auto it = m_meshes.find(handle);
  if (it != m_meshes.end()) {
    vmaDestroyBuffer(m_allocator, it->second.vertexBuffer,
                     it->second.vertexAllocation);
    if (it->second.indexBuffer != VK_NULL_HANDLE) {
      vmaDestroyBuffer(m_allocator, it->second.indexBuffer,
                       it->second.indexAllocation);
    }
    m_meshes.erase(it);
  }
}
} // namespace ob
