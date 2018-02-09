#include <vk/logical_device.h>

// Debug include, remove when we're done with this bit
#include <exception>

namespace shiny::vk {

logical_device::~logical_device()
{
    destroy();
}

void
logical_device::create(const physical_device&          physical_device,
                       const std::vector<const char*>* enabled_layers)
{
    queue_family_indices indices = find_queue_families(physical_device);

    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex = indices.graphics_family;
    queue_create_info.queueCount       = 1;

    float queue_priority               = 1.0;
    queue_create_info.pQueuePriorities = &queue_priority;

    VkPhysicalDeviceFeatures device_features = {};

    VkDeviceCreateInfo create_info    = {};
    create_info.sType                 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pQueueCreateInfos     = &queue_create_info;
    create_info.queueCreateInfoCount  = 1;
    create_info.pEnabledFeatures      = &device_features;
    create_info.enabledExtensionCount = 0;

    if (enabled_layers) {
        create_info.enabledLayerCount =
          static_cast<uint32_t>(enabled_layers->size());
        create_info.ppEnabledLayerNames = enabled_layers->data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physical_device, &create_info, nullptr, &m_device)
        != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a logical device!");
    }
}

void
logical_device::destroy()
{
    vkDestroyDevice(m_device, nullptr);
}

}  // namespace shiny::vk
