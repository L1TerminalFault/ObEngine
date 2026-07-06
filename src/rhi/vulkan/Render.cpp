#include "log/log.hpp"
#include "rhi/vulkan_renderer.hpp"
#include "scene/Components.hpp"
#include <array>
#include <glm/glm.hpp>

namespace ob {
static void recordCommandBuffer(
    VkCommandBuffer commandBuffer, VkRenderPass renderPass,
    VkFramebuffer framebuffer, VkExtent2D extent, VkPipeline graphicsPipeline,
    VkPipelineLayout pipelineLayout, std::span<const RenderItem> renderQueue,
    const std::unordered_map<MeshHandle, VulkanRenderer::VulkanMeshBackend>
        &meshCache) {
  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    OB_CORE_ERROR("Vulkan Execution Error: Failed to begin recording command "
                  "buffer stream.");
    return;
  }

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.02f, 0.04f, 0.06f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = framebuffer;
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = extent;
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(extent.width);
  viewport.height = static_cast<float>(extent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = extent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  for (const auto &item : renderQueue) {
    auto it = meshCache.find(item.handle);
    if (it == meshCache.end())
      continue; // Omit unregistered/invalid meshes safely

    const auto &backendMesh = it->second;

    if (backendMesh.vertexBuffer != VK_NULL_HANDLE) {
      // Push calculation down
      vkCmdPushConstants(commandBuffer, pipelineLayout,
                         VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4),
                         &item.transform);

      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffer, 0, 1, &backendMesh.vertexBuffer,
                             offsets);

      if (backendMesh.indexBuffer != VK_NULL_HANDLE) {
        vkCmdBindIndexBuffer(commandBuffer, backendMesh.indexBuffer, 0,
                             VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(commandBuffer, backendMesh.indexCount, 1, 0, 0, 0);
      } else {
        vkCmdDraw(commandBuffer, backendMesh.vertexCount, 1, 0, 0);
      }
    }
  }

  vkCmdEndRenderPass(commandBuffer);

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    OB_CORE_ERROR("Vulkan Execution Error: Failed to finalize command buffer "
                  "compilation.");
  }
}

void VulkanRenderer::present(std::span<const RenderItem> renderQueue) {
  vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE,
                  UINT64_MAX);

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(
      m_device, m_swapchain, UINT64_MAX,
      m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR)
    return;
  else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    OB_CORE_ERROR("Vulkan Execution Error: Critical failure acquiring next "
                  "swapchain image frame.");
    return;
  }

  vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
  vkResetCommandBuffer(m_commandBuffers[m_currentFrame], 0);

  recordCommandBuffer(m_commandBuffers[m_currentFrame], m_renderPass,
                      m_swapChainFramebuffers[imageIndex], m_swapChainExtent,
                      m_graphicsPipeline, m_pipelineLayout, renderQueue,
                      m_meshes);

  VkSubmitInfo submitInfo{.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO};
  VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

  VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[imageIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(m_graphics_queue, 1, &submitInfo,
                    m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
    OB_CORE_ERROR("Vulkan Execution Error: Failed to submit command buffer "
                  "payload to the device graphics queue.");
  }

  VkPresentInfoKHR presentInfo{.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
                               .waitSemaphoreCount = 1,
                               .pWaitSemaphores = signalSemaphores};
  VkSwapchainKHR swapChains[] = {m_swapchain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &imageIndex;

  result = vkQueuePresentKHR(m_present_queue, &presentInfo);
  if (result == VK_ERROR_OUT_OF_DATE_KHR ||
      result == VK_SUBOPTIMAL_KHR) { /* handle resize */
  } else if (result != VK_SUCCESS) {
    OB_CORE_ERROR("Vulkan Execution Error: Critical failure presenting "
                  "finalized frame to display surface.");
  }

  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

} // namespace ob
