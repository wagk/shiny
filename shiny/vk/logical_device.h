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

      // TODO: We need to add parameters to this function, to get different
      // queue types. We might also need to define some enums and adjust some
      // interfaces for this to work

      // TODO: Be able to query presentation queues from it, not just graphics
      // like what it's doing right now

      // NOTE: This will now take in an index that must be provided. This index
      // would represent the queue family index that is maintained by
      // queue_families
      */
    queue get_queue(int index) const;

private:
    VkDevice       m_device = VK_NULL_HANDLE;
    queue_families m_indices;
};  // namespace shiny::vk

// device is the common term, not logical_device, even though logical_device is
// factually correct
using device = logical_device;

}  // namespace shiny::vk
