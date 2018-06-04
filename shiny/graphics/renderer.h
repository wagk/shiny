#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#include <vulkan/vulkan.hpp>

namespace shiny::graphics {

class renderer
{
public:
    explicit renderer() {}

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
    void createImageViews();

    GLFWwindow* m_window = nullptr;

    // can't use UniqueDebugReportCallbackEXT because of
    // https://github.com/KhronosGroup/Vulkan-Hpp/issues/212

    vk::UniqueInstance         m_instance;
    vk::DebugReportCallbackEXT m_callback;
    vk::UniqueSurfaceKHR       m_surface;
    vk::PhysicalDevice         m_physical_device;
    vk::UniqueDevice           m_device;

    // swapchain things
    vk::SwapchainKHR                 m_swapchain;
    std::vector<vk::UniqueImage>     m_swapchain_images;
    std::vector<vk::UniqueImageView> m_swapchain_image_views;
    vk::Format                       m_swapchain_image_format;
    vk::Extent2D                     m_swapchain_extent;

    vk::Queue m_graphics_queue;
    vk::Queue m_presentation_queue;
};

}  // namespace shiny::graphics
