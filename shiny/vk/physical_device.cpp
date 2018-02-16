#include <vk/physical_device.h>
#include <vk/queue.h>
#include <vk/utils.h>

namespace shiny::vk {

/*
  UNDER CONSTRUCTION: Migrating to queue_families
*/
queue_families
physical_device::find_queue_families(const std::optional<ext::surface>& sur) const
{
    queue_families indices;

    auto queue_families =
      collect<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, m_device);

    for (int i = 0; i < queue_families.size(); ++i) {
        const auto& families = queue_families[i];

        bool graphics_support = families.queueFlags & VK_QUEUE_GRAPHICS_BIT;
        // if there is a surface passed in for us to query
        if (sur.has_value()) {
            VkBool32 presentation_support = false;
            // TODO: Wrap vkGetPhysicalDeviceSurfaceSupportKHR
            vkGetPhysicalDeviceSurfaceSupportKHR(m_device, i, sur.value(), &presentation_support);
            if (families.queueCount > 0 && presentation_support) {
                indices.presentation_family(i);
            }
        }

        if (families.queueCount > 0 && graphics_support) {
            indices.graphics_family(i);
        }
        if (indices.is_complete()) {
            indices.raw_properties(families);
            break;
        }
    }

    return indices;
}

void
physical_device::set_queue_families(const std::optional<ext::surface>& surface)
{
    m_indices = find_queue_families(surface);
}

/*
  Suitability is determined by how much queue family features does this card specify
*/
bool
physical_device::is_device_suitable() const
{
    return m_indices.is_complete();
}

physical_device::physical_device(const VkPhysicalDevice&            device,
                                 const std::optional<ext::surface>& surface,
                                 const std::vector<const char*>&    enabled_layers)
  : m_device(device)
  , m_indices(find_queue_families(surface))
  , m_enabled_layers(enabled_layers)
{}

/*
    It appears that the queues are created at the time of logical device creation
    Which means that all the information we need /must/ be present within this function
*/
logical_device
physical_device::create_logical_device() const
{
    float queue_priority = 1.0;

    queue::create_info queue_create_info = {};
    queue_create_info.sType              = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.queueFamilyIndex   = m_indices.graphics_family();
    queue_create_info.queueCount         = 1;
    queue_create_info.pQueuePriorities   = &queue_priority;

    VkPhysicalDeviceFeatures device_features = {};

    VkDeviceCreateInfo create_info    = {};
    create_info.sType                 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount  = 1;
    create_info.pQueueCreateInfos     = &queue_create_info;
    create_info.enabledExtensionCount = 0;
    create_info.pEnabledFeatures      = &device_features;

    if (m_enabled_layers.empty() == false) {
        create_info.enabledLayerCount   = static_cast<uint32_t>(m_enabled_layers.size());
        create_info.ppEnabledLayerNames = m_enabled_layers.data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    VkDevice device;
    if (vkCreateDevice(m_device, &create_info, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a logical device!");
    }

    return logical_device(device, m_indices);
}

std::vector<queue::create_info>
physical_device::generate_queue_info() const
{
    std::vector<queue::create_info> queue_info_structs;

    return queue_info_structs;
}


}  // namespace shiny::vk
