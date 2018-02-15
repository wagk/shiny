#include <vk/logical_device.h>

// Debug include, remove when we're done with this bit
#include <exception>
#include <iostream>
#include <utility>

namespace shiny::vk {

logical_device::logical_device(VkDevice device, const queue_families& indices)
  : m_device(device)
  , m_indices(indices)
{}

logical_device::~logical_device()
{
    vkDestroyDevice(m_device, nullptr);
}

logical_device::logical_device(logical_device&& other)
  : m_device(std::move(other.m_device))
  , m_indices(std::move(other.m_indices))
{
    other.m_device = VK_NULL_HANDLE;
}

logical_device&
logical_device::operator=(logical_device&& other)
{
    vkDestroyDevice(m_device, nullptr);

    m_device  = std::move(other.m_device);
    m_indices = std::move(other.m_indices);

    other.m_device = VK_NULL_HANDLE;

    return *this;
}

queue
logical_device::get_queue() const
{
    VkQueue device_queue;
    vkGetDeviceQueue(m_device, m_indices.graphics_family(), 0, &device_queue);
    return queue(device_queue);
}

}  // namespace shiny::vk
