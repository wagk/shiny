#pragma once

#include <vulkan\vulkan.h>
#include <vk/instance.h>

namespace shiny::vk {

    struct queue_family_indices {
        int graphics_family = -1;

        bool is_complete() const {
            return graphics_family >= 0;
        }

    };

    queue_family_indices find_queue_families(VkPhysicalDevice device);

    class physical_device
    {
    public:

        void select_physical_device(const instance& inst);
        VkPhysicalDevice& get_vk_physical_device() { return m_device; }

    private:
        bool is_device_suitable(VkPhysicalDevice device) const;

        VkPhysicalDevice m_device = VK_NULL_HANDLE;
    };
}
