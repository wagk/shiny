#include <vk/physical_device.h>
#include <vk/utils.h>

namespace shiny::vk {

queue_families
physical_device::find_queue_families() const
{
    queue_families indices;

    auto queue_families =
      collect<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, m_device);

    // we just want to find one card with VK_QUEUE_GRAPHICS_BIT
    int i = 0;
    for (const auto& families : queue_families) {
        if (families.queueCount > 0 && families.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphics_family = i;
        }
        if (indices.is_complete()) {
            indices.m_props = families;
            break;
        }
        ++i;
    }

    return indices;
}

/*
    Suitability is determined by how much queue family features does this card specify
*/
bool
physical_device::is_device_suitable() const
{
    return m_indices.is_complete();
}

physical_device::physical_device(const VkPhysicalDevice& device)
  : m_device(device)
  , m_indices()
{
    m_indices = find_queue_families();
}

logical_device
physical_device::create_logical_device(const std::vector<const char*>* enabled_layers) const
{

    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex        = m_indices.graphics_family;
    queue_create_info.queueCount              = 1;

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
        create_info.enabledLayerCount   = static_cast<uint32_t>(enabled_layers->size());
        create_info.ppEnabledLayerNames = enabled_layers->data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    VkDevice device;
    if (vkCreateDevice(m_device, &create_info, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a logical device!");
    }

    return logical_device(device, m_indices);
}

}  // namespace shiny::vk
