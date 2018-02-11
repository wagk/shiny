#pragma once

#include <vk/ext/surface.h>
#include <vk/instance.h>
#include <vk/logical_device.h>
#include <vk/physical_device.h>
#include <window.h>

namespace shiny {

class renderer
{
public:
    static renderer& singleton();

    renderer();
    ~renderer();

    void init();
    void draw();

    window& glfw_window() { return m_window; }

private:
    window m_window;

    vk::instance        m_instance;
    vk::physical_device m_physical_device;
    vk::logical_device  m_logical_device;
    vk::ext::surface    m_surface;
};

}  // namespace shiny
