#pragma once

#include <vk/ext/surface.h>
#include <vk/instance.h>
#include <vk/logical_device.h>
#include <vk/physical_device.h>
#include <window.h>

#include <optional>

namespace shiny {

class renderer
{
public:
    static renderer& singleton();

    renderer();

    void draw();

    window& glfw_window() { return m_window; }

private:
    window m_window;

    // IMPORTANT! Arrange in *Reverse* the order of destruction!!
    std::optional<vk::instance>        m_instance;
    std::optional<vk::physical_device> m_physical_device;
    std::optional<vk::logical_device>  m_logical_device;
    std::optional<vk::queue>           m_queue;
    std::optional<vk::ext::surface>    m_surface;
};

}  // namespace shiny
