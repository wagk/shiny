#include "renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <string>

namespace {

  const std::vector<std::string> validation_layers = {
    "VK_LAYER_LUNARG_standard_validation"
  };

#ifdef NDEBUG
  const bool enable_validation_layer = false;
#else
  const bool enable_validation_layer = true;
#endif

  // TODO: this is incomplete
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

namespace shiny {

  renderer::renderer()
    : m_bad_init(false)
  {
  }

  renderer::~renderer()
  {
  }

  //analogue for the initVulkan function in the tutorial
  void renderer::init()
  {
    if (enable_validation_layer == true && 
      check_validation_layer_support(validation_layers) == false) {
      throw std::runtime_error("validation layers are requested but not available!");
    }
    if (m_instance.create() == false) {
      throw std::runtime_error("failed to create instance!");
      m_bad_init = true;
    }
  }

  void renderer::draw()
  {
    return;
  }

}