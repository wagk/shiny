#pragma once

/*
  TODO: enable interface to submit callbacks and register them as debug
  callbacks
*/

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace shiny::vk {

     // https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers
     VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
          VkDebugReportFlagsEXT      flags,
          VkDebugReportObjectTypeEXT obj_type,
          uint64_t                   obj,
          size_t                     location,
          int32_t                    code,
          const char*                layer_prefix,
          const char*                msg,
          void*                      user_data);

     VkApplicationInfo default_appinfo();

     class instance
     {
     public:
          instance();
          instance(const instance&) = delete;
          ~instance();

          operator VkInstance() const;

          // if there are validation layers (ie, not nullptr or empty vector), we assume that
          // debug callbacks are turned on, since that's what the tutorial does
          bool create(const std::vector<const char*>* enabled_layers = nullptr);
          void destroy();

          std::vector<const char*> extension_names() const;

          void enable_debug_reporting();
          void disable_debug_reporting();

     private:

          std::vector<VkExtensionProperties> extensions() const;

          VkInstance m_instance;
          VkResult   m_result;

          VkDebugReportCallbackEXT m_callback = nullptr;

          bool m_has_init;

     };
}
