#pragma once

#include <vk/instance.h>
#include <vk/physical_device.h>
#include <vk/logical_device.h>

struct GLFWwindow;

namespace shiny {

    class renderer
    {
    public:
        renderer();
        ~renderer();

        void init();
        void draw();

        // Getter functions
        vk::physical_device* get_physical_device() { return &m_physical_device; }
        VkQueue& get_graphics_queue() { return m_graphics_queue; }

    private:

        // used in place of throwing exceptions
        bool m_bad_init;

        vk::instance m_instance;
        vk::physical_device m_physical_device;
        vk::logical_device m_logical_device;

        VkQueue m_graphics_queue;
    };

}
