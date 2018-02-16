#include <vk/physical_device.h>
#include <vk/utils.h>

namespace shiny::vk {

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

VkDeviceQueueCreateInfo& const
physical_device::device_queue_create_info(const VkStructureType vk_struct_type,
                                          uint32_t              queue_family_index,
                                          uint32_t              queue_count,
                                          float&                queue_priority) const
{
    VkDeviceQueueCreateInfo queue_create_info = {};
    queue_create_info.sType                   = vk_struct_type;
    queue_create_info.queueFamilyIndex        = queue_family_index;
    queue_create_info.queueCount              = queue_count;
    queue_create_info.pQueuePriorities        = &queue_priority;
    return queue_create_info;
}

VkDeviceCreateInfo& const
physical_device::device_create_info(VkStructureType                 vk_struct_type,
                                    VkDeviceQueueCreateInfo&        queue_create_info,
                                    uint32_t                        queue_create_info_count,
                                    VkPhysicalDeviceFeatures&       device_features,
                                    uint32_t                        enabled_extension_count,
                                    const std::vector<const char*>* enabled_layers) const
{
    VkDeviceCreateInfo create_info    = {};
    create_info.sType                 = vk_struct_type;
    create_info.pQueueCreateInfos     = &queue_create_info;
    create_info.queueCreateInfoCount  = queue_create_info_count;
    create_info.pEnabledFeatures      = &device_features;
    create_info.enabledExtensionCount = enabled_extension_count;

    if (enabled_layers) {
        create_info.enabledLayerCount   = static_cast<uint32_t>(enabled_layers->size());
        create_info.ppEnabledLayerNames = enabled_layers->data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    return create_info;
}


physical_device::physical_device(const VkPhysicalDevice&            device,
                                 const std::optional<ext::surface>& surface)
  : m_device(device)
  , m_indices(find_queue_families(surface))
{}


logical_device
physical_device::create_logical_device(const std::vector<const char*>* enabled_layers) const
{
    float                   queue_priority    = 1.0;
    VkDeviceQueueCreateInfo queue_create_info = device_queue_create_info(
      VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, m_indices.graphics_family(), 1, queue_priority);
    VkPhysicalDeviceFeatures device_features = {};
    VkDeviceCreateInfo       create_info =
      device_create_info(VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, queue_create_info, 1,
                         device_features, 0, enabled_layers);

    VkDevice device;
    if (vkCreateDevice(m_device, &create_info, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a logical device!");
    }

    return logical_device(device, m_indices);
}

}  // namespace shiny::vk
