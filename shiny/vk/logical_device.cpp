#include <vk/logical_device.h>

// Debug include, remove when we're done with this bit
#include <exception>

namespace shiny::vk {

    void logical_device::create(const physical_device& physical_device)
    {
        queue_family_indices indices = find_queue_families(physical_device);

        VkDeviceQueueCreateInfo queue_create_info = {};
        queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info.queueFamilyIndex = indices.graphics_family;
        queue_create_info.queueCount = 1;

        float queue_priority = 1.0;
        queue_create_info.pQueuePriorities = &queue_priority;

        throw std::runtime_error("We're not done with this function yet");
    }

}