#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>

#include <vulkan/vulkan.hpp>

namespace shiny::graphics {

using SpirvBytecode = std::vector<char>;

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
    void createGraphicsPipeline();

    GLFWwindow* m_window = nullptr;

    // can't use UniqueDebugReportCallbackEXT because of
    // https://github.com/KhronosGroup/Vulkan-Hpp/issues/212

    vk::Instance               m_instance;
    vk::DebugReportCallbackEXT m_callback;
    vk::SurfaceKHR             m_surface;
    vk::PhysicalDevice         m_physical_device;
    vk::Device                 m_device;

    // swapchain things
    // TODO: Find a way to turn this back into a UniqueSwapchainKHR
    vk::SwapchainKHR           m_swapchain;
    std::vector<vk::Image>     m_swapchain_images;
    std::vector<vk::ImageView> m_swapchain_image_views;
    vk::Format                 m_swapchain_image_format;
    vk::Extent2D               m_swapchain_extent;

    vk::ShaderModule m_vertex_shader_module;
    vk::ShaderModule m_fragment_shader_module;

    // used to define shader uniform value layouts
    vk::PipelineLayout m_pipeline_layout;

    vk::Queue m_graphics_queue;
    vk::Queue m_presentation_queue;
};

}  // namespace shiny::graphics
