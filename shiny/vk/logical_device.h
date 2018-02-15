#pragma once

#include <vulkan\vulkan.h>

#include <vk/queue.h>
#include <vk/queue_families.h>

namespace shiny::vk {

/*
  Logical devices are interfaces to the physical device.
*/
class logical_device
{
public:
    explicit logical_device(VkDevice device, const queue_families& indices);
    ~logical_device();

    logical_device(const logical_device&) = delete;
    logical_device& operator=(const logical_device&) = delete;

    logical_device(logical_device&& other);
    logical_device& operator=(logical_device&& other);

    /*
      Queues belong to the logical device, they automatically created along with
      the device, but we need to assign handles to them
    */
    queue get_queue() const;

private:
    VkDevice       m_device = VK_NULL_HANDLE;
    queue_families m_indices;
};  // namespace shiny::vk

// device is the common term, not logical_device, even though logical_device is
// factually correct
using device = logical_device;

}  // namespace shiny::vk
