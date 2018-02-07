#include "./renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <string>

namespace {
#ifdef NDEBUG
     const bool enable_validation_layer = false;
     const std::vector<const char*> validation_layers = {
     };
#else
     const bool enable_validation_layer = true;
     const std::vector<const char*> validation_layers = {
          "VK_LAYER_LUNARG_standard_validation"
     };
#endif

}

namespace shiny {

     renderer::renderer()
          : m_bad_init(false)
     {
     }

     renderer::~renderer()
     {
          m_instance.destroy();
     }

     //analogue for the initVulkan function in the tutorial
     void renderer::init()
     {
          if (m_instance.create(&validation_layers) == false) {
               m_bad_init = true; // why do this again?
               throw std::runtime_error("failed to create instance!");
          }

          m_instance.enable_debug_reporting();

        
     }

     void renderer::draw()
     {
          return;
     }

     renderer::queue_family_indices
     renderer::find_queue_families(VkPhysicalDevice device) const
     {
         queue_family_indices indices;

         uint32_t queue_family_count = 0;
         vkGetPhysicalDeviceQueueFamilyProperties(device, &queue_family_count, nullptr);

         return indices;
     }
}
