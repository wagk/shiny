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
    auto indices = _generate_queue_indices(m_indices);
    return _create_logical_device(m_device, m_indices, m_enabled_layers, indices);
}

std::vector<queue::create_info>
physical_device::generate_queue_info() const
{
    std::vector<queue::create_info> queue_info_structs;

    return queue_info_structs;
}

logical_device
physical_device::_create_logical_device(const VkPhysicalDevice&         phys_device,
                                        const queue_families&           queue_fam,
                                        const std::vector<const char*>& enabled_layers,
                                        // Every entry in this vec<int> must be unique
                                        const std::set<int>& queue_family_indices) const
{
    const float queue_priority = 1.0;

    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    for (int queue_index : queue_family_indices) {
        queue::create_info create_info = {};

        create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        create_info.queueFamilyIndex = queue_index;
        create_info.queueCount       = 1;  // the number of queues per queue family
        create_info.pQueuePriorities = &queue_priority;

        queue_create_infos.push_back(create_info);
    }

    VkPhysicalDeviceFeatures device_features = {};

    VkDeviceCreateInfo create_info    = {};
    create_info.sType                 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount  = (uint32_t)queue_create_infos.size();
    create_info.pQueueCreateInfos     = queue_create_infos.data();
    create_info.enabledExtensionCount = 0;
    create_info.pEnabledFeatures      = &device_features;

    if (m_enabled_layers.empty() == false) {
        create_info.enabledLayerCount   = static_cast<uint32_t>(enabled_layers.size());
        create_info.ppEnabledLayerNames = enabled_layers.data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    VkDevice device;
    if (vkCreateDevice(phys_device, &create_info, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a logical device!");
    }

    return logical_device(device, queue_fam);
}

/*
  Every queue family that is discovered has their own unique queue index;
  We have to make sure that these indices are unique when we populate the
  VkDeviceQueueCreateInfo structs.

  // TODO: Find out what happens if we submit multiple requests of the same
  // queue family then request for a device
  */
const std::set<int>
physical_device::_generate_queue_indices(const queue_families& fam) const
{
    return std::set<int>{ fam.graphics_family(), fam.presentation_family() };
}

}  // namespace shiny::vk
