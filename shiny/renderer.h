#pragma once

#include <vk/instance.h>
#include <vk/logical_device.h>
#include <vk/physical_device.h>

struct GLFWwindow;

namespace shiny {

class renderer
{

public:
    ~renderer();

    void init();
    void draw();

private:
    vk::instance        m_instance;
    vk::physical_device m_physical_device;
    vk::logical_device  m_logical_device;
};

}  // namespace shiny
