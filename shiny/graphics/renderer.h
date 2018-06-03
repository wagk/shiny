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
    void createSurface();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();

    GLFWwindow* m_window = nullptr;

    // can't use UniqueDebugReportCallbackEXT because of
    // https://github.com/KhronosGroup/Vulkan-Hpp/issues/212

    vk::UniqueInstance         m_instance;
    vk::DebugReportCallbackEXT m_callback;
    vk::UniqueSurfaceKHR       m_surface;
    vk::PhysicalDevice         m_physical_device;
    vk::UniqueDevice           m_device;

    vk::UniqueSwapchainKHR m_swapchain;

    vk::Queue m_graphics_queue;
    vk::Queue m_presentation_queue;
};

}  // namespace shiny::graphics
