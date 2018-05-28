#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#include <vulkan/vulkan.hpp>

namespace shiny::graphics {

class renderer
{
public:
    renderer() {}

    ~renderer() {}

    void run();

private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

    void createInstance();
    void setupDebugCallback();

    GLFWwindow* m_window = nullptr;

    vk::Instance               m_instance;
    vk::DebugReportCallbackEXT m_callback;
};

}  // namespace shiny::graphics
