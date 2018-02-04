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

    private:

         struct queue_family_indices {
              int graphics_family = -1;

              bool is_complete() const {
                   return graphics_family >= 0;
              }
         };

         queue_family_indices find_queue_families(VkPhysicalDevice device) const;

         bool is_device_suitable(VkPhysicalDevice device) const;

         VkPhysicalDevice m_device = VK_NULL_HANDLE;
    };
}
