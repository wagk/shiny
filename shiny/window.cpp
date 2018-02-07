#include "window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace shiny {

    window::window(int width, int height)
        : m_width(width),
        m_height(height),
        m_window(nullptr)
    {
    }

    window::~window()
    {
        glfwDestroyWindow(m_window);

        glfwTerminate();
    }

    void window::init()
    {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(m_width, m_height,
            "Vulkan", nullptr, nullptr);
    }

    void window::poll_events()
    {
        glfwPollEvents();
    }

    bool window::close_window()
    {
        return glfwWindowShouldClose(m_window);
    }

};