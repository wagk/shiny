#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

namespace shiny::graphics {

using spirvbytecode = std::vector<char>;

struct vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static vk::VertexInputBindingDescription                  getBindingDescription();
    static std::array<vk::VertexInputAttributeDescription, 2> getAttributeDescription();
};

/*
We can exactly match the definition in the shader using data types in GLM. The data in the matrices
is binary compatible with the way the shader expects it, so we can later just memcpy a
UniformBufferObject to a VkBuffer.
*/
struct uniformbufferobject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class renderer
{
public:
    explicit renderer() {}

    ~renderer() {}

    void run();
    void drawFrame();

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
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSemaphores();
    void createFences();

    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffer();

    void updateUniformBuffer();
    void createDescriptorPool();
    void createDescriptorSet();

    void createDescriptorSetLayout();

    // helper functions
    void createBuffer(vk::DeviceSize          size,
                      vk::BufferUsageFlags    usage,
                      vk::MemoryPropertyFlags properties,
                      vk::Buffer*             buffer,
                      vk::DeviceMemory*       buffermemory) const;
    void copyBuffer(vk::Buffer srcbuffer, vk::Buffer* dstbuffer, vk::DeviceSize size) const;

    void recreateSwapChain();
    void cleanupSwapChain();

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
    vk::SwapchainKHR             m_swapchain;
    std::vector<vk::Image>       m_swapchain_images;
    std::vector<vk::ImageView>   m_swapchain_image_views;
    vk::Format                   m_swapchain_image_format;
    vk::Extent2D                 m_swapchain_extent;
    std::vector<vk::Framebuffer> m_swapchain_framebuffers;

    vk::Buffer       m_vertex_buffer;
    vk::DeviceMemory m_vertex_buffer_memory;

    vk::Buffer       m_index_buffer;
    vk::DeviceMemory m_index_buffer_memory;

    vk::Buffer       m_uniform_buffer;
    vk::DeviceMemory m_uniform_buffer_memory;

    vk::ShaderModule m_vertex_shader_module;
    vk::ShaderModule m_fragment_shader_module;

    vk::DescriptorPool      m_descriptor_pool;
    vk::DescriptorSetLayout m_descriptor_set_layout;
    vk::DescriptorSet       m_descriptor_set;

    vk::RenderPass     m_render_pass;
    vk::PipelineLayout m_pipeline_layout;  // used to define shader uniform value layouts
    vk::Pipeline       m_graphics_pipeline;

    vk::CommandPool                m_command_pool;
    std::vector<vk::CommandBuffer> m_command_buffers;

    std::vector<vk::Semaphore> m_image_available_semaphores;
    std::vector<vk::Semaphore> m_render_finished_semaphores;

    static const uint32_t  max_frames_in_flight = 2;
    uint32_t               m_current_frame      = 0;
    std::vector<vk::Fence> m_in_flight_fences;

    vk::Queue m_graphics_queue;
    vk::Queue m_presentation_queue;
};

template<typename Func>
void
recordCommandBuffer(vk::CommandBuffer buffer, const vk::CommandBufferBeginInfo& info, Func action)
{
    buffer.begin(info);
    action();
    buffer.end();
}

template<typename Func>
void
recordCommandBufferRenderPass(vk::CommandBuffer              buffer,
                              const vk::RenderPassBeginInfo& renderPassBegin,
                              vk::SubpassContents            contents,
                              Func                           action)
{
    buffer.beginRenderPass(renderPassBegin, contents);
    action();
    buffer.endRenderPass();
}

}  // namespace shiny::graphics
