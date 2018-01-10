#pragma once

#include <vulkan/vulkan.h>

namespace vk {

  VkApplicationInfo default_appinfo();

  class instance
  {
  public:
    instance();
    instance(const instance&) = delete;
    ~instance();

    operator bool() const;
    operator VkInstance() const;

    bool create();
    void destroy();

  private:

    VkInstance m_instance;
    VkResult m_result;

    bool m_has_init;

  };
}