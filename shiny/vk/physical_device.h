#pragma once

#include "vulkan\vulkan.h"
#include "instance.h"

namespace shiny::vk {

    class physical_device
    {
    public:
        physical_device();
        ~physical_device();

        bool select_physical_device(const instance& inst);
        VkPhysicalDevice& get_vk_physical_device() { return m_device; }

    private:
        bool is_device_suitable(VkPhysicalDevice device) const;

        VkPhysicalDevice m_device = VK_NULL_HANDLE;
    };
}
