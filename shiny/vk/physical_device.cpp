#include <vk/physical_device.h>

#include <vk/utils.h>

namespace shiny::vk {

    queue_family_indices find_queue_families(VkPhysicalDevice device)
    {
        queue_family_indices indices;

        auto queue_families = collect<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, device);

        //we just want to find one card with VK_QUEUE_GRAPHICS_BIT
        int i = 0;
        for (const auto& queue_family : queue_families) {
            if (queue_family.queueCount > 0 &&
                queue_family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphics_family = i;
            }
            if (indices.is_complete())
                break;
            ++i;
        }

        return indices;
    }

    void physical_device::select_physical_device(const instance& inst)
    {
        auto devices = collect<VkPhysicalDevice>(vkEnumeratePhysicalDevices, inst);

        if (devices.empty()) {
            throw std::runtime_error("Failed to find a GPU with vulkan support!");
        }

        for (const auto& device : devices) {
            if (is_device_suitable(device)) {
                m_device = device;
                break;
            }
        }

        if (m_device == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU!");
        }
    }

    bool physical_device::is_device_suitable(VkPhysicalDevice device) const
    {
        queue_family_indices indices = find_queue_families(device);
        return indices.is_complete();
    }
}
