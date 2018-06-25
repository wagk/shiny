#pragma once

/*
  TODO: enable interface to submit callbacks and register them as debug
  callbacks
*/

#include <optional>
#include <string>
#include <vector>

#include <vulkan/vulkan.h>

#include <vk/ext/surface.h>
#include <vk/physical_device.h>
#include <window.h>

namespace shiny::vk {

// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugReportFlagsEXT      flags,
                                              VkDebugReportObjectTypeEXT obj_type,
                                              uint64_t                   obj,
                                              size_t                     location,
                                              int32_t                    code,
                                              const char*                layer_prefix,
                                              const char*                msg,
                                              void*                      user_data);

VkApplicationInfo default_appinfo();

/*
  Instances are the root interface to the vulkan application library.
*/
class instance
{
public:
    instance(const std::vector<const char*>* enabled_layers = nullptr);
    instance(const instance&) = delete;
    instance& operator=(const instance&) = delete;

    instance(instance&&);
    instance& operator=(instance&&);
    ~instance();

    operator VkInstance() const { return m_instance; }
    operator bool() const { return m_result == VK_SUCCESS; }

    std::vector<std::string> extension_names() const;

    void enable_debug_reporting();
    void disable_debug_reporting();

    physical_device select_physical_device(
      const std::optional<ext::surface>& surface = std::nullopt) const;
    ext::surface create_surface(window& context) const;

private:
    std::vector<VkExtensionProperties> extensions() const;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkResult   m_result;

    VkDebugReportCallbackEXT m_callback = nullptr;

    std::vector<const char*> m_enabled_layers = {};
};
}  // namespace shiny::vk

namespace shiny::graphics::vk {}
