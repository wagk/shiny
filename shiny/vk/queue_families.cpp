#include <vk/queue_families.h>
#include <vk/utils.h>

#include <vulkan/vulkan.h>

namespace shiny::vk {

/*
  UNDER CONSTRUCTION: Migrating from physical_device
*/
queue_families::queue_families(const VkPhysicalDevice&            device,
                               const std::optional<ext::surface>& surface)
{
    auto device_families =
      collect<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, device);

    for (int i = 0; i < device_families.size(); ++i) {
        const auto& families = device_families[i];

        bool graphics_support = families.queueFlags & VK_QUEUE_GRAPHICS_BIT;
        // if there is a surface passed in for us to query
        if (surface.has_value()) {
            VkBool32 presentation_support = false;
            // TODO: Wrap vkGetPhysicalDeviceSurfaceSupportKHR
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface.value(), &presentation_support);
            if (families.queueCount > 0 && presentation_support) {
                presentation_family(i);
            }
        }

        if (families.queueCount > 0 && graphics_support) {
            graphics_family(i);
        }
        if (is_complete()) {
            raw_properties(families);
            break;
        }
    }
}

bool
queue_families::is_complete() const
{
    return m_graphics_family >= 0 && m_presentation_family >= 0;
}

}  // namespace shiny::vk
