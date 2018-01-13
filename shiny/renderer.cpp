#include "renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <string>

namespace {
#ifdef NDEBUG
  const bool enable_validation_layer = false;
#else
  const bool enable_validation_layer = true;
#endif

  const std::vector<std::string> validation_layers = {
    "VK_LAYER_LUNARG_standard_validation"
  };
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
    if (m_instance.create(&validation_layers) == false) {
      throw std::runtime_error("failed to create instance!");
      m_bad_init = true;
    }
  }

  void renderer::draw()
  {
    return;
  }

}