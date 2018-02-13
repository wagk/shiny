#pragma once
/*
  Logical devices are interfaces to the physical device.

  I'm not sure why we're allowed to have multiple interfaces per physical
  device, but we are.
*/
#include <vulkan\vulkan.h>

#include <vector>

#include <vk/queue.h>
#include <vk/queue_families.h>

#include <iostream>

namespace shiny::vk {

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

    void print_device_addr() const { std::cout << m_device << std::endl; }

private:
    VkDevice       m_device = VK_NULL_HANDLE;
    queue_families m_indices;
};

// device is the common term, not logical_device, even though logical_device is
// factually correct
using device = logical_device;

}  // namespace shiny::vk
