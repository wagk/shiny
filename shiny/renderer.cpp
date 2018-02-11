#include <renderer.h>
#include <vk/queue.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <vector>

namespace {
#ifdef NDEBUG
const bool                     enable_validation_layer = false;
const std::vector<const char*> validation_layers       = {};
#else
const bool                     enable_validation_layer = true;
const std::vector<const char*> validation_layers       = { "VK_LAYER_LUNARG_standard_validation" };
#endif

}  // namespace

namespace shiny {

renderer&
renderer::singleton()
{
    static renderer single;
    return single;
}

renderer::renderer()
{
    init();
}

renderer::~renderer()
{
    m_logical_device.destroy();
    // physical devices have no destroy function for some reason
    m_instance.destroy();
}

// analogue for the initVulkan function in the tutorial
void
renderer::init()
{
    m_window.init();
    if (m_instance.create(&validation_layers) == false) {
        throw std::runtime_error("failed to create instance!");
    }

    m_instance.enable_debug_reporting();

    m_physical_device.select_physical_device(m_instance);
    m_logical_device.create(m_physical_device, &validation_layers);
    vk::queue m_queue = m_logical_device.get_queue();
}

void
renderer::draw()
{
    // stub function
    return;
}

}  // namespace shiny
