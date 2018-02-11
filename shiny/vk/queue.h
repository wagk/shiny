#pragma once

#include <vulkan/vulkan.h>

namespace shiny::vk {

class queue
{
public:
    explicit queue(const VkQueue& queue);

    operator VkQueue() const { return m_queue; }
    operator VkQueue&() { return m_queue; }

private:
    VkQueue m_queue = VK_NULL_HANDLE;
};

}  // namespace shiny::vk
