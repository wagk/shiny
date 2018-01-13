#include "instance.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#include <algorithm>
#include <iostream>

namespace {

  //checks if given layers are supported by the instance
  bool check_validation_layer_support(const std::vector<std::string>& layers)
  {
    uint32_t layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    std::vector<VkLayerProperties> available_layers(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());

    for (const auto& layer_name : layers) {
      bool available = false;
      for (const auto& elem : available_layers) {
        if (elem.layerName == layer_name) { available = true; }
      }
      if (available == false) {
        return false;
      }
    }
    
    return true;
  }
}

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

  bool instance::create(const std::vector<std::string>* enabled_layers)
  {
    if (enabled_layers && check_validation_layer_support(*enabled_layers) == false) {
      throw std::runtime_error("validation layers are requested but not available!");
    }

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

    //vulkan only sees a char array buffer
    std::vector<const char*> validation_strings;
    if (enabled_layers) {
      validation_strings.resize(enabled_layers->size());
      std::transform(enabled_layers->begin(), enabled_layers->end(), validation_strings.begin(),
        [](const std::string& s) {return s.c_str(); });
      create_info.enabledLayerCount = static_cast<uint32_t>(validation_strings.size());
      create_info.ppEnabledLayerNames = validation_strings.data();
    }
    else {
      create_info.enabledLayerCount = 0;
    }

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