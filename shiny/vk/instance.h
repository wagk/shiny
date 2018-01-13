#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <string>

namespace shiny::graphic::vk {

  VkApplicationInfo default_appinfo();

  class instance
  {
  public:
    instance();
    instance(const instance&) = delete;
    ~instance();

    operator VkInstance() const;

    bool create(const std::vector<std::string>* enabled_layers = nullptr);
    void destroy();

    std::vector<std::string> extension_names() const;

  private:

    std::vector<VkExtensionProperties> extensions() const;

    VkInstance m_instance;
    VkResult m_result;

    bool m_has_init;

  };
}