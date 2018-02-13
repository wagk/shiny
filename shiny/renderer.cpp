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
    m_window.init();

    m_instance.emplace(&validation_layers);
    if (m_instance.value() == false) {
        throw std::runtime_error("failed to create instance!");
    }

    // TODO: Improve this interface
    m_instance->enable_debug_reporting();

    m_physical_device.emplace(m_instance->select_physical_device());
    m_logical_device.emplace(m_physical_device->create_logical_device(&validation_layers));
    m_queue.emplace(m_logical_device->get_queue());

    // m_surface.create(m_instance, m_window);
}

void
renderer::draw()
{
    // stub function
    return;
}

}  // namespace shiny
