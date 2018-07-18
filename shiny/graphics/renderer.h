#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vulkan/vulkan.hpp>

#include <glm/glm.hpp>

namespace shiny::graphics {

using spirvbytecode = std::vector<char>;

struct Vertex
{
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texcoord;

    static vk::VertexInputBindingDescription                  getBindingDescription();
    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescription();
};

struct Mesh
{
    Mesh() {}
    Mesh(std::vector<Vertex> verticesIn, std::vector<uint32_t> indicesIn)
    {
        vertices = verticesIn;
        indices  = indicesIn;
    }
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
    vk::Buffer            vertex_buffer;
    vk::DeviceMemory      vertex_buffer_memory;
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

    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();

    void createDepthResources();

    void createDescriptorSetLayout();

    // Load functions
    // Implementing some specific ones for now
    void loadModels();  // Might eventually want to change this to accept a vector of strings
                        // to load more than one file. This will be called from elsewhere.
    Mesh loadObj(std::string objpath) const;

    // helper functions
    std::pair<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize          size,
                                                         vk::BufferUsageFlags    usage,
                                                         vk::MemoryPropertyFlags properties) const;
    void copyBuffer(vk::Buffer srcbuffer, vk::Buffer* dstbuffer, vk::DeviceSize size) const;

    std::pair<vk::Image, vk::DeviceMemory> createImage(uint32_t                width,
                                                       uint32_t                height,
                                                       vk::Format              format,
                                                       vk::ImageTiling         tiling,
                                                       vk::ImageUsageFlags     usage,
                                                       vk::MemoryPropertyFlags properties) const;

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;

    void transitionImageLayout(vk::Image       image,
                               vk::Format      format,
                               vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout) const;

    vk::ImageView createImageView(vk::Image               image,
                                  vk::Format              format,
                                  vk::ImageAspectFlagBits aspectflags) const;

    /* Find Format Helper Functions: Put them here due to their need for device reference */
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates,
                                   vk::ImageTiling                tiling,
                                   vk::FormatFeatureFlagBits      features) const;
    vk::Format findDepthFormat() const
    {
        return findSupportedFormat(
          { vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
          vk::ImageTiling::eOptimal, vk::FormatFeatureFlagBits::eDepthStencilAttachment);
    }

    bool hasStencilComponent(vk::Format format) const
    {
        return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
    }

    /* Template function for use within unique SingleTimeCommand functions.
     * It acts as a wrapper for BeginSingleTimeCommand and EndSingleTimeCommand. */
    template<typename Func>
    void executeSingleTimeCommands(Func func) const;

    /*
    This function requires a closure with a signature of void(void*)
    */
    template<typename Func>
    void withMappedMemory(vk::DeviceMemory memory,
                          vk::DeviceSize   offset,
                          vk::DeviceSize   size,
                          Func             action)
    {
        void* data = m_device.mapMemory(memory, offset, size);
        action(data);
        m_device.unmapMemory(memory);
    }

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

    vk::Image        m_texture_image;
    vk::ImageView    m_texture_image_view;
    vk::DeviceMemory m_texture_image_memory;
    vk::Sampler      m_texture_sampler;

    vk::Image        m_depth_image;
    vk::ImageView    m_depth_image_view;
    vk::DeviceMemory m_depth_image_memory;

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

    // Temporary model loading stuff for testing. Eventually these will become a cache of meshes and
    // assets stored elsewhere.
    Mesh m_mesh;
};

/*
The signature for Func must satisfy void(vk::CommandBuffer)
*/
template<typename Func>
void
renderer::executeSingleTimeCommands(Func func) const
{
    auto allocinfo = vk::CommandBufferAllocateInfo()
                       .setLevel(vk::CommandBufferLevel::ePrimary)
                       .setCommandPool(m_command_pool)
                       .setCommandBufferCount(1);

    vk::CommandBuffer commandbuffer = m_device.allocateCommandBuffers(allocinfo).front();

    auto begininfo = vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

    commandbuffer.begin(begininfo);

    func(commandbuffer);

    commandbuffer.end();

    auto submitinfo = vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&commandbuffer);

    m_graphics_queue.submit(submitinfo, nullptr);
    // Unlike the draw commands, there are no events we need to wait on this time. We just want to
    // execute the transfer on the buffers immediately. There are again two possible ways to wait on
    // this transfer to complete. We could use a fence and wait with vkWaitForFences, or simply wait
    // for the transfer queue to become idle with vkQueueWaitIdle. A fence would allow you to
    // schedule multiple transfers simultaneously and wait for all of them complete, instead of
    // executing one at a time. That may give the driver more opportunities to optimize.
    m_graphics_queue.waitIdle();

    m_device.freeCommandBuffers(m_command_pool, 1, &commandbuffer);
}

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
