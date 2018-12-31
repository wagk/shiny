#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <Windows.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include "graphics/texture.h"
#include "vulkanbase/camera.hpp"


namespace shiny {

using spirvbytecode = std::vector<char>;

/*
We can exactly match the definition in the shader using data types in GLM. The data in the matrices
is binary compatible with the way the shader expects it, so we can later just memcpy a
UniformBufferObject to a VkBuffer.
*/
#pragma region graphics_structs
struct uniformbufferobject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    // glm::vec4 lightPos     = glm::vec4(5.0f, 5.0f, 5.0f, 1.0f);
    /*glm::vec4 lightsPos[3] = { glm::vec4(7.0f, 7.0f, 7.0f, 1.0f),
                               glm::vec4(-9.0f, 5.0f, -9.0f, 1.0f),
                               glm::vec4(1.0f, -5.0f, 2.0f, 1.0f) };*/
};

// NOTE: These are borrowed from Sascha. I might need to rearrange to my own liking.
struct Light
{
    glm::vec4 position;
    glm::vec3 color;
    float     radius;
};

struct UniformBuffers
{
    vk::Buffer vsFullScreen;
    vk::Buffer vsOffScreen;
    vk::Buffer fsLights;
    // vk::Buffer               camera;
    vk::DeviceMemory vsFullScreen_mem;
    vk::DeviceMemory vsOffScreen_mem;
    vk::DeviceMemory fsLights_mem;
    // vk::DeviceMemory         camera_mem;
    vk::DescriptorBufferInfo vsFullscreen_des;
    vk::DescriptorBufferInfo vsOffscreen_des;
    vk::DescriptorBufferInfo fsLights_des;
    // vk::DescriptorBufferInfo camera_des;
    void* vsFullScreen_map;
    void* vsOffScreen_map;
    void* fsLights_map;
    // void*                    camera_map;
};

struct Vertex
{
    glm::vec3 pos;
    glm::vec2 texcoord;
    glm::vec3 color;
    glm::vec3 normal;
    glm::vec3 tangent;
};

struct Mesh
{
    Mesh() {}
    // uniformbufferobject      matrices;
    vk::Device*              device;
    uint32_t                 num_vertices;
    uint32_t                 num_indices;
    vk::Buffer               vertex_buffer;
    vk::Buffer               index_buffer;
    vk::Buffer               uniform_buffer;
    vk::DeviceMemory         vertex_buffer_memory;
    vk::DeviceMemory         index_buffer_memory;
    vk::DeviceMemory         uniform_buffer_memory;
    vk::DescriptorSet        descriptor_set;
    vk::DescriptorBufferInfo buffer_info;
    Texture2D                diffuse_tex;
    Texture2D                normal_tex;
    glm::vec3                rotation;
    std::string              texture_filename;
    void*                    mapped;

    void destroy(const vk::Device& device)
    {
        device.destroyBuffer(index_buffer);
        device.freeMemory(index_buffer_memory);
        device.destroyBuffer(vertex_buffer);
        device.freeMemory(vertex_buffer_memory);
        device.destroyBuffer(uniform_buffer);
        device.freeMemory(uniform_buffer_memory);
        // delete image and texture views and samplers
        diffuse_tex.destroy(device);
        normal_tex.destroy(device);
    }
};

// NOTE: WIP, this will eventually replace Mesh struct current usage
// Mesh struct will solely contain vertex and index data
// Models will encapsulate Textures and Meshes together.
struct Model
{
    vk::Device* device;
    Mesh        mesh;
    Texture2D   diffuse_tex;
    Texture2D   normal_tex;

    void destroy(const vk::Device& device)
    {
        mesh.destroy(device);
        diffuse_tex.destroy(device);
        normal_tex.destroy(device);
    }
};

struct FrameBufferAttachment
{
    vk::Image        image;
    vk::DeviceMemory memory;
    vk::ImageView    image_view;
    vk::Format       format;
};

struct FrameBuffer
{
    int32_t               width, height;
    vk::Framebuffer       frameBuffer;
    FrameBufferAttachment position, normal, albedo;
    FrameBufferAttachment depth;
    vk::RenderPass        renderPass;
};

struct SwapchainBuffer
{
    vk::Image     image;
    vk::ImageView view;
};

struct Swapchain
{
    vk::SwapchainKHR swapchain;

    vk::Format format;

    std::vector<vk::Image> images;
    // std::vector<vk::ImageView>   imageViews;
    std::vector<SwapchainBuffer> buffers;

    uint32_t width, height;
    uint32_t imageCount;
};

struct Pipelines
{
    vk::Pipeline deferred;
    vk::Pipeline offscreen;
    vk::Pipeline debug;
};

struct PipelineLayouts
{
    vk::PipelineLayout deferred;
    vk::PipelineLayout offscreen;
};

#pragma endregion graphics_structs

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
    void prepare();
    void mainLoop();
    void cleanup();

    // GLFW callbacks


    // Graphics functions
    void prepareFrame();
    void submitFrame();

    void createInstance();
    void setupDebugCallback();
    void createSurface();
    void generateQuads();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createSwapChain();
    void resizeSwapChain();
    void createImageViews();
    void createRenderPass();
    void createPipelineCache();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSemaphores();
    void createFences();

    vk::CommandBuffer createCommandBuffer(vk::CommandBufferLevel level, bool begin = false);
    void flushCommandBuffer(vk::CommandBuffer commandBuffer, vk::Queue queue, bool free);

    // Build Functionality (Called after creation down the line)
    void buildCommandBuffers();
    void buildDeferredCommandBuffer();

    // These are the functions called for each mesh
    void createVertexBuffer(Mesh& mesh, const std::vector<Vertex> vertices);
    void createIndexBuffer(Mesh& mesh, const std::vector<uint32_t> indices);
    void createUniformBuffer(Mesh& mesh, const vk::DeviceSize buffersize);

    // Prepare resources
    void prepareOffscreenFramebuffer();
    void prepareUniformBuffers();

    void updateUniformBuffer();  // let's hack this to update the camera matrices
    void updateUniformBuffersScreen();
    void updateUniformBufferDeferredMatrices();
    void updateUniformBufferDeferredLights();

    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSet();

    // Used for loading textures from files to Texture structs
    void createTextureImage(const std::string filename, Texture& texture);
    void createTextureImageView(Texture& texture);
    void createTextureSampler(Texture& texture);

    void createDepthResources();


    // Load functions
    // Implementing some specific ones for now
    void loadAssets();

    void loadModels(std::vector<std::string> filenames);
    Mesh loadObj(std::string objpath) const;
    Mesh loadFbx(std::string fbxpath);

    void loadTextures(std::vector<std::string> filenames);
    void loadTextures(Mesh& mesh, std::string diffuseFilename, std::string normalFilename);

    vk::PipelineShaderStageCreateInfo loadShader(const char*             entrypoint,
                                                 std::string             fileName,
                                                 vk::ShaderStageFlagBits stage);

    // helper functions
    std::pair<vk::Buffer, vk::DeviceMemory> createBuffer(vk::DeviceSize          size,
                                                         vk::BufferUsageFlags    usage,
                                                         vk::MemoryPropertyFlags properties) const;
    void copyBuffer(vk::Buffer srcbuffer, vk::Buffer* dstbuffer, vk::DeviceSize size) const;

    std::pair<vk::Image, vk::DeviceMemory> createImage(uint32_t                width,
                                                       uint32_t                height,
                                                       uint32_t                mipLevels,
                                                       vk::Format              format,
                                                       vk::ImageTiling         tiling,
                                                       vk::ImageUsageFlags     usage,
                                                       vk::MemoryPropertyFlags properties) const;

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;

    void transitionImageLayout(vk::Image       image,
                               vk::Format      format,
                               vk::ImageLayout oldLayout,
                               vk::ImageLayout newLayout,
                               uint32_t        mipLevels) const;

    vk::ImageView createImageView(vk::Image               image,
                                  vk::Format              format,
                                  vk::ImageAspectFlagBits aspectflags,
                                  uint32_t                mipLevels) const;

    void createAttachment(vk::Format             format,
                          vk::ImageUsageFlagBits usage,
                          FrameBufferAttachment* attachment);

    void generateMipmaps(vk::Image  image,
                         vk::Format imageFormat,
                         int32_t    tex_width,
                         int32_t    tex_height,
                         uint32_t   mipLevels);

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

    vk::Bool32 getSupportedDepthFormat(vk::Format* depthFormat);
    uint32_t   getMemoryType(uint32_t                typeBits,
                             vk::MemoryPropertyFlags properties,
                             vk::Bool32*             memTypeFound = nullptr);

    void findEnabledFeatures();

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

    std::vector<vk::VertexInputBindingDescription>   getBindingDescription();
    std::vector<vk::VertexInputAttributeDescription> getAttributeDescription();

    void recreateSwapChain();
    void cleanupSwapChain();

    // Choose Swapchain properties
    vk::Extent2D                  chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    vk::CompositeAlphaFlagBitsKHR chooseSwapAlpha(const vk::SurfaceCapabilitiesKHR& capabilities);

    // Window variables
    GLFWwindow* m_window = nullptr;

    int m_win_width  = 0;
    int m_win_height = 0;

    // can't use UniqueDebugReportCallbackEXT because of
    // https://github.com/KhronosGroup/Vulkan-Hpp/issues/212

    vk::Instance                       m_instance;
    vk::DebugReportCallbackEXT         m_callback;
    vk::SurfaceKHR                     m_surface;
    vk::PhysicalDevice                 m_physical_device;
    vk::PhysicalDeviceProperties       m_device_properties;
    vk::PhysicalDeviceMemoryProperties m_device_memory_properties;
    vk::PhysicalDeviceFeatures         m_device_features;
    vk::PhysicalDeviceFeatures         m_enabled_features;
    vk::Device                         m_device;

    // swapchain things
    // TODO: Find a way to turn this back into a UniqueSwapchainKHR
    // TODO: shift these into m_swapchain_struct;
    Swapchain m_swapchain_struct;

    std::vector<vk::Framebuffer> m_framebuffers;

    // vk::Buffer       m_uniform_buffer;
    // vk::DeviceMemory m_uniform_buffer_memory;

    // TODO: These are hardcoded variables that should exist in an asset cache.
    // Like m_shader_cache or something. Move these there.
    // vk::ShaderModule              m_vertex_shader_module;
    // vk::ShaderModule              m_fragment_shader_module;
    std::vector<vk::ShaderModule> m_shader_modules;

    vk::DescriptorPool      m_descriptor_pool;
    vk::DescriptorSetLayout m_descriptor_set_layout;
    // vk::DescriptorSet       m_descriptor_set;

    vk::RenderPass m_render_pass;

    // Pipeline variables
    Pipelines         m_graphics_pipelines;
    PipelineLayouts   m_pipeline_layouts;
    vk::PipelineCache m_pipeline_cache;
    // vk::PipelineLayout m_pipeline_layout;  // used to define shader uniform value layouts
    // vk::Pipeline       m_graphics_pipeline;

    vk::CommandPool                m_command_pool;
    std::vector<vk::CommandBuffer> m_command_buffers;  // Draw Command Buffers

    std::vector<vk::Semaphore> m_image_available_semaphores;
    std::vector<vk::Semaphore> m_render_finished_semaphores;

    // NOTE: Added these to isolate possible setup issues
    vk::Semaphore          m_present_complete;
    vk::Semaphore          m_render_complete;
    vk::Semaphore          m_offscreen_semaphore;
    vk::SubmitInfo         m_submit_info;
    vk::PipelineStageFlags m_submit_pipeline_stages =
      vk::PipelineStageFlagBits::eColorAttachmentOutput;

    static const uint32_t  max_frames_in_flight = 2;
    uint32_t               m_current_frame      = 0;
    std::vector<vk::Fence> m_wait_fences;

    vk::Queue m_graphics_queue;
    vk::Queue m_presentation_queue;

    // Validation Layer stuff

    // Temporary model loading stuff for testing. Eventually these will become a cache of meshes and
    // assets stored elsewhere.
    // Mesh m_mesh;

    // Asset Caches. TODO: use maps instead of a vector of meshes
    // uniformbufferobject m_camera;
    Camera            m_camera;
    UniformBuffers    m_uniform_buffers;
    FrameBuffer       m_offscreen_framebuffer;
    Mesh              m_offscreen_quads;
    vk::CommandBuffer m_offscreen_commandbuffer;
    std::vector<Mesh> m_meshes;
    vk::Sampler       m_color_sampler;

    // NOTE: These are temporarily here until I do it my way.
    struct
    {
        glm::mat4 projection;
        glm::mat4 model;
        glm::mat4 view;
        glm::vec4 instancePos[3];
    } uboVS, uboOffscreenVS;

    struct
    {
        Light     lights[6];
        glm::vec4 viewPos;
    } uboFragmentLights;
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

}  // namespace shiny
