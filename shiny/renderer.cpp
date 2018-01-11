#include "renderer.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

namespace shiny {

  renderer::renderer()
    : m_bad_init(false)
  {
  }

  renderer::~renderer()
  {
  }

  void renderer::init()
  {
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