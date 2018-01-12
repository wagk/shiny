#include "instance.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#include <algorithm>
#include <iostream>

namespace shiny::graphic::vk {
   
  VkApplicationInfo default_appinfo()
  {
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    return app_info;
  }

  instance::instance()
    : m_instance(VK_NULL_HANDLE)
    , m_result(VK_SUCCESS)
    , m_has_init(false)
  {
    
  }

  instance::~instance()
  {
    destroy();
  }

  instance::operator VkInstance() const
  {
    return m_instance;
  }

  bool instance::create()
  {
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = nullptr;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    //check which extensions does glfw need that isn't supported
    auto instance_ext_names = extension_names();
    for (size_t i = 0; i < glfwExtensionCount; i++) {
      bool exists = false;
      for (const auto& elem : instance_ext_names) {
        if (elem == glfwExtensions[i]) { exists = true; }
      }
      if (exists == false) {
        std::cerr << glfwExtensions[i] << " is not supported." << std::endl;
      }
    }

    VkApplicationInfo app_info = default_appinfo();
    
    VkInstanceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;
    create_info.enabledExtensionCount = glfwExtensionCount;
    create_info.ppEnabledExtensionNames = glfwExtensions;
    create_info.enabledLayerCount = 0;

    // https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCreateInstance.html
    m_result = vkCreateInstance(&create_info, nullptr, &m_instance);
    m_has_init = true;

    return bool(*this);
  }

  void instance::destroy()
  {
    if (m_instance != VK_NULL_HANDLE) {
      vkDestroyInstance(m_instance, nullptr);
      m_instance = VK_NULL_HANDLE;
      m_has_init = false;
    }
  }

  // does not need created instance to be called
  std::vector<VkExtensionProperties> instance::extensions() const
  {
    uint32_t ext_count;
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr);
    
    std::vector<VkExtensionProperties> extensions(ext_count);
    vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, extensions.data());

    return extensions;
  }

  std::vector<std::string> instance::extension_names() const
  {
    auto extensions = this->extensions();
    std::vector<std::string> names(extensions.size());

    std::transform(extensions.begin(), extensions.end(), names.begin(), 
      [](const VkExtensionProperties& p) {
      return p.extensionName;
    });

    return names;
  }

}