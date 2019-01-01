/*Brief overview of the graphics pipeline

The input assembler collects the raw vertex data from the buffers you specify and may also use an
index buffer to repeat certain elements without having to duplicate the vertex data itself.

The vertex shader is run for every vertex and generally applies transformations to turn vertex
positions from model space to screen space. It also passes per-vertex data down the pipeline.

The tessellation shaders allow you to subdivide geometry based on certain rules to increase the mesh
quality. This is often used to make surfaces like brick walls and staircases look less flat when
they are nearby.

The geometry shader is run on every primitive (triangle, line, point) and can discard it or output
more primitives than came in. This is similar to the tessellation shader, but much more flexible.
However, it is not used much in today's applications because the performance is not that good on
most graphics cards except for Intel's integrated GPUs.

The rasterization stage discretizes the primitives into fragments. These are the pixel elements that
they fill on the framebuffer. Any fragments that fall outside the screen are discarded and the
attributes outputted by the vertex shader are interpolated across the fragments, as shown in the
figure. Usually the fragments that are behind other primitive fragments are also discarded here
because of depth testing.

The fragment shader is invoked for every fragment that survives and determines which framebuffer(s)
the fragments are written to and with which color and depth values. It can do this using the
interpolated data from the vertex shader, which can include things like texture coordinates and
normals for lighting.

The color blending stage applies operations to mix different fragments that map to the same pixel in
the framebuffer. Fragments can simply overwrite each other, add up or be mixed based upon
transparency.

Stages with a green color are known as fixed-function stages. These stages allow you to tweak their
operations using parameters, but the way they work is predefined.

Stages with an orange color on the other hand are programmable, which means that you can upload your
own code to the graphics card to apply exactly the operations you want. This allows you to use
fragment shaders, for example, to implement anything from texturing and lighting to ray tracers.
These programs run on many GPU cores simultaneously to process many objects, like vertices and
fragments in parallel.

If you've used older APIs like OpenGL and Direct3D before, then you'll be used to being able to
change any pipeline settings at will with calls like glBlendFunc and OMSetBlendState. The graphics
pipeline in Vulkan is almost completely immutable, so you must recreate the pipeline from scratch if
you want to change shaders, bind different framebuffers or change the blend function. The
disadvantage is that you'll have to create a number of pipelines that represent all of the different
combinations of states you want to use in your rendering operations. However, because all of the
operations you'll be doing in the pipeline are known in advance, the driver can optimize for it much
better.

Some of the programmable stages are optional based on what you intend to do. For example, the
tessellation and geometry stages can be disabled if you are just drawing simple geometry. If you are
only interested in depth values then you can disable the fragment shader stage, which is useful for
shadow map generation.
*/
#pragma once

#include "graphics/renderer.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <vector>

#define TEX_DIM 2048
#define FB_DIM TEX_DIM

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// The header only defines the prototypes of the functions by default. One code file needs to
// include the header with the STB_IMAGE_IMPLEMENTATION definition to include the function bodies,
// otherwise we'll get linking errors.
#define STB_IMAGE_IMPLEMENTATION
#include <include/stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <include/tiny_obj_loader.h>

#include <assimp/Importer.hpp>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define UNREFERENCED_PARAMETER(P) (P)

namespace {
const bool debugDisplay = false;

const uint32_t WIDTH  = 1280;
const uint32_t HEIGHT = 800;

using VulkanExtensionName = const char*;
using VulkanLayerName     = const char*;

const std::vector<VulkanLayerName> validationLayers = { "VK_LAYER_LUNARG_standard_validation" };

const std::vector<VulkanExtensionName> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

const std::array<float, 4> backgroundColor = { 0.033f, 0.111f, 0.124f, 1.0f };

const std::vector<std::string> all_model_filenames = { "models/imrodnew.fbx" };
// const std::vector<std::string> all_model_filenames = { "models/singleCubeSelection.fbx" };
//"models/singleCubeSelection.fbx" };

const std::vector<std::string> all_texture_filenames = { "textures/Imrod_Diffuse.png",
                                                         "textures/texture.jpg" };

const char* texture_filename = "textures/Imrod_Diffuse.png";
const char* model_filename   = "models/imrod.fbx";

#ifdef NDEBUG
constexpr const bool enableValidationLayers = false;
#else
constexpr const bool enableValidationLayers = true;
#endif

static VKAPI_ATTR VkBool32 VKAPI_CALL
                           debugCallback(VkDebugReportFlagsEXT      flags,
                                         VkDebugReportObjectTypeEXT objType,
                                         uint64_t                   obj,
                                         size_t                     location,
                                         int32_t                    code,
                                         const char*                layerPrefix,
                                         const char*                msg,
                                         void*                      userData)
{
    UNREFERENCED_PARAMETER(flags);
    UNREFERENCED_PARAMETER(objType);
    UNREFERENCED_PARAMETER(obj);
    UNREFERENCED_PARAMETER(location);
    UNREFERENCED_PARAMETER(code);
    UNREFERENCED_PARAMETER(layerPrefix);
    UNREFERENCED_PARAMETER(userData);

    std::cerr << "validation layer: " << msg << std::endl;

    return VK_FALSE;
}

vk::Result
CreateDebugReportCallbackEXT(vk::Instance                                instance,
                             const vk::DebugReportCallbackCreateInfoEXT& pCreateInfo,
                             vk::DebugReportCallbackEXT&                 pCallback,
                             const vk::AllocationCallbacks*              pAllocator = nullptr)
{
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugReportCallbackEXT");

    if (func != nullptr) {
        VkDebugReportCallbackCreateInfoEXT createinfo = pCreateInfo;
        const VkAllocationCallbacks*       allocator  = (VkAllocationCallbacks*)pAllocator;
        VkDebugReportCallbackEXT           callback   = pCallback;
        VkResult result = func(instance, &createinfo, allocator, &callback);

        pCallback = callback;
        return static_cast<vk::Result>(result);
    } else {
        return vk::Result::eErrorExtensionNotPresent;
    }
}

void
DestroyDebugReportCallbackEXT(vk::Instance                   instance,
                              vk::DebugReportCallbackEXT&    rCallback,
                              const vk::AllocationCallbacks* pAllocator = nullptr)
{
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        VkDebugReportCallbackEXT callback = rCallback;
        func(instance, callback, (VkAllocationCallbacks*)pAllocator);
        rCallback = nullptr;
    }
}

class QueueFamilyIndices
{
    int m_graphicsFamily     = -1;
    int m_presentationFamily = -1;

public:
    bool isComplete() { return graphicsFamily() >= 0 && presentFamily() >= 0; }

    int  graphicsFamily() const { return m_graphicsFamily; }
    void graphicsFamily(int i) { m_graphicsFamily = i; }

    int  presentFamily() const { return m_presentationFamily; }
    void presentFamily(int i) { m_presentationFamily = i; }
};

/*
It has been briefly touched upon before that almost every operation in Vulkan, anything from drawing
to uploading textures, requires commands to be submitted to a queue. There are different types of
queues that originate from different queue families and each family of queues allows only a subset
of commands. For example, there could be a queue family that only allows processing of compute
commands or one that only allows memory transfer related commands.
*/
QueueFamilyIndices
findQueueFamilies(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface)
{
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> families = device.getQueueFamilyProperties();

    for (size_t i = 0; i < families.size(); i++) {
        auto       queuefamily          = families[i];
        vk::Bool32 presentation_support = device.getSurfaceSupportKHR((uint32_t)i, surface);

        if (queuefamily.queueCount > 0 && presentation_support) {
            indices.presentFamily((int)i);
        }

        if (queuefamily.queueCount > 0 && queuefamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily((int)i);
        }

        if (indices.isComplete()) {
            break;
        }
    }

    return indices;
}

struct SwapChainSupportDetails
{
    vk::SurfaceCapabilitiesKHR        capabilities;
    std::vector<vk::SurfaceFormatKHR> formats;
    std::vector<vk::PresentModeKHR>   presentModes;
};

/*
Just checking if a swap chain is available is not sufficient, because it may not actually be
compatible with our window surface. Creating a swap chain also involves a lot more settings than
instance and device creation, so we need to query for some more details before we're able to
proceed.

There are basically three kinds of properties we need to check:

    - Basic surface capabilities (min/max number of images in swap chain, min/max width and height
of images)
    - Surface formats (pixel format, color space)
    - Available presentation modes
*/
SwapChainSupportDetails
querySwapChainSupport(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface)
{
    SwapChainSupportDetails details;

    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats      = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);

    return details;
}

/*
Not all graphics cards are capable of presenting images directly to a screen for various reasons,
for example because they are designed for servers and don't have any display outputs. Secondly,
since image presentation is heavily tied into the window system and the surfaces associated with
windows, it is not actually part of the Vulkan core. You have to enable the VK_KHR_swapchain device
extension after querying for its support.
*/
bool
checkDeviceExtensionSupport(const vk::PhysicalDevice& device)
{
    std::set<std::string> requiredExtensions = { deviceExtensions.begin(), deviceExtensions.end() };

    for (const vk::ExtensionProperties& extension : device.enumerateDeviceExtensionProperties()) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

/*
We attempt to select for a device that supports all the features we need to draw something on the
screen
*/
bool
isDeviceSuitable(const vk::PhysicalDevice& device, const vk::SurfaceKHR& surface)
{
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    bool queueFamilyComplete = findQueueFamilies(device, surface).isComplete();
    bool swapChainAdequate   = false;

    if (extensionsSupported) {
        auto details      = querySwapChainSupport(device, surface);
        swapChainAdequate = !details.formats.empty() && !details.presentModes.empty();
    }

    /*The vkGetPhysicalDeviceFeatures repurposes the VkPhysicalDeviceFeatures struct to indicate
     * which features are supported rather than requested by setting the boolean values.*/
    vk::PhysicalDeviceFeatures supportedFeatures;
    device.getFeatures(&supportedFeatures);

    return extensionsSupported && queueFamilyComplete && swapChainAdequate
           && supportedFeatures.samplerAnisotropy;
}

/*
if the swapChainAdequate conditions were met then the support is definitely sufficient, but there
may still be many different modes of varying optimality. We'll now write a couple of functions to
find the right settings for the best possible swap chain. There are a few types of settings to
determine:

    - Surface format (color depth)
    - Presentation mode (conditions for "swapping" images to the screen)
    - Swap extent (resolution of images in swap chain)
        - Surface transform
        - Composite Alpha
*/
vk::SurfaceFormatKHR
chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableformats)
{
    if (availableformats.size() == 1 && availableformats[0].format == vk::Format::eUndefined) {
        return vk::SurfaceFormatKHR{ vk::Format::eB8G8R8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear };
    }

    for (const auto& availableformat : availableformats) {
        if (availableformat.format == vk::Format::eB8G8R8A8Unorm
            && availableformat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableformat;
        }
    }

    // TODO: This can probably be done better
    return availableformats.front();
}

/*
The presentation mode is arguably the most important setting for the swap chain, because it
represents the actual conditions for showing images to the screen. There are four possible modes
available in Vulkan:

    - VK_PRESENT_MODE_IMMEDIATE_KHR: Images submitted by your application are transferred to the
screen right away, which may result in tearing.

    - VK_PRESENT_MODE_FIFO_KHR: The swap chain is a queue where the display takes an image from the
front of the queue when the display is refreshed and the program inserts rendered images at the back
of the queue. If the queue is full then the program has to wait. This is most similar to vertical
sync as found in modern games. The moment that the display is refreshed is known as "vertical
blank".

    - VK_PRESENT_MODE_FIFO_RELAXED_KHR: This mode only differs from the previous one if the
application is late and the queue was empty at the last vertical blank. Instead of waiting for the
next vertical blank, the image is transferred right away when it finally arrives. This may result in
visible tearing.

    - VK_PRESENT_MODE_MAILBOX_KHR: This is another variation of the second mode. Instead of blocking
the application when the queue is full, the images that are already queued are simply replaced with
the newer ones. This mode can be used to implement triple buffering, which allows you to avoid
tearing with significantly less latency issues than standard vertical sync that uses double
buffering.

Only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available, so we'll again have to write a
function that looks for the best mode that is available:
*/
vk::PresentModeKHR
chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
{
    vk::PresentModeKHR bestmode = vk::PresentModeKHR::eFifo;

    for (const auto& availablepresentmode : availablePresentModes) {
        if (availablepresentmode == vk::PresentModeKHR::eMailbox) {
            return availablepresentmode;
        }
        if (availablepresentmode == vk::PresentModeKHR::eImmediate) {
            bestmode = availablepresentmode;
        }
    }

    return bestmode;
}

/*
The swap extent is the resolution of the swap chain images and it's almost always exactly equal to
the resolution of the window that we're drawing to. The range of the possible resolutions is defined
in the VkSurfaceCapabilitiesKHR structure. Vulkan tells us to match the resolution of the window by
setting the width and height in the currentExtent member. However, some window managers do allow us
to differ here and this is indicated by setting the width and height in currentExtent to a special
value: the maximum value of uint32_t. In that case we'll pick the resolution that best matches the
window within the minImageExtent and maxImageExtent bounds.
NOTE: I have commented out this function because I needed to make it a class function */
// vk::Extent2D
// chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
//{
//    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
//        return capabilities.currentExtent;
//    }
//
//    vk::Extent2D actual_extent(
//      std::clamp(WIDTH, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
//      std::clamp(HEIGHT, capabilities.minImageExtent.height, capabilities.maxImageExtent.height));
//
//    return actual_extent;
//}

bool
checkValidationLayerSupport()
{
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (const auto& layerProperties : availableLayers) {
            if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }

        if (!layerFound) {
            return false;
        }
    }

    return true;
}

/*
GLFW needs certain extensions to be present before it can work its windowing magic, and
these are known through the glfwGetRequiredInstanceExtensions function
*/
std::vector<VulkanExtensionName>
getRequiredExtensions()
{
    uint32_t             glfwExtensionCount = 0;
    VulkanExtensionName* glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<VulkanExtensionName> extensions(glfwExtensions,
                                                glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    return extensions;
}

/*
Creating a shader module is simple, we only need to specify a pointer to the buffer with the
bytecode and the length of it. This information is specified in a VkShaderModuleCreateInfo
structure. The one catch is that the size of the bytecode is specified in bytes, but the bytecode
pointer is a uint32_t pointer rather than a char pointer. Therefore we will need to cast the pointer
with reinterpret_cast as shown below. When you perform a cast like this, you also need to ensure
that the data satisfies the alignment requirements of uint32_t. Lucky for us, the data is stored in
an std::vector where the default allocator already ensures that the data satisfies the worst case
alignment requirements.
*/
vk::ShaderModule
createShaderModule(const shiny::spirvbytecode& code, const vk::Device& device)
{
    vk::ShaderModuleCreateInfo create_info;
    create_info.setCodeSize(code.size());
    create_info.setPCode((const uint32_t*)code.data());

    return device.createShaderModule(create_info);
}

/*
Now that we have a way of producing SPIR-V shaders, it's time to load them into our program to
plug them into the graphics pipeline at some point. We'll first write a simple helper function to
load the binary data from the files.
*/
shiny::spirvbytecode
readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary | std::ios::in | std::ios::ate);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for reading!");
    }

    size_t            filesize = (size_t)file.tellg();
    std::vector<char> buffer(filesize);

    file.seekg(0, std::ios::beg);
    file.read(buffer.data(), filesize);
    file.close();

    return shiny::spirvbytecode(buffer);
}

using MemoryTypeIndex = uint32_t;

/*
Graphics cards can offer different types of memory to allocate from. Each type of memory varies in
terms of allowed operations and performance characteristics. We need to combine the requirements of
the buffer and our own application requirements to find the right type of memory to use.
*/
MemoryTypeIndex
findMemoryType(const vk::PhysicalDevice& physical_device,
               // The typeFilter parameter will be used to specify the bit field of memory types
               // that are suitable.
               uint32_t                typefilter,
               vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memproperties = physical_device.getMemoryProperties();

    // The VkPhysicalDeviceMemoryProperties structure has two arrays memoryTypes and memoryHeaps.
    // Memory heaps are distinct memory resources like dedicated VRAM and swap space in RAM for when
    // VRAM runs out. The different types of memory exist within these heaps. Right now we'll only
    // concern ourselves with the type of memory and not the heap it comes from, but you can imagine
    // that this can affect performance.

    // However, we're not just interested in a memory type that is suitable for the vertex buffer.
    // We also need to be able to write our vertex data to that memory. The memoryTypes array
    // consists of VkMemoryType structs that specify the heap and properties of each type of memory.
    // The properties define special features of the memory, like being able to map it so we can
    // write to it from the CPU. This property is indicated with
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, but we also need to use the
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT property. We'll see why when we map the memory.

    for (uint32_t i = 0; i < memproperties.memoryTypeCount; ++i) {
        if ((typefilter & (1 << i)) && (memproperties.memoryTypes[i].propertyFlags & properties)) {
            return i;
        }
    }

    throw std::runtime_error("Unable to find a suitable Memory Type!");
}

}  // namespace

namespace shiny {

/*
A vertex binding describes at which rate to load data from memory throughout the vertices. It
specifies the number of bytes between data entries and whether to move to the next data entry after
each vertex or after each instance.
*/
std::vector<vk::VertexInputBindingDescription>
renderer::getBindingDescription()
{
    return {
        vk::VertexInputBindingDescription()
          //  The binding parameter specifies the index of the binding in the array of bindings.
          .setBinding(0)
          // The stride parameter specifies the number of bytes from one entry to the next
          .setStride(sizeof(Vertex))
          // the inputRate parameter can have one of the following values:
          // - VK_VERTEX_INPUT_RATE_VERTEX: Move to the next data entry after each vertex
          // - VK_VERTEX_INPUT_RATE_INSTANCE: Move to the next data entry after each instance
          // We're not going to use instanced rendering, so we'll stick to per-vertex data.
          .setInputRate(vk::VertexInputRate::eVertex)
    };
}

/*
As the function prototype indicates, there are going to be two of these structures. An attribute
description struct describes how to extract a vertex attribute from a chunk of vertex data
originating from a binding description. We have two attributes, position and color, so we need two
attribute description structs.
*/
std::vector<vk::VertexInputAttributeDescription>
renderer::getAttributeDescription()
{
    return {
        /*Position Description*/
        vk::VertexInputAttributeDescription()
          // The binding parameter tells Vulkan from which binding the per-vertex data comes
          .setBinding(0)
          // The location parameter references the location directive of the input in the
          // vertex shader. The input in the vertex shader with location 0 is the position,
          // which has two 32-bit float components.
          .setLocation(0)
          // The format parameter describes the type of data for the attribute. A bit
          // confusingly, the formats are specified using the same enumeration as color
          // formats. The following shader types and formats are commonly used together:
          //  - float: VK_FORMAT_R32_SFLOAT
          //  - vec2: VK_FORMAT_R32G32_SFLOAT
          //  - vec3: VK_FORMAT_R32G32B32_SFLOAT
          //  - vec4: VK_FORMAT_R32G32B32A32_SFLOAT
          // As you can see, you should use the format where the amount of color channels matches
          // the number of components in the shader data type. It is allowed to use more channels
          // than the number of components in the shader, but they will be silently discarded. If
          // the number of channels is lower than the number of components, then the BGA components
          // will use default values of (0, 0, 1). The color type (SFLOAT, UINT, SINT) and bit width
          // should also match the type of the shader input. See the following examples:
          //  - ivec2: VK_FORMAT_R32G32_SINT, a 2-component vector of 32-bit signed integers
          //  - uvec4: VK_FORMAT_R32G32B32A32_UINT, a 4-component vector of 32-bit unsigned
          //  - integers double: VK_FORMAT_R64_SFLOAT, a double-precision (64-bit) float
          // The format parameter implicitly defines the byte size of attribute data
          .setFormat(vk::Format::eR32G32B32Sfloat)
          // The offset parameter specifies the number of bytes since the start of the per-vertex
          // data to read from.
          .setOffset(0),
        //.setOffset(offsetof(Vertex, pos)),
        /*TexCoord Description*/
        vk::VertexInputAttributeDescription()
          .setBinding(0)
          .setLocation(1)
          .setFormat(vk::Format::eR32G32Sfloat)
          .setOffset(sizeof(float) * 3),
        //.setOffset(offsetof(Vertex, texcoord)),
        /*Color Description*/
        vk::VertexInputAttributeDescription()
          .setBinding(0)
          .setLocation(2)
          .setFormat(vk::Format::eR32G32B32Sfloat)
          .setOffset(sizeof(float) * 5),
        //.setOffset(offsetof(Vertex, color)),
        /*Normal Description*/
        vk::VertexInputAttributeDescription()
          .setBinding(0)
          .setLocation(3)
          .setFormat(vk::Format::eR32G32B32Sfloat)
          .setOffset(sizeof(float) * 8),
        //.setOffset(offsetof(Vertex, normal)),
        /*Tangent Description*/
        vk::VertexInputAttributeDescription()
          .setBinding(0)
          .setLocation(4)
          .setFormat(vk::Format::eR32G32B32Sfloat)
          .setOffset(sizeof(float) * 11)
        //.setOffset(offsetof(Vertex, tangent))
    };
}

}  // namespace shiny

namespace shiny {

void
renderer::run()
{
    initWindow();
    initVulkan();
    prepare();
    // @TODO: Everything below here should be in an update loop
    // which will be called by Game Class
    mainLoop();
    cleanup();
}

/*
The drawFrame function will perform the following operations:

 -  Acquire an image from the swap chain
 -  Execute the command buffer with that image as attachment in the framebuffer
 -  Return the image to the swap chain for presentation

Each of these events is set in motion using a single function call, but they are executed
asynchronously. The function calls will return before the operations are actually finished and the
order of execution is also undefined. That is unfortunate, because each of the operations depends on
the previous one finishing.

There are two ways of synchronizing swap chain events: fences and semaphores. They're both objects
that can be used for coordinating operations by having one operation signal and another operation
wait for a fence or semaphore to go from the unsignaled to signaled state.

The difference is that the state of fences can be accessed from your program using calls like
vkWaitForFences and semaphores cannot be. Fences are mainly designed to synchronize your application
itself with rendering operation, whereas semaphores are used to synchronize operations within or
across command queues. We want to synchronize the queue operations of draw commands and
presentation, which makes semaphores the best fit.
*/
void
renderer::drawFrame()
{
    // Wait for the old fences. When the frames in flight goes above 2, this might not work anymore
    // m_device.waitForFences(m_wait_fences[m_current_frame], true,
    // std::numeric_limits<uint64_t>::max());
    // m_device.resetFences(m_wait_fences[m_current_frame]);

    // Acquire image from swapchain.
    prepareFrame();

    // Queue submission and synchronization is configured through parameters in the VkSubmitInfo
    // structure.

    // assert(wait_semaphores.size() == wait_stages.size() && "wait_semaphores and wait_stages must
    // have same size!");

    // Wait for swap chain presentation to finish
    m_submit_info.setPWaitSemaphores(&m_present_complete);
    // Signal ready with offscreen semaphore
    m_submit_info.setPSignalSemaphores(&m_offscreen_semaphore);

    // Submit work
    // The next two parameters specify which command buffers to actually submit
    // for execution. As mentioned earlier, we should submit the command buffer
    // that binds the swap chain image we just acquired as color attachment.
    m_submit_info.setCommandBufferCount(1).setPCommandBuffers(&m_offscreen_commandbuffer);
    m_graphics_queue.submit(m_submit_info, nullptr);

    // Scene rendering...

    // Wait for offscreen semaphore
    m_submit_info.setPWaitSemaphores(&m_offscreen_semaphore);
    // Signal ready with render complete semaphore
    m_submit_info.setPSignalSemaphores(&m_render_complete);

    // Submit work
    m_submit_info.setPCommandBuffers(&m_command_buffers[m_current_frame]);
    m_graphics_queue.submit(1, &m_submit_info, nullptr);

    submitFrame();
    // m_current_frame = (m_current_frame + 1) % max_frames_in_flight;
}

void
renderer::initWindow()
{
    // Initialize glfw
    glfwInit();

    // Address Hard Hints for glfw settings.
    // NOTE: Hard hints are compulsory
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    // Get parameters for size
    GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();

    const GLFWvidmode* mode = glfwGetVideoMode(primary_monitor);
    // Set window size to be 3/4 of screen resolution
    int win_width  = (mode->width / 4) * 3;
    int win_height = (mode->height / 4) * 3;
    m_win_width    = win_width;
    m_win_height   = win_height;

    // Initilize our window
    // m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    m_window =
      glfwCreateWindow(win_width, win_height, "Shiny: A Vulkan Renderer", nullptr, nullptr);

    // Set window position to be center of screen
    // NOTE: origin of screenspace and windowspace are top-left corner. Y pointing down.
    glfwSetWindowPos(m_window, (mode->width / 2) - (win_width / 2),
                     (mode->height / 2) - (win_height / 2));
}

/*
There is no global state in Vulkan and all per-application state is stored in a VkInstance object.
Creating a VkInstance object initializes the Vulkan library and allows the application to pass
information about itself to the implementation.

https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#initialization-instances
 */
void
renderer::createInstance()
{
    if (enableValidationLayers && !checkValidationLayerSupport()) {
        throw std::runtime_error("Validation layers requested but unavailable!");
    }

    auto appinfo = vk::ApplicationInfo()
                     .setPApplicationName("Shiny Engine: A Vulkan renderer")
                     .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
                     .setPEngineName("No Engine")
                     .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
                     .setApiVersion(VK_API_VERSION_1_0);

    uint32_t             glfwExtensionCount = 0;
    VulkanExtensionName* glfwExtensions     = nullptr;

    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    auto createinfo = vk::InstanceCreateInfo()
                        .setPApplicationInfo(&appinfo)
                        .setEnabledExtensionCount(glfwExtensionCount)
                        .setPpEnabledExtensionNames(glfwExtensions);

    if (enableValidationLayers) {
        createinfo.setEnabledLayerCount((uint32_t)validationLayers.size());
        createinfo.setPpEnabledLayerNames(validationLayers.data());
    }

    auto extensions = getRequiredExtensions();
    createinfo.setEnabledExtensionCount((uint32_t)extensions.size());
    createinfo.setPpEnabledExtensionNames(extensions.data());

    m_instance = vk::createInstance(createinfo);

    // for debugging
    {
        std::cout << "Extension Properties:\n";
        std::vector<vk::ExtensionProperties> extensionProperties =
          vk::enumerateInstanceExtensionProperties();

        for (const auto& extension : extensionProperties) {
            std::cout << "\t" << extension.extensionName << std::endl;
        }
    }
}

void
renderer::setupDebugCallback()
{
    if (!enableValidationLayers)
        return;

    using DebugFlags = vk::DebugReportFlagBitsEXT;

    auto createinfo = vk::DebugReportCallbackCreateInfoEXT()
                        .setPfnCallback(debugCallback)
                        .setFlags(DebugFlags::eError | DebugFlags::eWarning | DebugFlags::eDebug
                                  | DebugFlags::eInformation | DebugFlags::ePerformanceWarning);

    if (CreateDebugReportCallbackEXT(m_instance, createinfo, m_callback) != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to set up debug callback!");
    }
}

/*
GLFW wraps around nearly all of surface creation for us, since it is a platform agnostic windowing
library

https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#boilerplate-wsi-header
*/
void
renderer::createSurface()
{
    VkSurfaceKHR surface;
    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
    m_surface = surface;
}

void
renderer::generateQuads()
{
    std::vector<Vertex> vertexBuffer;

    float x = 0.0f;
    float y = 0.0f;
    for (uint32_t i = 0; i < 3; i++) {
        // Last component of normal is used for debug display sampler index
        vertexBuffer.push_back({ { x + 1.0f, y + 1.0f, 0.0f },
                                 { 1.0f, 1.0f },
                                 { 1.0f, 1.0f, 1.0f },
                                 { 0.0f, 0.0f, (float)i } });
        vertexBuffer.push_back({ { x, y + 1.0f, 0.0f },
                                 { 0.0f, 1.0f },
                                 { 1.0f, 1.0f, 1.0f },
                                 { 0.0f, 0.0f, (float)i } });
        vertexBuffer.push_back({ { x + 0.0f, y, 0.0f },
                                 { 0.0f, 0.0f },
                                 { 1.0f, 1.0f, 1.0f },
                                 { 0.0f, 0.0f, (float)i } });
        vertexBuffer.push_back({ { x + 1.0f, y, 0.0f },
                                 { 1.0f, 0.0f },
                                 { 1.0f, 1.0f, 1.0f },
                                 { 0.0f, 0.0f, (float)i } });
        x += 1.0f;
        if (x > 1.0f) {
            x = 0.0f;
            y += 1.0f;
        }
    }

    createVertexBuffer(m_offscreen_quads, vertexBuffer);

    // Setup indices
    std::vector<uint32_t> indexBuffer = { 0, 1, 2, 2, 3, 0 };
    for (uint32_t i = 0; i < 3; ++i) {
        uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };
        for (auto index : indices) {
            indexBuffer.push_back(i * 4 + index);
        }
    }

    m_offscreen_quads.num_indices = static_cast<uint32_t>(indexBuffer.size());

    createIndexBuffer(m_offscreen_quads, indexBuffer);

    createUniformBuffer(m_offscreen_quads, sizeof(uint32_t) * indexBuffer.size());

    m_offscreen_quads.device = &m_device;
}

/*
A physical device usually represents a single complete implementation of Vulkan (excluding
instance-level functionality) available to the host, of which there are a finite number.

https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#devsandqueues-physical-device-enumeration

Physical devices don't have deleter functions, since they're not actually allocated out to the user.
*/
void
renderer::pickPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> physical_devices = m_instance.enumeratePhysicalDevices();

    if (physical_devices.empty())
        throw std::runtime_error("Failed to find a GPU with Vulkan support!");

    for (const auto& device : physical_devices) {
        if (isDeviceSuitable(device, m_surface)) {
            m_physical_device = device;
            m_physical_device.getProperties(&m_device_properties);
            m_physical_device.getFeatures(&m_device_features);
            m_physical_device.getMemoryProperties(&m_device_memory_properties);
            findEnabledFeatures();
            break;
        }
    }

    if (!m_physical_device)
        throw std::runtime_error("Failed to find a suitable GPU!");
}

/*
Device objects represent logical connections to physical devices. Each device exposes a number of
queue families each having one or more queues. All queues in a queue family support the same
operations.

https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#devsandqueues-devices

https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html#devsandqueues-queues
*/
void
renderer::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(m_physical_device, m_surface);

    float queuepriority = 1.f;

    std::vector<vk::DeviceQueueCreateInfo> queuecreateinfos;
    std::set<int> uniquefamilies = { indices.graphicsFamily(), indices.presentFamily() };

    for (int queuefamily : uniquefamilies) {
        queuecreateinfos.emplace_back(vk::DeviceQueueCreateInfo()
                                        .setQueueFamilyIndex(queuefamily)
                                        .setQueueCount(1)
                                        .setPQueuePriorities(&queuepriority));
    }

    auto devicefeatures = vk::PhysicalDeviceFeatures().setSamplerAnisotropy(true);

    auto createinfo = vk::DeviceCreateInfo()
                        .setQueueCreateInfoCount((uint32_t)queuecreateinfos.size())
                        .setPQueueCreateInfos(queuecreateinfos.data())
                        .setPEnabledFeatures(&devicefeatures)
                        .setEnabledExtensionCount((uint32_t)deviceExtensions.size())
                        .setPpEnabledExtensionNames(deviceExtensions.data());

    if (enableValidationLayers) {
        createinfo.setEnabledLayerCount((uint32_t)validationLayers.size());
        createinfo.setPpEnabledLayerNames(validationLayers.data());
    }

    m_device = m_physical_device.createDevice(createinfo);

    m_graphics_queue     = m_device.getQueue(indices.graphicsFamily(), 0);
    m_presentation_queue = m_device.getQueue(indices.presentFamily(), 0);
}

/*
A swapchain is an abstraction for an array of presentable images that are associated with a surface.
The presentable images are represented by VkImage objects created by the platform. One image (which
can be an array image for multiview/stereoscopic-3D surfaces) is displayed at a time, but multiple
images can be queued for presentation. An application renders to the image, and then queues the
image for presentation to the surface.

https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html#_wsi_swapchain
*/
void
renderer::createSwapChain()
{
    auto oldSwapchain = m_swapchain_struct.swapchain;

    SwapChainSupportDetails support = querySwapChainSupport(m_physical_device, m_surface);

    vk::SurfaceFormatKHR surfaceformat = chooseSwapSurfaceFormat(support.formats);
    vk::PresentModeKHR   presentmode   = chooseSwapPresentMode(support.presentModes);

    uint32_t presentModeCount;
    m_physical_device.getSurfacePresentModesKHR(m_surface, &presentModeCount, NULL);
    std::vector<vk::PresentModeKHR> presentModes(presentModeCount);
    m_physical_device.getSurfacePresentModesKHR(m_surface, &presentModeCount, presentModes.data());

    // vk::Extent2D                  extent         = chooseSwapExtent(support.capabilities);

    // Setup extent info
    vk::Extent2D swapchainExtent = {};
    if (support.capabilities.currentExtent.width == (uint32_t)-1) {
        swapchainExtent.width  = m_win_width;
        swapchainExtent.height = m_win_height;
    } else {
        swapchainExtent = support.capabilities.currentExtent;
        m_win_width     = support.capabilities.currentExtent.width;
        m_win_height    = support.capabilities.currentExtent.height;
    }

    // Determine the number of images
    uint32_t imagecount = support.capabilities.minImageCount + 1;
    if ((support.capabilities.maxImageCount > 0)
        && (imagecount > support.capabilities.maxImageCount)) {
        imagecount = support.capabilities.maxImageCount;
    }

    // Find the transformation of the surface
    vk::SurfaceTransformFlagBitsKHR preTransform;
    if (support.capabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
        // We prefer a non-rotated transform
        preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    } else {
        preTransform = support.capabilities.currentTransform;
    }

    // Find a supported composite alpha format
    vk::CompositeAlphaFlagBitsKHR compositeAlpha = chooseSwapAlpha(support.capabilities);

    // Now we tie it all together in a CreateInfo
    auto createinfo = vk::SwapchainCreateInfoKHR()
                        .setPNext(NULL)
                        .setSurface(m_surface)
                        .setMinImageCount(imagecount)
                        .setImageFormat(surfaceformat.format)
                        .setImageColorSpace(surfaceformat.colorSpace)
                        .setImageExtent(swapchainExtent)
                        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
                        .setPreTransform(preTransform)
                        .setImageArrayLayers(1)
                        .setImageSharingMode(vk::SharingMode::eExclusive)  // For now
                        .setQueueFamilyIndexCount(0)
                        .setPQueueFamilyIndices(NULL)
                        .setPresentMode(presentmode)
                        .setOldSwapchain(oldSwapchain)
                        .setClipped(true)
                        .setCompositeAlpha(compositeAlpha);

    /* NOTE: removing this for now until I understand image sharing modes better
QueueFamilyIndices indices = findQueueFamilies(m_physical_device, m_surface);

if (indices.graphicsFamily() != indices.presentFamily()) {
    std::array<uint32_t, 2> queuefamilyindices = { (uint32_t)indices.graphicsFamily(),
                                                   (uint32_t)indices.presentFamily() };
    createinfo.setImageSharingMode(vk::SharingMode::eConcurrent)
      .setQueueFamilyIndexCount(2)
      .setPQueueFamilyIndices(queuefamilyindices.data());
} else {
    createinfo.setImageSharingMode(vk::SharingMode::eExclusive);
}*/

    // Enable transfer source on swap chain images if supported
    if (support.capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferSrc) {
        createinfo.imageUsage |= vk::ImageUsageFlagBits::eTransferSrc;
    }

    // Enable transfer destination on swap chain images if supported
    if (support.capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst) {
        createinfo.imageUsage |= vk::ImageUsageFlagBits::eTransferDst;
    }
    // Create Swap Chain
    m_swapchain_struct.swapchain = m_device.createSwapchainKHR(createinfo);

    // If an existing swap chain is re-created, destroy the old swap chain
    // This also cleans up all the presentable images
    if (oldSwapchain) {
        for (uint32_t i = 0; i < imagecount; ++i) {
            m_device.destroyImageView(m_swapchain_struct.buffers[i].view);
        }
        m_device.destroySwapchainKHR(oldSwapchain);
    }

    // Get the swap chain images
    auto images = m_device.getSwapchainImagesKHR(m_swapchain_struct.swapchain);

    m_swapchain_struct.imageCount = static_cast<uint32_t>(images.size());
    m_swapchain_struct.images.reserve(images.size());
    for (const auto& image : images) {
        m_swapchain_struct.images.emplace_back(image);
    }

    // Get the swap chain buffers containing the image and imageview
    m_swapchain_struct.buffers.resize(images.size());
    for (uint32_t i = 0; i < images.size(); ++i) {
        auto ssr = vk::ImageSubresourceRange()
                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                     .setBaseMipLevel(0)
                     .setLevelCount(1)
                     .setBaseArrayLayer(0)
                     .setLayerCount(1);

        auto colorAttachmentView =
          vk::ImageViewCreateInfo()
            .setPNext(NULL)
            .setFormat(surfaceformat.format)
            .setComponents({ vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG,
                             vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA })
            .setSubresourceRange(ssr)
            .setViewType(vk::ImageViewType::e2D)
            .setFlags(vk::ImageViewCreateFlagBits(0));
        m_swapchain_struct.buffers[i].image = images[i];
        colorAttachmentView.setImage(m_swapchain_struct.buffers[i].image);

        m_swapchain_struct.buffers[i].view = m_device.createImageView(colorAttachmentView);
    }

    m_swapchain_struct.format = surfaceformat.format;
    m_swapchain_struct.height = m_win_height;
    m_swapchain_struct.width  = m_win_width;
}

void
renderer::resizeSwapChain()
{
    vk::SurfaceCapabilitiesKHR capabilities =
      m_physical_device.getSurfaceCapabilitiesKHR(m_surface);

    uint32_t newWidth  = capabilities.currentExtent.width;
    uint32_t newHeight = capabilities.currentExtent.height;

    // Check base case: swapchain is still the same size.
    if (m_swapchain_struct.height == newHeight && m_swapchain_struct.width == newWidth) {
        return;
    }

    Swapchain old = m_swapchain_struct;

    // create Swapchain
}

/*
Images represent multidimensional - up to 3 - arrays of data which can be used for various purposes
(e.g. attachments, textures), by binding them to a graphics or compute pipeline via descriptor sets,
or by directly specifying them as parameters to certain commands.

To use any VkImage, including those in the swap chain, in the render pipeline we have to create a
VkImageView object. An image view is quite literally a view into an image. It describes how to
access the image and which part of the image to access, for example if it should be treated as a 2D
texture depth texture without any mipmapping levels.

https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#resources-images
*/
void
renderer::createImageViews()
{
    // m_swapchain_struct.imageViews.reserve(m_swapchain_struct.images.size());

    for (const vk::Image& image : m_swapchain_struct.images) {
        // m_swapchain_struct.buffers.views.emplace_back(
        // createImageView(image, m_swapchain_struct.format, vk::ImageAspectFlagBits::eColor));
    }
}

/*
Before we can finish creating the pipeline, we need to tell Vulkan about the framebuffer attachments
that will be used while rendering. We need to specify how many color and depth buffers there will
be, how many samples to use for each of them and how their contents should be handled throughout the
rendering operations. All of this information is wrapped in a render pass object.
*/
void
renderer::createRenderPass()
{
    auto colorattachment =
      vk::AttachmentDescription()
        // The format of the color attachment should match the format of the swap chain images.
        .setFormat(m_swapchain_struct.format)
        // we're not doing anything with multisampling
        .setSamples(vk::SampleCountFlagBits::e1)
        // The loadOp determines what to do with the data in the attachment before rendering
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        // The storeOp determines what to do with the data in the attachment after
        // rendering.
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        // We don't care what the initial memory layout of the VkImage is
        .setInitialLayout(vk::ImageLayout::eUndefined)
        // We wnat the final layout of the VkImage to be something that can be presented in the swap
        // chain
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);

    // A single render pass can consist of multiple subpasses. Subpasses are subsequent rendering
    // operations that depend on the contents of framebuffers in previous passes, for example a
    // sequence of post-processing effects that are applied one after another. If you group these
    // rendering operations into one render pass, then Vulkan is able to reorder the operations and
    // conserve memory bandwidth for possibly better performance. For our very first triangle,
    // however, we'll stick to a single subpass.

    // Every subpass references one or more of the attachments that we've described using the
    // structure in the previous sections.
    auto colorattachmentref = vk::AttachmentReference()
                                // The attachment parameter specifies which attachment to reference
                                // by its index in the attachment descriptions array. Our array
                                // consists of a single VkAttachmentDescription, so its index is 0
                                .setAttachment(0)
                                // The layout specifies which layout we would like the attachment to
                                // have during a subpass that uses this reference.
                                .setLayout(vk::ImageLayout::eColorAttachmentOptimal);

    /*The format should be the same as the depth image itself. This time we don't care about storing
     * the depth data (storeOp), because it will not be used after drawing has finished. This may
     * allow the hardware to perform additional optimizations. Just like the color buffer, we don't
     * care about the previous depth contents, so we can use VK_IMAGE_LAYOUT_UNDEFINED as
     * initialLayout.*/
    auto depthattachment = vk::AttachmentDescription()
                             .setFormat(findDepthFormat())
                             .setSamples(vk::SampleCountFlagBits::e1)
                             .setLoadOp(vk::AttachmentLoadOp::eClear)
                             .setStoreOp(vk::AttachmentStoreOp::eStore)
                             .setStencilLoadOp(vk::AttachmentLoadOp::eClear)
                             .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                             .setInitialLayout(vk::ImageLayout::eUndefined)
                             .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    auto depthattachmentref = vk::AttachmentReference().setAttachment(1).setLayout(
      vk::ImageLayout::eDepthStencilAttachmentOptimal);

    auto subpass = vk::SubpassDescription()
                     // Vulkan may also support compute subpasses in the future, so we have to be
                     // explicit about this being a graphics subpass.
                     .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                     // The index of the attachment in this array is directly referenced from the
                     // fragment shader with the layout(location = 0) out vec4 outColor directive!
                     .setColorAttachmentCount(1)
                     // vk::AttachmentReference, not the vk::AttachmentDescription itself
                     // The following other types of attachments can be referenced by a subpass:
                     // pInputAttachments: Attachments that are read from a shader
                     // pResolveAttachments: Attachments used for multisampling color attachments
                     // pDepthStencilAttachment: Attachments for depth and stencil data
                     // pPreserveAttachments: Attachments that are not used by this subpass, but for
                     // which the data must be preserved
                     .setPColorAttachments(&colorattachmentref)
                     .setPDepthStencilAttachment(&depthattachmentref)
                     .setInputAttachmentCount(0)
                     .setPInputAttachments(nullptr)
                     .setPreserveAttachmentCount(0)
                     .setPPreserveAttachments(nullptr)
                     .setPResolveAttachments(nullptr);

    // Remember that the subpasses in a render pass automatically take care of image layout
    // transitions. These transitions are controlled by subpass dependencies, which specify memory
    // and execution dependencies between subpasses. We have only a single subpass right now, but
    // the operations right before and right after this subpass also count as implicit "subpasses".

    // There are two built-in dependencies that take care of the transition at the start of the
    // render pass and at the end of the render pass, but the former does not occur at the right
    // time. It assumes that the transition occurs at the start of the pipeline, but we haven't
    // acquired the image yet at that point! There are two ways to deal with this problem. We could
    // change the waitStages for the imageAvailableSemaphore to VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT to
    // ensure that the render passes don't begin until the image is available, or we can make the
    // render pass wait for the VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT stage. I've decided to
    // go with the second option here, because it's a good excuse to have a look at subpass
    // dependencies and how they work.

    std::array<vk::SubpassDependency, 2> subpassdependencies;

    subpassdependencies[0]
      // The special value VK_SUBPASS_EXTERNAL refers to the implicit subpass before or after the
      // render pass depending on whether it is specified in srcSubpass or dstSubpass
      // https://stackoverflow.com/questions/45724495/no-definition-for-vk-subpass-external-in-vulkan-hpp
      .setSrcSubpass(VK_SUBPASS_EXTERNAL)
      // The index 0 refers to our subpass, which is the first and only one. The dstSubpass must
      // always be higher than srcSubpass to prevent cycles in the dependency graph. (Not sure
      // about this because it looks like VK_SUBPASS_EXTERNAL is uint and greater
      .setDstSubpass(0)
      // The next two fields specify the operations to wait on and the stages in which these
      // operations occur. We need to wait for the swap chain to finish reading from the image
      // before we can access it. This can be accomplished by waiting on the color attachment
      // output stage itself.
      .setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
      .setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
      // The operations that should wait on this are in the color attachment stage and involve the
      // reading and writing of the color attachment. These settings will prevent the transition
      // from happening until it's actually necessary (and allowed): when we want to start writing
      // colors to it.
      .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentRead
                        | vk::AccessFlagBits::eColorAttachmentWrite)
      .setDependencyFlags(vk::DependencyFlagBits::eByRegion);

    subpassdependencies[1]
      .setSrcSubpass(0)
      .setDstSubpass(VK_SUBPASS_EXTERNAL)
      .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentRead
                        | vk::AccessFlagBits::eColorAttachmentWrite)
      .setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
      .setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
      .setDependencyFlags(vk::DependencyFlagBits::eByRegion);

    std::array<vk::AttachmentDescription, 2> attachments = { colorattachment, depthattachment };

    auto renderpasscreateinfo =
      vk::RenderPassCreateInfo()
        .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
        .setPAttachments(attachments.data())
        .setSubpassCount(1)
        .setPSubpasses(&subpass)
        .setDependencyCount(static_cast<uint32_t>(subpassdependencies.size()))
        .setPDependencies(subpassdependencies.data());

    if (!(m_render_pass = m_device.createRenderPass(renderpasscreateinfo))) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void
renderer::createPipelineCache()
{
    auto pipelineCacheCreateInfo = vk::PipelineCacheCreateInfo();
    if (!(m_pipeline_cache = m_device.createPipelineCache(pipelineCacheCreateInfo))) {
        std::runtime_error("Could not create Pipeline Cache!");
    }
}

/*
Graphics pipelines consist of multiple shader stages, multiple fixed-function pipeline stages, and a
pipeline layout.

Each pipeline is controlled by a monolithic object created from a description of all of the shader
stages and any relevant fixed-function stages. Linking the whole pipeline together allows the
optimization of shaders based on their input/outputs and eliminates expensive draw time state
validation.

A pipeline object is bound to the device state in command buffers. Any pipeline object state that is
marked as dynamic is not applied to the device state when the pipeline is bound. Dynamic state not
set by binding the pipeline object can be modified at any time and persists for the lifetime of the
command buffer, or until modified by another dynamic state command or another pipeline bind. No
state, including dynamic state, is inherited from one command buffer to another. Only dynamic state
that is required for the operations performed in the command buffer needs to be set. For example, if
blending is disabled by the pipeline state then the dynamic color blend constants do not need to be
specified in the command buffer, even if this state is marked as dynamic in the pipeline state
object. If a new pipeline object is bound with state not marked as dynamic after a previous pipeline
object with that same state as dynamic, the new pipeline object state will override the dynamic
state. Modifying dynamic state that is not set as dynamic by the pipeline state object will lead to
undefined results.

https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html#pipelines-graphics

https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Fixed_functions
*/
void
renderer::createGraphicsPipeline()
{
    // The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry
    // will be drawn from the vertices and if primitive restart should be enabled.
    auto inputassembly = vk::PipelineInputAssemblyStateCreateInfo()
                           .setTopology(vk::PrimitiveTopology::eTriangleList)
                           .setPrimitiveRestartEnable(false);

    // rasterisation
    // The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and
    // turns it into fragments to be colored by the fragment shader. It also performs depth testing,
    // face culling and the scissor test, and it can be configured to output fragments that fill
    // entire polygons or just the edges (wireframe rendering). All this is configured using the
    // VkPipelineRasterizationStateCreateInfo structure.
    auto rasterizer =
      vk::PipelineRasterizationStateCreateInfo()
        // If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far
        // planes are clamped to them as opposed to discarding them. This is useful in some special
        // cases like shadow maps. Using this requires enabling a GPU feature.
        .setDepthClampEnable(false)
        // If rasterizerDiscardEnable is set to VK_TRUE, then geometry never passes through the
        // rasterizer stage. This basically disables any output to the framebuffer.
        .setRasterizerDiscardEnable(false)
        // The polygonMode determines how fragments are generated for geometry. The following modes
        // are available:
        //    VK_POLYGON_MODE_FILL: fill the area of the polygon with fragments
        //    VK_POLYGON_MODE_LINE: polygon edges are drawn as lines
        //    VK_POLYGON_MODE_POINT: polygon vertices are drawn as points
        // Using any mode other than fill requires enabling a GPU feature.
        .setPolygonMode(vk::PolygonMode::eFill)
        // The lineWidth member is straightforward, it describes the thickness of lines in terms of
        // number of fragments. The maximum line width that is supported depends on the hardware and
        // any line thicker than 1.0f requires you to enable the wideLines GPU feature.
        .setLineWidth(1.f)
        // The cullMode variable determines the type of face culling to use. You can disable
        // culling, cull the front faces, cull the back faces or both.
        .setCullMode(vk::CullModeFlagBits::eBack)
        // The frontFace variable specifies the vertex order for faces to be considered front-facing
        // and can be clockwise or counterclockwise.
        .setFrontFace(vk::FrontFace::eClockwise)
        // The rasterizer can alter the depth values by adding a constant value or biasing them
        // based on a fragment's slope. This is sometimes used for shadow mapping, but we won't be
        // using it.
        .setDepthBiasEnable(false);

    // Contains configuration per attached framebuffer
    auto colorblendattachment =
      vk::PipelineColorBlendAttachmentState()
        .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                           | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
        .setBlendEnable(false);

    // VkPipelineColorBlendStateCreateInfo references the array of structures for all of the
    // framebuffers and allows you to set blend constants that you can use as blend factors in the
    // aforementioned calculations.
    // If you want to use the second method of blending (bitwise combination), then you should set
    // logicOpEnable to VK_TRUE. The bitwise operation can then be specified in the logicOp field.
    // Note that this will automatically disable the first method, as if you had set blendEnable to
    // VK_FALSE for every attached framebuffer! The colorWriteMask will also be used in this mode to
    // determine which channels in the framebuffer will actually be affected. It is also possible to
    // disable both modes, as we've done here, in which case the fragment colors will be written to
    // the framebuffer unmodified.
    auto colorblending = vk::PipelineColorBlendStateCreateInfo()
                           //.setLogicOpEnable(false)
                           .setAttachmentCount(1)
                           .setPAttachments(&colorblendattachment);

    /*The depth attachment is ready to be used now, but depth testing still needs to be enabled in
     * the graphics pipeline. It is configured through the VkPipelineDepthStencilStateCreateInfo
     * struct:
     * The depthTestEnable field specifies if the depth of new fragments should be compared to the
     * depth buffer to see if they should be discarded. The depthWriteEnable field specifies if the
     * new depth of fragments that pass the depth test should actually be written to the depth
     * buffer. This is useful for drawing transparent objects. They should be compared to the
     * previously rendered opaque objects, but not cause further away transparent objects to not be
     * drawn.*/
    vk::PipelineDepthStencilStateCreateInfo depthstencil =
      vk::PipelineDepthStencilStateCreateInfo()
        .setDepthTestEnable(true)
        .setDepthWriteEnable(true)
        .setDepthCompareOp(vk::CompareOp::eLessOrEqual);
    //.setDepthBoundsTestEnable(false)
    //.setMinDepthBounds(0.0f)  // optional for now
    //.setMaxDepthBounds(1.0f)  // optional for now
    //.setStencilTestEnable(false)
    depthstencil.setFront(depthstencil.back);
    depthstencil.back.compareOp = vk::CompareOp::eAlways;

    // Now this viewport and scissor rectangle need to be combined into a viewport state using the
    // VkPipelineViewportStateCreateInfo struct. It is possible to use multiple viewports and
    // scissor rectangles on some graphics cards, so its members reference an array of them. Using
    // multiple requires enabling a GPU feature (see logical device creation).
    auto viewportstate =
      vk::PipelineViewportStateCreateInfo().setViewportCount(1).setScissorCount(1).setFlags(
        vk::PipelineViewportStateCreateFlagBits(0));
    //.setPViewports(&viewport)
    //.setPScissors(&scissor);

    // The VkPipelineMultisampleStateCreateInfo struct configures multisampling, which is one of the
    // ways to perform anti-aliasing. It works by combining the fragment shader results of multiple
    // polygons that rasterize to the same pixel. This mainly occurs along edges, which is also
    // where the most noticeable aliasing artifacts occur. Because it doesn't need to run the
    // fragment shader multiple times if only one polygon maps to a pixel, it is significantly less
    // expensive than simply rendering to a higher resolution and then downscaling. Enabling it
    // requires enabling a GPU feature (We don't).
    auto multisampling = vk::PipelineMultisampleStateCreateInfo()
                           //.setSampleShadingEnable(false)
                           .setRasterizationSamples(vk::SampleCountFlagBits::e1)
                           .setFlags(vk::PipelineMultisampleStateCreateFlags(0));

    // A limited amount of the state that we've specified in the previous structs can actually be
    // changed without recreating the pipeline. Examples are the size of the viewport, line width
    // and blend constants. If you want to do that, then you'll have to fill in a
    // VkPipelineDynamicStateCreateInfo
    // This will cause the configuration of these values to be ignored and you will be required to
    // specify the data at drawing time.
    std::array<vk::DynamicState, 2> dynamicStateEnables = { vk::DynamicState::eViewport,
                                                            vk::DynamicState::eScissor };

    auto dynamicStateInfo =
      vk::PipelineDynamicStateCreateInfo()
        .setPDynamicStates(dynamicStateEnables.data())
        .setDynamicStateCount(static_cast<uint32_t>(dynamicStateEnables.size()))
        .setFlags(vk::PipelineDynamicStateCreateFlagBits(0));


    // This is where we setup the Vertex Descriptions
    // vertex input
    // The VkPipelineVertexInputStateCreateInfo structure describes the format of the vertex data
    // that will be passed to the vertex shader. It describes this in roughly two ways:
    //  - Bindings: spacing between data and whether the data is per-vertex or per-instance (see
    //    instancing)
    //  - Attribute descriptions: type of the attributes passed to the vertex shader,
    //    which binding to load them from and at which offset
    auto bindingdescription    = getBindingDescription();
    auto attributedescriptions = getAttributeDescription();

    auto vertexinputinfo =
      vk::PipelineVertexInputStateCreateInfo()
        .setVertexBindingDescriptionCount(static_cast<uint32_t>(bindingdescription.size()))
        .setPVertexBindingDescriptions(bindingdescription.data())
        .setVertexAttributeDescriptionCount(static_cast<uint32_t>(attributedescriptions.size()))
        .setPVertexAttributeDescriptions(attributedescriptions.data());


    // After a fragment shader has returned a color, it needs to be combined with the color that is
    // already in the framebuffer. This transformation is known as color blending and there are two
    // ways to do it:
    //  - Mix the old and new value to produce a final color
    //  - Combine the old and new value using a bitwise operation


    auto shaderstages = std::array<vk::PipelineShaderStageCreateInfo, 2>();

    // And finally we have the reference to the render pass. It is also possible to use other
    // render passes with this pipeline instead of this specific instance, but they have to be
    // compatible with renderPass. The requirements for compatibility are described
    // https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#renderpass-compatibility
    auto pipelinecreateinfo = vk::GraphicsPipelineCreateInfo()
                                .setLayout(m_pipeline_layouts.deferred)
                                .setRenderPass(m_render_pass)
                                .setFlags(vk::PipelineCreateFlags(0))
                                .setBasePipelineIndex(-1)
                                .setPInputAssemblyState(&inputassembly)
                                .setPRasterizationState(&rasterizer)
                                .setPColorBlendState(&colorblending)
                                .setPMultisampleState(&multisampling)
                                .setPViewportState(&viewportstate)
                                .setPDepthStencilState(&depthstencil)
                                .setPDynamicState(&dynamicStateInfo)
                                .setStageCount(static_cast<uint32_t>(shaderstages.size()))
                                .setPStages(shaderstages.data());
    //.setPVertexInputState(&vertexinputinfo)
    // The index of the sub pass where this graphics pipeline will be used.
    //.setSubpass(0);

    // Shader loading
    // Final fullscreen composition pass pipeline
    shaderstages[0] =
      loadShader("main", "shaders/deferred/deferred.vert.spv", vk::ShaderStageFlagBits::eVertex);
    shaderstages[1] =
      loadShader("main", "shaders/deferred/deferred.frag.spv", vk::ShaderStageFlagBits::eFragment);

    auto emptyInputState                 = vk::PipelineVertexInputStateCreateInfo();
    pipelinecreateinfo.pVertexInputState = &emptyInputState;
    pipelinecreateinfo.layout            = m_pipeline_layouts.deferred;
    // pipelinecreateinfo.setPVertexInputState(&emptyInputState)
    //.setLayout(m_pipeline_layouts.deferred);
    if (!(m_graphics_pipelines.deferred =
            m_device.createGraphicsPipeline(m_pipeline_cache, pipelinecreateinfo))) {
        std::runtime_error("Error creating deferred pipeline!\n");
    }

    // Debug display pipeline
    pipelinecreateinfo.pVertexInputState = &vertexinputinfo;
    shaderstages[0] =
      loadShader("main", "shaders/deferred/debug.vert.spv", vk::ShaderStageFlagBits::eVertex);
    shaderstages[1] =
      loadShader("main", "shaders/deferred/debug.frag.spv", vk::ShaderStageFlagBits::eFragment);
    if (!(m_graphics_pipelines.debug =
            m_device.createGraphicsPipeline(m_pipeline_cache, pipelinecreateinfo))) {
        std::runtime_error("Error creating debug pipeline!\n");
    }

    // Offscreen pipeline
    shaderstages[0] =
      loadShader("main", "shaders/deferred/mrt.vert.spv", vk::ShaderStageFlagBits::eVertex);
    shaderstages[1] =
      loadShader("main", "shaders/deferred/mrt.frag.spv", vk::ShaderStageFlagBits::eFragment);
    // Separate render pass
    pipelinecreateinfo.setRenderPass(m_offscreen_framebuffer.renderPass);
    // Separate layout
    pipelinecreateinfo.setLayout(m_pipeline_layouts.offscreen);

    // Blend attachment states required for all color attachments
    // This is important, as color write mask will otherwise be 0x0 and you
    // won't see anything rendered to the attachment
    auto blendState =
      vk::PipelineColorBlendAttachmentState()
        .setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
                           | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
        .setBlendEnable(false);
    auto blendAttachmentStates =
      std::array<vk::PipelineColorBlendAttachmentState, 3>{ blendState, blendState, blendState };
    colorblending.setAttachmentCount(static_cast<uint32_t>(blendAttachmentStates.size()))
      .setPAttachments(blendAttachmentStates.data());

    if (!(m_graphics_pipelines.offscreen =
            m_device.createGraphicsPipeline(m_pipeline_cache, pipelinecreateinfo))) {
        std::runtime_error("Error creating offscreen pipeline!\n");
    }
}

/*
The attachments specified during render pass creation are bound by wrapping them into a
VkFramebuffer object. A framebuffer object references all of the VkImageView objects that represent
the attachments. In our case that will be only a single one: the color attachment. However, the
image that we have to use for the attachment depends on which image the swap chain returns when we
retrieve one for presentation. That means that we have to create a framebuffer for all of the images
in the swap chain and use the one that corresponds to the retrieved image at drawing time.

https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VkFramebuffer
*/
void
renderer::createFramebuffers()
{
    vk::ImageView attachments[2];
    attachments[1] = m_offscreen_framebuffer.depth.image_view;

    auto framebufferinfo = vk::FramebufferCreateInfo()
                             .setPNext(NULL)
                             .setRenderPass(m_render_pass)
                             // The attachmentCount and pAttachments parameters specify the
                             // VkImageView objects that should be bound to the respective
                             // attachment descriptions in the render pass pAttachment array.
                             .setAttachmentCount(2)
                             //.setAttachmentCount(static_cast<uint32_t>(attachments.size()))
                             .setPAttachments(attachments)
                             .setWidth(m_win_width)
                             .setHeight(m_win_height)
                             // layers refers to the number of layers in image arrays. Our swap
                             // chain images are single images, so the number of layers is 1.
                             .setLayers(1);

    m_framebuffers.resize(m_swapchain_struct.imageCount);
    for (uint32_t i = 0; i < m_framebuffers.size(); ++i) {
        attachments[0]   = m_swapchain_struct.buffers[i].view;
        auto framebuffer = m_device.createFramebuffer(framebufferinfo);
        if (!framebuffer) {
            throw std::runtime_error("failed to create framebuffer!");
        }
        m_framebuffers[i] = framebuffer;
        std::cout << "FrameBuffer[" << i << "] created: address is " << &m_framebuffers[i]
                  << std::endl;
    }
    std::cout << "Swapchain Framebuffers created!\n";
}

/*
Commands in Vulkan, like drawing operations and memory transfers, are not executed directly using
function calls. You have to record all of the operations you want to perform in command buffer
objects. The advantage of this is that all of the hard work of setting up the drawing commands can
be done in advance and in multiple threads. After that, you just have to tell Vulkan to execute the
commands in the main loop.

We have to create a command pool before we can create command buffers. Command pools manage the
memory that is used to store the buffers and command buffers are allocated from them.

https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VkCommandPool
*/
void
renderer::createCommandPool()
{
    auto indices = findQueueFamilies(m_physical_device, m_surface);

    auto commandpoolinfo =
      vk::CommandPoolCreateInfo()
        // Command buffers are executed by submitting them on one of the device queues, like the
        // graphics and presentation queues we retrieved. Each command pool can only allocate
        // command buffers that are submitted on a single type of queue. We're going to record
        // commands for drawing, which is why we've chosen the graphics queue family.
        .setQueueFamilyIndex(indices.graphicsFamily())
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer);

    m_command_pool = m_device.createCommandPool(commandpoolinfo);
}

vk::CommandBuffer
renderer::createCommandBuffer(vk::CommandBufferLevel level, bool begin)
{
    auto cmdBufAllocInfo = vk::CommandBufferAllocateInfo()
                             .setCommandPool(m_command_pool)
                             .setLevel(level)
                             .setCommandBufferCount(1);

    vk::CommandBuffer cmdBuffer;
    m_device.allocateCommandBuffers(&cmdBufAllocInfo, &cmdBuffer);

    if (begin) {
        auto cmdBufInfo = vk::CommandBufferBeginInfo();
        cmdBuffer.begin(&cmdBufInfo);
    }
    return cmdBuffer;
}

void
renderer::flushCommandBuffer(vk::CommandBuffer commandBuffer, vk::Queue queue, bool free)
{
    if (!commandBuffer) {
        return;
    }

    commandBuffer.end();

    auto submitInfo = vk::SubmitInfo().setCommandBufferCount(1).setPCommandBuffers(&commandBuffer);

    queue.submit(1, &submitInfo, vk::Fence());
    queue.waitIdle();

    if (free) {
        m_device.freeCommandBuffers(m_command_pool, 1, &commandBuffer);
    }
}

/*
Command buffers are automatically freed when their command pools are destroyed

Command buffers are objects used to record commands which can be subsequently submitted to a device
queue for execution. There are two levels of command buffers - primary command buffers, which can
execute secondary command buffers, and which are submitted to queues, and secondary command buffers,
which can be executed by primary command buffers, and which are not directly submitted to queues.

Recorded commands include commands to bind pipelines and descriptor sets to the command buffer,
commands to modify dynamic state, commands to draw (for graphics rendering), commands to dispatch
(for compute), commands to execute secondary command buffers (for primary command buffers only),
commands to copy buffers and images, and other commands.

Each command buffer manages state independently of other command buffers. There is no inheritance of
state across primary and secondary command buffers, or between secondary command buffers. When a
command buffer begins recording, all state in that command buffer is undefined. When secondary
command buffer(s) are recorded to execute on a primary command buffer, the secondary command buffer
inherits no state from the primary command buffer, and all state of the primary command buffer is
undefined after an execute secondary command buffer command is recorded. There is one exception to
this rule - if the primary command buffer is inside a render pass instance, then the render pass and
subpass state is not disturbed by executing secondary command buffers. Whenever the state of a
command buffer is undefined, the application must set all relevant state on the command buffer
before any state dependent commands such as draws and dispatches are recorded, otherwise the
behavior of executing that command buffer is undefined.

Unless otherwise specified, and without explicit synchronization, the various commands submitted to
a queue via command buffers may execute in arbitrary order relative to each other, and/or
concurrently. Also, the memory side-effects of those commands may not be directly visible to other
commands without explicit memory dependencies. This is true within a command buffer, and across
command buffers submitted to a given queue.

https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#VkCommandBuffer
*/
void
renderer::createCommandBuffers()
{
    m_command_buffers.resize(m_swapchain_struct.imageCount);

    auto allocinfo =
      vk::CommandBufferAllocateInfo()
        .setCommandPool(m_command_pool)
        // The level parameter specifies if the allocated command buffers are primary or secondary
        // command buffers.
        //  - VK_COMMAND_BUFFER_LEVEL_PRIMARY: Can be submitted to a queue for execution, but cannot
        //  be called from other command buffers.
        //  - VK_COMMAND_BUFFER_LEVEL_SECONDARY: Cannot be submitted directly, but can be called
        //  from primary command buffers.
        // We won't make use of the secondary command buffer functionality here, but you can imagine
        // that it's helpful to reuse common operations from primary command buffers.
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(static_cast<uint32_t>(m_command_buffers.size()));

    m_command_buffers = m_device.allocateCommandBuffers(allocinfo);
    // std::cout << "m_command_buffers\n";
}

/*
Semaphores are a synchronization primitive that can be used to insert a dependency between batches
submitted to queues. Semaphores have two states - signaled and unsignaled. The state of a semaphore
can be signaled after execution of a batch of commands is completed. A batch can wait for a
semaphore to become signaled before it begins execution, and the semaphore is also unsignaled before
the batch begins execution.

As with most objects in Vulkan, semaphores are an interface to internal data which is typically
opaque to applications. This internal data is referred to as a semaphore’s payload.

However, in order to enable communication with agents outside of the current device, it is necessary
to be able to export that payload to a commonly understood format, and subsequently import from that
format as well.

The internal data of a semaphore may include a reference to any resources and pending work
associated with signal or unsignal operations performed on that semaphore object. Mechanisms to
import and export that internal data to and from semaphores are provided below. These mechanisms
indirectly enable applications to share semaphore state between two or more semaphores and other
synchronization primitives across process and API boundaries.

Semaphores are fundamentally a GPU-GPU synchronisation object

https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#synchronization-semaphores
*/
void
renderer::createSemaphores()
{
    for (uint32_t i = 0; i < max_frames_in_flight; ++i) {
        m_image_available_semaphores.push_back(m_device.createSemaphore(vk::SemaphoreCreateInfo()));
        m_render_finished_semaphores.push_back(m_device.createSemaphore(vk::SemaphoreCreateInfo()));
    }

    m_present_complete = m_device.createSemaphore(vk::SemaphoreCreateInfo());
    m_render_complete  = m_device.createSemaphore(vk::SemaphoreCreateInfo());

    m_submit_info = vk::SubmitInfo()
                      .setPWaitDstStageMask(&m_submit_pipeline_stages)
                      .setWaitSemaphoreCount(1)
                      // every semaphore here must match a vk::PipelineStageFlags, index for
                      // index. The masks dictate which stage of the pipeline is mapped to which
                      // semaphore the application must wait on
                      // Therefore wait_stages must be exactly the same length as wait_semaphores
                      .setPWaitSemaphores(&m_present_complete)
                      .setSignalSemaphoreCount(1)
                      .setPSignalSemaphores(&m_render_complete);
}

/*
To perform CPU-GPU synchronization, Vulkan offers a second type of synchronization primitive called
fences. Fences are similar to semaphores in the sense that they can be signaled and waited for, but
this time we actually wait for them in our own code.

https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html#synchronization-fences
*/
void
renderer::createFences()
{
    // we create this fence already signalled, which it isn't by default
    auto fenceinfo = vk::FenceCreateInfo().setFlags(vk::FenceCreateFlagBits::eSignaled);

    m_wait_fences.reserve(static_cast<uint32_t>(m_command_buffers.size()));
    for (uint32_t i = 0; i < m_command_buffers.size(); ++i) {
        m_wait_fences.emplace_back(m_device.createFence(fenceinfo));
    }
}

void
renderer::buildCommandBuffers()
{
    // The flags parameter specifies how we're going to use the command buffer. The
    // following values are available:
    //  - VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded
    //    right after executing it once.
    //  - VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT:
    //    This is a secondary command buffer that will be entirely within a single render
    //    pass.
    //  - VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The command buffer can be
    //    resubmitted while it is also already pending execution.
    // We have used the last flag because we may already be scheduling the drawing commands
    // for the next frame while the last frame is not finished yet.
    auto begininfo = vk::CommandBufferBeginInfo();
    //.setFlags(vk::CommandBufferUsageFlagBits::eSimultaneousUse);

    /*The range of depths in the depth buffer is 0.0 to 1.0 in Vulkan, where 1.0 lies at the
     * far view plane and 0.0 at the near view plane. The initial value at each point in the
     * depth buffer should be the furthest possible depth, which is 1.0.*/
    std::array<vk::ClearValue, 2> clearValues = {};
    clearValues[0].setColor(vk::ClearColorValue(backgroundColor));
    clearValues[1].setDepthStencil(vk::ClearDepthStencilValue(1.0f, 0));

    // Render Area
    auto renderarea = vk::Rect2D({ 0, 0 }, vk::Extent2D(m_win_width, m_win_height));

    // Create Render pass begin info
    auto renderpassinfo =
      vk::RenderPassBeginInfo()
        // The first parameters are the render pass itself and the attachments to bind. We
        // created a framebuffer for each swap chain image that specifies it as color
        // attachment.
        .setRenderPass(m_render_pass)
        //.setFramebuffer(framebuffer)
        // The render area defines where shader loads and stores will take place. The pixels
        // outside this region will have undefined values. It should match the size of the
        // attachments for best performance.
        .setRenderArea(renderarea)
        // The last two parameters define the clear values to use for
        // VK_ATTACHMENT_LOAD_OP_CLEAR, which we used as load operation for the color
        // attachment. I've defined the clear color to simply be black with 100% opacity.
        .setClearValueCount(static_cast<uint32_t>(clearValues.size()))
        .setPClearValues(clearValues.data());

    // We begin recording a command buffer by calling vkBeginCommandBuffer with a small
    // VkCommandBufferBeginInfo structure as argument that specifies some details about the usage of
    // this specific command buffer.whereupon which we can use vkCmd*-named calls to record draw
    // commands/dispatch compute commands

    for (size_t i = 0; i < m_command_buffers.size(); ++i) {
        auto const& command_buffer = m_command_buffers[i];
        renderpassinfo.setFramebuffer(m_framebuffers[i]);

        recordCommandBuffer(command_buffer, begininfo, [=]() {
            // The render pass can now begin. All of the functions that record commands can be
            // recognized by their vkCmd prefix. They all return void, so there will be no error
            // handling until we've finished recording.
            // The first parameter for every command is always the command buffer to record the
            // command to. The second parameter specifies the details of the render pass we've
            // just provided. The final parameter controls how the drawing commands within the
            // render pass will be provided. It can have one of two values:
            //  - VK_SUBPASS_CONTENTS_INLINE: The render pass commands will be embedded in the
            //    primary command buffer itself and no secondary command buffers will be executed.
            //  - VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS: The render pass commands will be
            //    executed from secondary command buffers.
            // We will not be using secondary command buffers, so we'll go with the first option.
            recordCommandBufferRenderPass(
              command_buffer, renderpassinfo, vk::SubpassContents::eInline, [=]() {
                  // Setup the Viewport, Scissor, and Device Size offsets
                  auto viewport =
                    vk::Viewport(0.0f, 0.0f, (float)m_win_width, (float)m_win_height, 0.0f, 1.0f);
                  command_buffer.setViewport(0, 1, &viewport);

                  auto scissor =
                    vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(m_win_width, m_win_height));
                  command_buffer.setScissor(0, 1, &scissor);

                  vk::DeviceSize offsets[1] = { 0 };
                  command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                    m_pipeline_layouts.deferred, 0, 1,
                                                    &m_offscreen_quads.descriptor_set, 0, NULL);

                  // TODO: Add debugdisplay boolean that allows us to set debug view
                  if (debugDisplay) {
                      command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                                  m_graphics_pipelines.debug);
                      command_buffer.bindVertexBuffers(0, 1, &m_offscreen_quads.vertex_buffer,
                                                       offsets);
                      command_buffer.bindIndexBuffer(m_offscreen_quads.index_buffer, 0,
                                                     vk::IndexType::eUint32);
                      command_buffer.drawIndexed(m_offscreen_quads.num_indices, 1, 0, 0, 1);

                      viewport.x      = viewport.width * 0.5f;
                      viewport.y      = viewport.height * 0.5f;
                      viewport.width  = viewport.width * 0.5f;
                      viewport.height = viewport.height * 0.5f;
                      command_buffer.setViewport(0, 1, &viewport);
                  }


                  // Final composition is a full screen quad
                  command_buffer.bindVertexBuffers(0, 1, &m_offscreen_quads.vertex_buffer, offsets);
                  // The second parameter specifies if the pipeline object is a graphics or
                  // compute pipeline. We've now told Vulkan which operations to execute in the
                  // graphics pipeline and which attachment to use in the fragment shader, so all
                  // that remains is telling it to draw the triangle:
                  command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                              m_graphics_pipelines.deferred);
                  command_buffer.bindIndexBuffer(m_offscreen_quads.index_buffer, 0,
                                                 vk::IndexType::eUint32);
                  command_buffer.drawIndexed(6, 1, 0, 0, 1);

                  /* NOTE: Everything below here is the previous version of how we bound each mesh
                   * to the Render pass. Instead we are now using a single quad for the final pass.
                   * this quad is full screen.
                   */
                  // Unlike vertex and index buffers, descriptor sets are not unique to
                  // graphics pipelines. Therefore we need to specify if we want to bind
                  // descriptor sets to the graphics or compute pipeline. The first parameter
                  // is the layout that the descriptors are based on. The next three
                  // parameters specify the index of the first descriptor set, the number of
                  // sets to bind, and the array of sets to bind. We'll get back to this in a
                  // moment. The last two parameters specify an array of offsets that are used
                  // for dynamic descriptors. We'll look at these in a future chapter.

                  // The actual vkCmdDraw function is a bit anticlimactic, but it's so simple
                  // because of all the information we specified in advance. It has the
                  // following parameters, aside from the command buffer:
                  // - vertexCount: Even though we don't have a vertex buffer, we technically
                  // still have
                  // 3
                  //   vertices to draw.
                  // - instanceCount: Used for instanced rendering, use 1 if you're not
                  //   doing that.
                  // - firstVertex: Used as an offset into the vertex buffer, defines the
                  // lowest
                  //   value of gl_VertexIndex.
                  // - firstInstance: Used as an offset for instanced rendering,
                  //   defines the lowest value of gl_InstanceIndex.
                  // A call to this function is very similar to vkCmdDraw. The first two
                  // parameters specify the number of indices and the number of instances.
                  // We're not using instancing, so just specify 1 instance. The number of
                  // indices represents the number of vertices that will be passed to the
                  // vertex buffer. The next parameter specifies an offset into the index
                  // buffer, using a value of 1 would cause the graphics card to start reading
                  // at the second index. The second to last parameter specifies an offset to
                  // add to the indices in the index buffer. The final parameter specifies an
                  // offset for instancing, which we're not using.
              });
        });
    }
}

void
renderer::buildDeferredCommandBuffer()
{
    if (!m_offscreen_commandbuffer) {
        m_offscreen_commandbuffer = createCommandBuffer(vk::CommandBufferLevel::ePrimary, false);
    }

    // Create a semaphore used to syncrhonize offscreen rendering and usage
    auto semaphoreCreateInfo = vk::SemaphoreCreateInfo();
    m_device.createSemaphore(&semaphoreCreateInfo, nullptr, &m_offscreen_semaphore);

    // Clear values for all attachments written in the fragment shader
    std::array<float, 4> colorR  = { 1.f, 0.f, 0.f, 0.f };
    std::array<float, 4> colorG  = { 0.f, 1.f, 0.f, 0.f };
    std::array<float, 4> colorB  = { 0.f, 0.f, 1.f, 0.f };
    std::array<float, 4> NoColor = { 0.f, 0.f, 0.f, 0.f };

    std::array<vk::ClearValue, 4> clearValues = {};
    clearValues[0].setColor(NoColor);
    clearValues[1].setColor(NoColor);
    clearValues[2].setColor(NoColor);
    clearValues[3].setDepthStencil(vk::ClearDepthStencilValue(1.0f, 0));

    auto renderarea = vk::Rect2D(
      { 0, 0 }, vk::Extent2D(m_offscreen_framebuffer.width, m_offscreen_framebuffer.height));

    auto cmdBufinfo = vk::CommandBufferBeginInfo();

    auto renderPassBeginInfo = vk::RenderPassBeginInfo()
                                 .setRenderPass(m_offscreen_framebuffer.renderPass)
                                 .setFramebuffer(m_offscreen_framebuffer.frameBuffer)
                                 .setRenderArea(renderarea)
                                 .setClearValueCount(static_cast<uint32_t>(clearValues.size()))
                                 .setPClearValues(clearValues.data());

    // Record Command Buffer
    auto const& command_buffer = m_offscreen_commandbuffer;

    recordCommandBuffer(command_buffer, cmdBufinfo, [=]() {
        recordCommandBufferRenderPass(
          command_buffer, renderPassBeginInfo, vk::SubpassContents::eInline, [=]() {
              // Setup the Viewport, Scissor, and Device Size offsets
              auto viewport =
                vk::Viewport(0.0f, 0.0f, (float)m_win_width, (float)m_win_height, 0.0f, 1.0f);
              command_buffer.setViewport(0, 1, &viewport);

              auto scissor =
                vk::Rect2D(vk::Offset2D(0, 0), vk::Extent2D(m_win_width, m_win_height));
              command_buffer.setScissor(0, 1, &scissor);

              vk::DeviceSize offsets[1] = { 0 };
              command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
                                          m_graphics_pipelines.offscreen);

              // For all meshes
              /*
              for (auto mesh : m_meshes) {
      // std::vector<vk::Buffer> vertexbuffers = { mesh.vertex_buffer };
      command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                        m_pipeline_layouts.offscreen, 0, 1,
                                        &mesh.descriptor_set, 0, NULL);
      command_buffer.bindVertexBuffers(0, 1, &mesh.vertex_buffer, offsets);
      command_buffer.bindIndexBuffer(mesh.index_buffer, 0, vk::IndexType::eUint32);
      // Note: This is where I can set number of instanced meshes
      command_buffer.drawIndexed(mesh.num_indices, 3, 0, 0, 0);
  }*/

              command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                m_pipeline_layouts.offscreen, 0, 1,
                                                &m_meshes[0].descriptor_set, 0, NULL);
              command_buffer.bindVertexBuffers(0, 1, &m_meshes[0].vertex_buffer, offsets);
              command_buffer.bindIndexBuffer(m_meshes[0].index_buffer, 0, vk::IndexType::eUint32);
              // Note: This is where I can set number of instanced meshes
              command_buffer.drawIndexed(m_meshes[0].num_indices, 3, 0, 0, 0);
          });
    });

    // Meshes
}

/*
Buffers in Vulkan are regions of memory used for storing arbitrary data that can be read by the
graphics card. They can be used to store vertex data, which we'll do in this chapter, but they can
also be used for many other purposes that we'll explore in future chapters. Unlike the Vulkan
objects we've been dealing with so far, buffers do not automatically allocate memory for themselves.
The work from the previous chapters has shown that the Vulkan API puts the programmer in control of
almost everything and memory management is one of those things.

https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#vkCreateBuffer
*/

void
renderer::createVertexBuffer(Mesh& mesh, const std::vector<Vertex> vertices)
{
    vk::DeviceSize size = sizeof(decltype(vertices)::value_type) * vertices.size();

    // vk::Buffer       stagingbuffer;
    // vk::DeviceMemory stagingbuffermemory;

    auto [stagingbuffer, stagingbuffermemory] = createBuffer(
      size, vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    // Now we copy the triangle vertex data into the buffer
    // You can now simply memcpy the vertex data to the mapped memory and unmap it again using
    // vkUnmapMemory. Unfortunately the driver may not immediately copy the data into the buffer
    // memory, for example because of caching. It is also possible that writes to the buffer are
    // not visible in the mapped memory yet. There are two ways to deal with that problem:
    //  - Use a memory heap that is host coherent, indicated with
    //  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    //  - Call vkFlushMappedMemoryRanges to after writing to the mapped memory, and call
    //  vkInvalidateMappedMemoryRanges before reading from the mapped memory
    // We went for the first approach, which ensures that the mapped memory always matches the
    // contents of the allocated memory. Do keep in mind that this may lead to slightly worse
    // performance than explicit flushing, but we'll see why that doesn't matter in the next
    // chapter.
    // {
    //     void* data = m_device.mapMemory(stagingbuffermemory, 0, size);
    //     std::memcpy(data, triangle_vertices.data(), size);
    //     m_device.unmapMemory(stagingbuffermemory);
    // }

    withMappedMemory(stagingbuffermemory, 0, size,
                     [=](void* data) { std::memcpy(data, vertices.data(), size); });

    // The vertexBuffer is now allocated from a memory type that is device local, which
    // generally means that we're not able to use vkMapMemory. However, we can copy data from
    // the stagingBuffer to the vertexBuffer (using copyBuffer()). We have to indicate that we
    // intend to do that by specifying the transfer source flag for the stagingBuffer and the
    // transfer destination flag for the vertexBuffer, along with the vertex buffer usage flag.
    std::tie(mesh.vertex_buffer, mesh.vertex_buffer_memory) = createBuffer(
      size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
      vk::MemoryPropertyFlagBits::eDeviceLocal);

    copyBuffer(stagingbuffer, &mesh.vertex_buffer, size);

    m_device.destroyBuffer(stagingbuffer);
    m_device.freeMemory(stagingbuffermemory);

    // All that remains now is binding the vertex buffer during rendering operations.
}

void
renderer::createIndexBuffer(Mesh& mesh, const std::vector<uint32_t> indices)
{
    vk::DeviceSize size = sizeof(decltype(indices)::value_type) * indices.size();

    // vk::Buffer       stagingbuffer;
    // vk::DeviceMemory stagingbuffermemory;

    auto [stagingbuffer, stagingbuffermemory] = createBuffer(
      size, vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    // {
    //     void* data = m_device.mapMemory(stagingbuffermemory, 0, size);
    //     std::memcpy(data, triangle_indices.data(), size);
    //     m_device.unmapMemory(stagingbuffermemory);
    // }

    withMappedMemory(stagingbuffermemory, 0, size,
                     [=](void* data) { std::memcpy(data, indices.data(), size); });

    std::tie(mesh.index_buffer, mesh.index_buffer_memory) = createBuffer(
      size, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
      vk::MemoryPropertyFlagBits::eDeviceLocal);

    copyBuffer(stagingbuffer, &mesh.index_buffer, size);

    m_device.destroyBuffer(stagingbuffer);
    m_device.freeMemory(stagingbuffermemory);

    // All that remains now is binding the index buffer during rendering operations.
}

void
renderer::createUniformBuffer(Mesh& mesh, const vk::DeviceSize buffersize)
{
    std::tie(mesh.uniform_buffer, mesh.uniform_buffer_memory) = createBuffer(
      buffersize, vk::BufferUsageFlagBits::eUniformBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
}

void
renderer::prepareOffscreenFramebuffer()
{
    // m_offscreen_framebuffer.width  = 2048;
    // m_offscreen_framebuffer.height = 2048;
    m_offscreen_framebuffer.width  = m_win_width;
    m_offscreen_framebuffer.height = m_win_height;

    // Color attachments

    // (World space) positions
    createAttachment(vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment,
                     &m_offscreen_framebuffer.position);

    // (World space) Normals
    createAttachment(vk::Format::eR16G16B16A16Sfloat, vk::ImageUsageFlagBits::eColorAttachment,
                     &m_offscreen_framebuffer.normal);

    // Albedo (color)
    createAttachment(vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eColorAttachment,
                     &m_offscreen_framebuffer.albedo);

    // Depth attachment
    // Find a suitable depth format
    vk::Format attachmentDepthFormat;
    vk::Bool32 validDepthFormat = getSupportedDepthFormat(&attachmentDepthFormat);
    if (!validDepthFormat) {
        throw std::runtime_error("No valid Depth Format found!\n");
    }
    // Then create the Depth attachment
    createAttachment(attachmentDepthFormat, vk::ImageUsageFlagBits::eDepthStencilAttachment,
                     &m_offscreen_framebuffer.depth);

    // Set up separate renderpass with references to the color and depth attachments
    std::array<vk::AttachmentDescription, 4> attachmentDescriptions = {};

    // Init attachment properties
    for (uint32_t i = 0; i < 4; ++i) {
        attachmentDescriptions[i].samples        = vk::SampleCountFlagBits::e1;
        attachmentDescriptions[i].loadOp         = vk::AttachmentLoadOp::eClear;
        attachmentDescriptions[i].storeOp        = vk::AttachmentStoreOp::eStore;
        attachmentDescriptions[i].stencilLoadOp  = vk::AttachmentLoadOp::eDontCare;
        attachmentDescriptions[i].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachmentDescriptions[i].initialLayout  = vk::ImageLayout::eUndefined;
        if (i == 3) {
            attachmentDescriptions[i].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        } else {
            attachmentDescriptions[i].finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        }
    }
    // Formats of each attachment
    attachmentDescriptions[0].format = m_offscreen_framebuffer.position.format;
    attachmentDescriptions[1].format = m_offscreen_framebuffer.normal.format;
    attachmentDescriptions[2].format = m_offscreen_framebuffer.albedo.format;
    attachmentDescriptions[3].format = m_offscreen_framebuffer.depth.format;

    std::vector<vk::AttachmentReference> colorReferences;
    colorReferences.push_back({ 0, vk::ImageLayout::eColorAttachmentOptimal });
    colorReferences.push_back({ 1, vk::ImageLayout::eColorAttachmentOptimal });
    colorReferences.push_back({ 2, vk::ImageLayout::eColorAttachmentOptimal });

    vk::AttachmentReference depthReference = {};
    depthReference.attachment              = 3;
    depthReference.layout                  = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    vk::SubpassDescription subpass  = {};
    subpass.pipelineBindPoint       = vk::PipelineBindPoint::eGraphics;
    subpass.pColorAttachments       = colorReferences.data();
    subpass.colorAttachmentCount    = static_cast<uint32_t>(colorReferences.size());
    subpass.pDepthStencilAttachment = &depthReference;

    // Use subpass dependencies for attachment layout transitions
    std::array<vk::SubpassDependency, 2> dependencies;

    dependencies[0]
      .setSrcSubpass(VK_SUBPASS_EXTERNAL)
      .setDstSubpass(0)
      .setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
      .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setSrcAccessMask(vk::AccessFlagBits::eMemoryRead)
      .setDstAccessMask(vk::Flags(vk::AccessFlagBits::eColorAttachmentRead)
                        | vk::Flags(vk::AccessFlagBits::eColorAttachmentWrite))
      .setDependencyFlags(vk::DependencyFlagBits::eByRegion);

    dependencies[1]
      .setSrcSubpass(0)
      .setDstSubpass(VK_SUBPASS_EXTERNAL)
      .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
      .setDstStageMask(vk::PipelineStageFlagBits::eBottomOfPipe)
      .setSrcAccessMask(vk::Flags(vk::AccessFlagBits::eColorAttachmentRead)
                        | vk::Flags(vk::AccessFlagBits::eColorAttachmentWrite))
      .setDstAccessMask(vk::AccessFlagBits::eMemoryRead)
      .setDependencyFlags(vk::DependencyFlagBits::eByRegion);

    vk::RenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.setPAttachments(attachmentDescriptions.data())
      .setAttachmentCount(static_cast<uint32_t>(attachmentDescriptions.size()))
      .setSubpassCount(1)
      .setPSubpasses(&subpass)
      .setDependencyCount(2)
      .setPDependencies(dependencies.data());

    // m_device.createRenderPass(&renderPassInfo, nullptr, &m_offscreen_framebuffer.renderPass);

    if (!(m_offscreen_framebuffer.renderPass = m_device.createRenderPass(renderPassInfo))) {
        std::runtime_error("Error creating Offscreen Render Pass!\n");
    }

    // Create framebuffer
    std::array<vk::ImageView, 4> attachments;
    attachments[0] = m_offscreen_framebuffer.position.image_view;
    attachments[1] = m_offscreen_framebuffer.normal.image_view;
    attachments[2] = m_offscreen_framebuffer.albedo.image_view;
    attachments[3] = m_offscreen_framebuffer.depth.image_view;

    auto fbufCreateInfo = vk::FramebufferCreateInfo()
                            .setPNext(NULL)
                            .setRenderPass(m_offscreen_framebuffer.renderPass)
                            .setAttachmentCount(static_cast<uint32_t>(attachments.size()))
                            .setPAttachments(attachments.data())
                            .setWidth(m_offscreen_framebuffer.width)
                            .setHeight(m_offscreen_framebuffer.height)
                            .setLayers(1);
    if (!(m_offscreen_framebuffer.frameBuffer = m_device.createFramebuffer(fbufCreateInfo))) {
        std::runtime_error("Error creating Offscreen Render Pass!\n");
    }
    std::cout << "Offscreen FrameBuffer created: " << &m_offscreen_framebuffer.frameBuffer
              << std::endl;

    // Create Sampler to sample from the color attachments
    vk::SamplerCreateInfo sampler = vk::SamplerCreateInfo();
    sampler.setMagFilter(vk::Filter::eNearest)
      .setMinFilter(vk::Filter::eNearest)
      .setMipmapMode(vk::SamplerMipmapMode::eLinear)
      .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
      .setAddressModeV(sampler.addressModeU)
      .setAddressModeW(sampler.addressModeU)
      .setMipLodBias(0.0f)
      .setMaxAnisotropy(1.0f)
      .setMinLod(0.0f)
      .setMaxLod(1.0f)
      .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
      .setUnnormalizedCoordinates(false)
      .setCompareEnable(false);
    if (!(m_color_sampler = m_device.createSampler(sampler))) {
        std::runtime_error("Error creating Offscreen Render Pass!\n");
    }
    std::cout << "Offscreen Framebuffer created!\n";
}

void
renderer::prepareUniformBuffers()
{
    // NOTE: The reason why I don't use the createUniformBuffer
    // helper function is simply because the uniform buffers
    // here are not full on meshes.

    // Camera
    /*std::tie(m_uniform_buffers.camera, m_uniform_buffers.camera_mem) = createBuffer(
      sizeof(uniformbufferobject), vk::BufferUsageFlagBits::eUniformBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);*/
    // Fullscreen vertex shader
    std::tie(m_uniform_buffers.vsFullScreen, m_uniform_buffers.vsFullScreen_mem) = createBuffer(
      sizeof(uboVS), vk::BufferUsageFlagBits::eUniformBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    // Deferred vertex shader
    std::tie(m_uniform_buffers.vsOffScreen, m_uniform_buffers.vsOffScreen_mem) = createBuffer(
      sizeof(uboOffscreenVS), vk::BufferUsageFlagBits::eUniformBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);
    // Deferred fragment shader
    std::tie(m_uniform_buffers.fsLights, m_uniform_buffers.fsLights_mem) = createBuffer(
      sizeof(uboFragmentLights), vk::BufferUsageFlagBits::eUniformBuffer,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    // Maybe need to call mapmemory
    /*m_uniform_buffers.camera_map =
      m_device.mapMemory(m_uniform_buffers.camera_mem, 0, sizeof(uniformbufferobject));*/
    m_uniform_buffers.vsFullScreen_map =
      m_device.mapMemory(m_uniform_buffers.vsFullScreen_mem, 0, sizeof(uboVS));
    m_uniform_buffers.vsOffScreen_map =
      m_device.mapMemory(m_uniform_buffers.vsOffScreen_mem, 0, sizeof(uboOffscreenVS));
    m_uniform_buffers.fsLights_map =
      m_device.mapMemory(m_uniform_buffers.fsLights_mem, 0, sizeof(uboFragmentLights));

    // Init ubo values
    uboOffscreenVS.instancePos[0] = glm::vec4(0.0f, 0.0, -2.0f, 0.0f);
    uboOffscreenVS.instancePos[1] = glm::vec4(-2.0f, 0.0, 0.0f, 0.0f);
    uboOffscreenVS.instancePos[2] = glm::vec4(2.0f, 0.0, 0.0f, 0.0f);

    // Update
    // updateUniformBuffer();  // update the camera
    updateUniformBuffersScreen();
    updateUniformBufferDeferredMatrices();
    updateUniformBufferDeferredLights();
}


/*
We need to provide details about every descriptor binding used in the shaders for pipeline creation,
just like we had to do for every vertex attribute and its location index.
*/
void
renderer::createDescriptorSetLayout()
{
    std::array<vk::DescriptorSetLayoutBinding, 5> setLayoutBindings{};

    // Binding 0: Vertex shader uniform buffer
    setLayoutBindings[0] =
      vk::DescriptorSetLayoutBinding()
        // The first two fields specify the `binding` used in the shader and
        // the type of descriptor, which is a uniform buffer object.
        .setBinding(0)
        // It is possible for the shader variable to represent an array of uniform buffer objects,
        // and descriptorCount specifies the number of values in the array. This could be used to
        // specify a transformation for each of the bones in a skeleton for skeletal animation, for
        // example.
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        // We also need to specify in which shader stages the descriptor is going to be referenced.
        // The stageFlags field can be a combination of VkShaderStageFlagBits values or the value
        // VK_SHADER_STAGE_ALL_GRAPHICS. In our case, we're only referencing the descriptor from the
        // vertex shader.
        .setStageFlags(vk::ShaderStageFlagBits::eVertex);

    // Binding 1: Position texture target / Scene colormap
    setLayoutBindings[1] = vk::DescriptorSetLayoutBinding()
                             .setBinding(1)
                             .setDescriptorCount(1)
                             .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                             .setStageFlags(vk::ShaderStageFlagBits::eFragment);
    //.setPImmutableSamplers(nullptr);

    // Binding 2: Normals texture target
    setLayoutBindings[2] = vk::DescriptorSetLayoutBinding()
                             .setBinding(2)
                             .setDescriptorCount(1)
                             .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                             .setStageFlags(vk::ShaderStageFlagBits::eFragment);

    // Binding 3: Albedo texture target
    setLayoutBindings[3] = vk::DescriptorSetLayoutBinding()
                             .setBinding(3)
                             .setDescriptorCount(1)
                             .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                             .setStageFlags(vk::ShaderStageFlagBits::eFragment);

    // Binding 4: Fragment shader uniform buffer
    setLayoutBindings[4] = vk::DescriptorSetLayoutBinding()
                             .setBinding(4)
                             .setDescriptorCount(1)
                             .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                             .setStageFlags(vk::ShaderStageFlagBits::eFragment);

    // Pack it all into Descriptor Set Layout Create Info
    auto layoutinfo = vk::DescriptorSetLayoutCreateInfo()
                        .setBindingCount(static_cast<uint32_t>(setLayoutBindings.size()))
                        .setPBindings(setLayoutBindings.data());

    if (!(m_descriptor_set_layout = m_device.createDescriptorSetLayout(layoutinfo))) {
        throw std::runtime_error("failed to create descriptor set layout1");
    }


    // You can use uniform values in shaders, which are globals similar to dynamic state variables
    // that can be changed at drawing time to alter the behavior of your shaders without having to
    // recreate them. They are commonly used to pass the transformation matrix to the vertex shader,
    // or to create texture samplers in the fragment shader.
    // These uniform values need to be specified during pipeline creation by creating a
    // VkPipelineLayout object.
    auto pipelinelayout =
      vk::PipelineLayoutCreateInfo().setSetLayoutCount(1).setPSetLayouts(&m_descriptor_set_layout);

    if (!(m_pipeline_layouts.deferred = m_device.createPipelineLayout(pipelinelayout))) {
        throw std::runtime_error("failed to create descriptor set layout1");
    }
    if (!(m_pipeline_layouts.offscreen = m_device.createPipelineLayout(pipelinelayout))) {
        throw std::runtime_error("failed to create descriptor set layout1");
    }
}

/*
Descriptor sets can't be created directly, they must be allocated from a pool like command buffers.
The equivalent for descriptor sets is unsurprisingly called a descriptor pool.

        Descriptor pool

        Actual descriptors are allocated from a descriptor pool telling the driver what types and
        how many descriptors this application will use

        An application can have multiple pools (e.g. for multiple threads) with any number of
        descriptor types as long as device limits are not surpassed

        It's good practice to allocate pools with actually required descriptor types and counts
*/
void
renderer::createDescriptorPool()
{
    std::array<vk::DescriptorPoolSize, 2> descriptorPoolSizes = {};
    // Uniform buffers : 3 for scene UBOs and 1 per object (scene and local matrices)
    descriptorPoolSizes[0].setType(vk::DescriptorType::eUniformBuffer).setDescriptorCount(20);
    //.setDescriptorCount(5 + static_cast<uint32_t>(m_meshes.size()));
    // Combined image samples : 4 for the render targets, 1 per mesh texture
    descriptorPoolSizes[1]
      .setType(vk::DescriptorType::eCombinedImageSampler)
      .setDescriptorCount(21);
    //.setDescriptorCount(4 + static_cast<uint32_t>(m_meshes.size()));

    auto poolinfo = vk::DescriptorPoolCreateInfo()
                      .setPoolSizeCount(static_cast<uint32_t>(descriptorPoolSizes.size()))
                      .setPPoolSizes(descriptorPoolSizes.data())
                      .setMaxSets(3);
    //.setMaxSets(static_cast<uint32_t>(descriptorPoolSizes.size()));

    if (!(m_descriptor_pool = m_device.createDescriptorPool(poolinfo))) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

/*
It appears that descriptor sets are the results, being specified via descriptor set layouts and
allocated from a descriptor pool

You don't need to explicitly clean up descriptor sets, because they will be automatically freed when
the descriptor pool is destroyed
*/
void
renderer::createDescriptorSet()
{
    std::vector<vk::DescriptorSetLayout> layouts = { m_descriptor_set_layout };

    // Textured Quad descriptor set
    auto allocinfo = vk::DescriptorSetAllocateInfo()
                       .setDescriptorPool(m_descriptor_pool)
                       .setDescriptorSetCount((uint32_t)layouts.size())
                       .setPSetLayouts(layouts.data());

    // m_offscreen_quads.descriptor_set = m_device.allocateDescriptorSets(allocinfo).front();

    if (!(m_offscreen_quads.descriptor_set = m_device.allocateDescriptorSets(allocinfo).front())) {
        std::runtime_error("Error allocating descriptor sets!\n");
    }

    // Uniform Buffer descriptors
    m_uniform_buffers.vsFullscreen_des = vk::DescriptorBufferInfo()
                                           //.setBuffer(m_offscreen_quads.uniform_buffer)
                                           .setBuffer(m_uniform_buffers.vsFullScreen)
                                           .setOffset(0)
                                           .setRange(sizeof(uboVS));

    m_uniform_buffers.vsOffscreen_des = vk::DescriptorBufferInfo()
                                          .setBuffer(m_uniform_buffers.vsOffScreen)
                                          .setOffset(0)
                                          .setRange(sizeof(uboOffscreenVS));

    m_uniform_buffers.fsLights_des = vk::DescriptorBufferInfo()
                                       .setBuffer(m_uniform_buffers.fsLights)
                                       .setOffset(0)
                                       .setRange(sizeof(uboFragmentLights));

    /*m_uniform_buffers.camera_des = vk::DescriptorBufferInfo()
                                     .setBuffer(m_uniform_buffers.camera)
                                     .setOffset(0)
                                     .setRange(sizeof(uniformbufferobject));*/

    // Image descriptors for the offscreen color attachments
    auto texDescriptorPosition = vk::DescriptorImageInfo()
                                   .setSampler(m_color_sampler)
                                   .setImageView(m_offscreen_framebuffer.position.image_view)
                                   .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    auto texDescriptorAlbedo = vk::DescriptorImageInfo()
                                 .setSampler(m_color_sampler)
                                 .setImageView(m_offscreen_framebuffer.albedo.image_view)
                                 .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    auto texDescriptorNormal = vk::DescriptorImageInfo()
                                 .setSampler(m_color_sampler)
                                 .setImageView(m_offscreen_framebuffer.normal.image_view)
                                 .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal);

    std::vector<vk::WriteDescriptorSet> writeDescriptorSets = {
        // Binding 0: Vertex shader uniform buffer
        vk::WriteDescriptorSet()
          .setDstBinding(0)
          .setPBufferInfo(&m_uniform_buffers.vsFullscreen_des)
          .setDstSet(m_offscreen_quads.descriptor_set)
          .setDescriptorType(vk::DescriptorType::eUniformBuffer)
          .setDescriptorCount(1),
        // Binding 1: Position texture target
        vk::WriteDescriptorSet()
          .setDstBinding(1)
          .setPImageInfo(&texDescriptorPosition)
          .setDstSet(m_offscreen_quads.descriptor_set)
          .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
          .setDescriptorCount(1),
        // Binding 2: Normals texture target
        vk::WriteDescriptorSet()
          .setDstBinding(2)
          .setPImageInfo(&texDescriptorNormal)
          .setDstSet(m_offscreen_quads.descriptor_set)
          .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
          .setDescriptorCount(1),
        // Binding 3: Albedo texture target
        vk::WriteDescriptorSet()
          .setDstBinding(3)
          .setPImageInfo(&texDescriptorAlbedo)
          .setDstSet(m_offscreen_quads.descriptor_set)
          .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
          .setDescriptorCount(1),
        // Binding 4: Fragment shader uniform buffer
        vk::WriteDescriptorSet()
          .setDstBinding(4)
          .setPBufferInfo(&m_uniform_buffers.fsLights_des)
          .setDstSet(m_offscreen_quads.descriptor_set)
          .setDescriptorType(vk::DescriptorType::eUniformBuffer)
          .setDescriptorCount(1)
    };

    m_device.updateDescriptorSets(static_cast<uint32_t>(writeDescriptorSets.size()),
                                  writeDescriptorSets.data(), 0, NULL);

    /*
// NOTE: This returns a vector, not a single set
// NOTE: This call of allocateDescriptorSets causes an error message of a failure to
// allocate. Have to keep an eye on this to see if it goes away once the tutorial is done.
m_descriptor_set = m_device.allocateDescriptorSets(allocinfo).front();

// The descriptor set has been allocated now, but the descriptors within still need to be
// configured. Descriptors that refer to buffers, like our uniform buffer descriptor, are
// configured with a VkDescriptorBufferInfo struct. This structure specifies the buffer and
// the region within it that contains the data for the descriptor:
auto bufferInfo = vk::DescriptorBufferInfo()
                    .setBuffer(m_uniform_buffer)
                    .setOffset(0)
                    // If you're overwriting the whole buffer, like we are in this case, then
                    // it is is also possible to use the VK_WHOLE_SIZE value for the range.
                    .setRange(sizeof(uniformbufferobject));

// The configuration of descriptors is updated using the vkUpdateDescriptorSets function,
// which takes an array of VkWriteDescriptorSet structs as parameter.
std::array<vk::WriteDescriptorSet, 1> ddescriptorWrites = {};
ddescriptorWrites[0]
  // The first two fields specify the descriptor set to update and the binding
  .setDstSet(m_descriptor_set)
  //  We gave our uniform buffer binding index 0
  .setDstBinding(0)
  // Remember that descriptors can be arrays, so we also need to specify the first index in
  // the array that we want to update. We're not using an array, so the index is simply 0
  .setDstArrayElement(0)
  // We need to specify the type of descriptor again. It's possible to update multiple
  // descriptors at once in an array, starting at index dstArrayElement
  .setDescriptorType(vk::DescriptorType::eUniformBuffer)
  // The descriptorCount field specifies how many array elements you want to update.
  .setDescriptorCount(1)
  // The last field references an array with $descriptorCount structs that actually
  // configure the descriptors. It depends on the type of descriptor which one of the three
  // you actually need to use.
  .setPBufferInfo(&bufferInfo);

// The 0 is for the number of copies, and nullptr is for a vk::CopyDescriptorSet
m_device.updateDescriptorSets(static_cast<uint32_t>(ddescriptorWrites.size()),
                              ddescriptorWrites.data(), 0, nullptr);*/
    for (auto& mesh : m_meshes) {

        if (!(mesh.descriptor_set = m_device.allocateDescriptorSets(allocinfo).front())) {
            std::runtime_error("Error allocating Mesh descriptor set!");
        }

        mesh.buffer_info = vk::DescriptorBufferInfo()
                             .setBuffer(mesh.uniform_buffer)
                             .setOffset(0)
                             .setRange(sizeof(uboOffscreenVS));

        mesh.diffuse_tex.descriptor = vk::DescriptorImageInfo()
                                        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                                        .setImageView(mesh.diffuse_tex.image_view)
                                        .setSampler(mesh.diffuse_tex.sampler);

        mesh.normal_tex.descriptor = vk::DescriptorImageInfo()
                                       .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                                       .setImageView(mesh.normal_tex.image_view)
                                       .setSampler(mesh.normal_tex.sampler);

        std::array<vk::WriteDescriptorSet, 3> descriptorWrites = {};
        // Binding 0: Vertex shader uniform buffer
        descriptorWrites[0]
          .setDstSet(mesh.descriptor_set)
          .setDstBinding(0)
          .setDescriptorType(vk::DescriptorType::eUniformBuffer)
          .setDescriptorCount(1)
          .setPBufferInfo(&m_uniform_buffers.vsOffscreen_des);
        // Binding 1: Color map
        descriptorWrites[1]
          .setDstSet(mesh.descriptor_set)
          .setDstBinding(1)
          .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
          .setDescriptorCount(1)
          .setPImageInfo(&mesh.diffuse_tex.descriptor);
        // Binding 0: Normal map
        descriptorWrites[2]
          .setDstSet(mesh.descriptor_set)
          .setDstBinding(2)
          .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
          .setDescriptorCount(1)
          .setPImageInfo(&mesh.normal_tex.descriptor);

        m_device.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()),
                                      descriptorWrites.data(), 0, nullptr);
    }
}

/*
The geometry has been colored using per-vertex colors so far, which is a rather limited approach. In
this part of the tutorial we're going to implement texture mapping to make the geometry look more
interesting. This will also allow us to load and draw basic 3D models in a future chapter.

Adding a texture to our application will involve the following steps:

  - Create an image object backed by device memory
  - Fill it with pixels from an image file
  - Create an image sampler
  - Add a combined image sampler descriptor to sample colors from the texture

We've already worked with image objects before, but those were automatically created by the swap
chain extension. This time we'll have to create one by ourselves. Creating an image and filling it
with data is similar to vertex buffer creation. We'll start by creating a staging resource and
filling it with pixel data and then we copy this to the final image object that we'll use for
rendering. Although it is possible to create a staging image for this purpose, Vulkan also allows
you to copy pixels from a VkBuffer to an image and the API for this is actually faster on some
hardware. We'll first create this buffer and fill it with pixel values, and then we'll create an
image to copy the pixels to. Creating an image is not very different from creating buffers. It
involves querying the memory requirements, allocating device memory and binding it, just like we've
seen before.

However, there is something extra that we'll have to take care of when working with images. Images
can have different layouts that affect how the pixels are organized in memory. Due to the way
graphics hardware works, simply storing the pixels row by row may not lead to the best performance,
for example. When performing any operation on images, you must make sure that they have the layout
that is optimal for use in that operation. We've actually already seen some of these layouts when we
specified the render pass:

  - VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: Optimal for presentation
  - VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: Optimal as attachment for writing colors from the
fragment shader
  - VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: Optimal as source in a transfer operation,
like vkCmdCopyImageToBuffer
  - VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: Optimal as destination in a
transfer operation, like vkCmdCopyBufferToImage
  - VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: Optimal
for sampling from a shader

One of the most common ways to transition the layout of an image is a pipeline barrier. Pipeline
barriers are primarily used for synchronizing access to resources, like making sure that an image
was written to before it is read, but they can also be used to transition layouts. In this chapter
we'll see how pipeline barriers are used for this purpose. Barriers can additionally be used to
transfer queue family ownership when using VK_SHARING_MODE_EXCLUSIVE.
*/
void
renderer::createTextureImage(const std::string filename, Texture& texture)
{
    int      width, height, channels;
    stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("Failed to load image!");
    }

    vk::DeviceSize imagedim = width * height * 4;  // we load RGBA

    texture.width      = static_cast<uint32_t>(width);
    texture.height     = static_cast<uint32_t>(height);
    texture.mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    auto [stagingbuffer, stagingbuffermemory] = createBuffer(
      imagedim, vk::BufferUsageFlagBits::eTransferSrc,
      vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent);

    // NOTE: We might have to manipulate the bitmap because it stores things as BGRA for big-endian
    // systems.

    withMappedMemory(stagingbuffermemory, 0, imagedim,
                     [=](void* data) { std::memcpy(data, pixels, (size_t)imagedim); });

    stbi_image_free(pixels);

    std::tie(texture.image, texture.device_memory) = createImage(
      width, height, texture.mip_levels, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal,
      vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst
        | vk::ImageUsageFlagBits::eSampled,
      vk::MemoryPropertyFlagBits::eDeviceLocal);
    // Transition the texture image to VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
    transitionImageLayout(texture.image, vk::Format::eR8G8B8A8Unorm, vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eTransferDstOptimal, texture.mip_levels);
    // Execute the buffer to image copy operation
    copyBufferToImage(stagingbuffer, texture.image, width, height);

    m_device.destroyBuffer(stagingbuffer, nullptr);
    m_device.freeMemory(stagingbuffermemory, nullptr);

    generateMipmaps(texture.image, vk::Format::eR8G8B8A8Unorm, width, height, texture.mip_levels);
}

void
renderer::createTextureImageView(Texture& texture)
{
    texture.image_view = createImageView(texture.image, vk::Format::eR8G8B8A8Unorm,
                                         vk::ImageAspectFlagBits::eColor, texture.mip_levels);
}

void
renderer::createTextureSampler(Texture& texture)
{
    // Samplers are configured through a VkSamplerCreateInfo structure, which specifies all filters
    // and transformations that it should apply.
    vk::SamplerCreateInfo samplerInfo;
    // The magFilter and minFilter fields specify how to interpolate texels that are magnified or
    // minified. Magnification concerns the oversampling problem describes above, and minification
    // concerns undersampling. The choices are VK_FILTER_NEAREST and VK_FILTER_LINEAR, corresponding
    // to the modes demonstrated in the images above.
    samplerInfo.setMagFilter(vk::Filter::eLinear)
      .setMinFilter(vk::Filter::eLinear)
      /*The addressing mode can be specified per axis using the addressMode fields. The available
      values are listed below. Most of these are demonstrated in the image above. Note that the axes
      are called U, V and W instead of X, Y and Z. This is a convention for texture space
      coordinates.
      ===VK_SAMPLER_ADDRESS_MODE_REPEAT: Repeat the texture when going beyond the image dimensions.
      ===VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT: Like repeat, but inverts the coordinates to mirror
      the image when going beyond the dimensions. VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE: Take the
      color of the edge closest to the coordinate beyond the image dimensions.
      ===VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE: Like clamp to edge, but instead uses the edge
      opposite to the closest edge.
      ===VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER: Return a solid color when sampling
      beyond the dimensions of the image.
      It doesn't really matter which addressing mode we use here,
      because we're not going to sample outside of the image in this tutorial. However, the repeat
      mode is probably the most common mode, because it can be used to tile textures like floors and
      walls.*/
      .setAddressModeU(vk::SamplerAddressMode::eRepeat)
      .setAddressModeV(vk::SamplerAddressMode::eRepeat)
      .setAddressModeW(vk::SamplerAddressMode::eRepeat)
      /*These two fields specify if anisotropic filtering should be used. There is no reason not to
         use this unless performance is a concern. The maxAnisotropy field limits the amount of
         texel samples that can be used to calculate the final color. A lower value results in
         better performance, but lower quality results. There is no graphics hardware available
         today that will use more than 16 samples, because the difference is negligible beyond that
         point.*/
      // Side note by Jason: currently we only check if the physicaldevice is capable of anisotropy
      // in the isDeviceSuitable() function. At some point we might be better off storing the
      // physicalDeviceFeatures as a member value or scoping it here to set anisotropy settings
      // correctly based on available features.
      .setAnisotropyEnable(true)
      .setMaxAnisotropy(16.0)
      /*The borderColor field specifies which color is returned when sampling beyond the image with
         clamp to border addressing mode. It is possible to return black, white or transparent in
         either float or int formats. You cannot specify an arbitrary color.*/
      .setBorderColor(vk::BorderColor::eFloatOpaqueBlack)
      /*The unnormalizedCoordinates field specifies which coordinate system you want to use to
        address texels in an image. If this field is VK_TRUE, then you can simply use coordinates
        within the [0, texWidth) and [0, texHeight) range. If it is VK_FALSE, then the texels are
        addressed using the [0, 1) range on all axes. Real-world applications almost always use
        normalized coordinates, because then it's possible to use textures of varying resolutions
        with the exact same coordinates.*/
      .setUnnormalizedCoordinates(false)
      .setCompareEnable(false)
      .setCompareOp(vk::CompareOp::eAlways)
      .setMipmapMode(vk::SamplerMipmapMode::eLinear);

    if (!(texture.sampler = m_device.createSampler(samplerInfo))) {
        throw std::runtime_error("failed to create tetxture sampler!");
    }
}

void
renderer::createDepthResources()
{
    vk::Format depthFormat = findDepthFormat();

    std::tie(m_offscreen_framebuffer.depth.image, m_offscreen_framebuffer.depth.memory) =
      createImage(m_swapchain_struct.width, m_swapchain_struct.height, 1, depthFormat,
                  vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment,
                  vk::MemoryPropertyFlagBits::eDeviceLocal);
    m_offscreen_framebuffer.depth.image_view = createImageView(
      m_offscreen_framebuffer.depth.image, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);

    transitionImageLayout(m_offscreen_framebuffer.depth.image, depthFormat,
                          vk::ImageLayout::eUndefined,
                          vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);
}


/*
This is practically the core loop. Here we update and load the uniform variables per frame.

The tutorial mentioned something about push variables that are more efficient at sending small
amounts memory to the GPU, instead of using an expensive buffer operation

https://www.khronos.org/registry/vulkan/specs/1.1-extensions/html/vkspec.html#vkCmdPushConstants
*/
void
renderer::updateUniformBuffer()
{
    static auto start_t = std::chrono::high_resolution_clock::now();

    auto current_t = std::chrono::high_resolution_clock::now();

    float time =
      std::chrono::duration<float, std::chrono::seconds::period>(current_t - start_t).count();

    // @TODO Change this from automatic turntable to input based rotation
    // auto rotAngle = time * glm::radians(2.f);
    auto rotAngle = glm::radians(0.03f);  // *(time / 1000);
    m_camera.rotate(glm::vec3(0.f, rotAngle, 0.f));
    float c = glm::cos(rotAngle);
    float s = glm::sin(rotAngle);

    // Rotate camera about the origin (zero, zero, zero)
    auto newPos = m_camera.position;
    newPos.x    = (m_camera.position.x * c) - (m_camera.position.z * s);
    newPos.z    = (m_camera.position.x * s) + (m_camera.position.z * c);
    // std::cout << "New Position: " << newPos.x << " " << newPos.y << " " << newPos.z << std::endl;
    m_camera.setPosition(newPos);
    m_camera.matrices.view =
      glm::lookAt(m_camera.position, glm::vec3(0.f, 1.5f, 0.f), glm::vec3(0.f, 1.f, 0.f));

    // uniformbufferobject ubo;
    // m_camera.model = glm::mat4(1.0f);
    /*m_camera.model =
      glm::rotate(glm::mat4(1.f), time * glm::radians(15.f), glm::vec3(0.f, 1.f, 0.f));*/
    /*m_camera.matrices.view =
      glm::lookAt(m_camera.position, glm::vec3(0.f, 2.f, 0.f), glm::vec3(0.f, 1.f, 0.f));*/
    /*m_camera.matrices.perspective =
      glm::perspective(glm::radians(45.0f), (float)m_win_width / (float)m_win_height, 0.1f,
      100.0f);*/

    // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is
    // inverted. The easiest way to compensate for that is to flip the sign on the scaling factor of
    // the Y axis in the projection matrix. If you don't do this, then the image will be rendered
    // upside down.
    // m_camera.matrices.perspective[1][1] *= -1;

    // Until I figure out how to map a uniform buffer for the camera, this will just remain
    // commented out
    /*withMappedMemory(m_uniform_buffer_memory, 0, sizeof(uniformbufferobject),
                     [=](void* data) { std::memcpy(data, &ubo, sizeof(uniformbufferobject)); });*/

    // Update the meshes
    // Imrod translation
    // m_meshes[0].matrices.model = glm::translate(m_camera.model, glm::vec3(-2.0f, 0.0f, 0.0f));

    // Cube translation
    /*m_meshes[1].matrices.model = glm::translate(m_camera.model, glm::vec3(1.5f, 0.5f, 1.0f));
    m_meshes[1].matrices.model = glm::rotate(m_meshes[1].matrices.model, time * glm::radians(60.f),
                                             glm::vec3(1.0f, 1.0f, 1.0f));*/
    /*
for (auto& mesh : m_meshes) {
    mesh.matrices.proj = m_camera.proj;
    mesh.matrices.view = m_camera.view;
    withMappedMemory(
      mesh.uniform_buffer_memory, 0, sizeof(uniformbufferobject),
      [=](void* data) { std::memcpy(data, &mesh.matrices, sizeof(uniformbufferobject)); });
}*/
}

void
renderer::updateUniformBuffersScreen()
{
    if (debugDisplay) {
        uboVS.projection = glm::ortho(0.0f, 2.0f, 0.0f, 1.0f, -1.0f, 1.0f);
    } else {
        uboVS.projection = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f, -1.0f, 1.0f);
    }
    uboVS.model = glm::mat4(1.0f);

    withMappedMemory(m_uniform_buffers.vsFullScreen_mem, 0, sizeof(uboVS),
                     [=](void* data) { std::memcpy(data, &uboVS, sizeof(uboVS)); });
}

void
renderer::updateUniformBufferDeferredMatrices()
{

    uboOffscreenVS.projection = m_camera.matrices.perspective;
    uboOffscreenVS.view       = m_camera.matrices.view;
    uboOffscreenVS.model      = glm::mat4(1.0f);

    withMappedMemory(m_uniform_buffers.vsOffScreen_mem, 0, sizeof(uboOffscreenVS), [=](void* data) {
        std::memcpy(data, &uboOffscreenVS, sizeof(uboOffscreenVS));
    });
}

void
renderer::updateUniformBufferDeferredLights()
{
    // White
    uboFragmentLights.lights[0].position = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
    uboFragmentLights.lights[0].color    = glm::vec3(1.5f);
    uboFragmentLights.lights[0].radius   = 15.0f * 0.25f;
    // Red
    uboFragmentLights.lights[1].position = glm::vec4(-2.0f, 0.0f, 0.0f, 0.0f);
    uboFragmentLights.lights[1].color    = glm::vec3(1.0f, 0.0f, 0.0f);
    uboFragmentLights.lights[1].radius   = 15.0f;
    // Blue
    uboFragmentLights.lights[2].position = glm::vec4(2.0f, 1.0f, 0.0f, 0.0f);
    uboFragmentLights.lights[2].color    = glm::vec3(0.0f, 0.0f, 2.5f);
    uboFragmentLights.lights[2].radius   = 5.0f;
    // Yellow
    uboFragmentLights.lights[3].position = glm::vec4(0.0f, 0.9f, 0.5f, 0.0f);
    uboFragmentLights.lights[3].color    = glm::vec3(1.0f, 1.0f, 0.0f);
    uboFragmentLights.lights[3].radius   = 2.0f;
    // Green
    uboFragmentLights.lights[4].position = glm::vec4(0.0f, 0.5f, 0.0f, 0.0f);
    uboFragmentLights.lights[4].color    = glm::vec3(0.0f, 1.0f, 0.2f);
    uboFragmentLights.lights[4].radius   = 5.0f;
    // Yellow
    uboFragmentLights.lights[5].position = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    uboFragmentLights.lights[5].color    = glm::vec3(1.0f, 0.7f, 0.3f);
    uboFragmentLights.lights[5].radius   = 25.0f;

    // Current view position
    uboFragmentLights.viewPos =
      glm::vec4(m_camera.position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

    withMappedMemory(m_uniform_buffers.fsLights_mem, 0, sizeof(uboFragmentLights), [=](void* data) {
        std::memcpy(data, &uboFragmentLights, sizeof(uboFragmentLights));
    });
}


void
renderer::loadAssets()
{
    // TEMPORARY: Camera settings here
    // m_camera.setPosition(glm::vec3(0.f, 1.0f, 5.f));  // Straight ahead view
    m_camera.setPosition(glm::vec3(4.f, 3.0f, 5.3f));  // 3/4 view
    m_camera.setPerspective(60.0f, (float)m_win_width / (float)m_win_height, 0.1f, 256.0f);
    m_camera.matrices.perspective[0][0] *= -1;
    m_camera.matrices.perspective[1][1] *= -1;
    // m_camera.model;
    // m_camera.matrices.view = glm::ortho(-4.0f / 3.0f, 4.0f / 3.0f, -1.0f, 1.0f, -100.0f, 100.0f);
    m_camera.matrices.view =
      glm::lookAt(m_camera.position, glm::vec3(0.f, 1.5f, 0.f), glm::vec3(0.f, 1.f, 0.f));
    /*m_camera.proj =
      glm::perspective(glm::radians(60.0f), m_win_width / (float)m_win_height, 0.1f, 100.0f);
    m_camera.proj[1][1] *= -1;*/

    // Set Lights once at the start
    uboFragmentLights.viewPos =
      glm::vec4(m_camera.position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

    // Load models and Textures
    loadModels(all_model_filenames);
    loadTextures(m_meshes[0], "textures/Imrod_Diffuse.png", "textures/Imrod_norm.png");
    // loadTextures(all_texture_filenames);
}

void
renderer::loadModels(std::vector<std::string> filenames)
{
    // m_mesh = Mesh(triangle_vertices, triangle_indices);
    // Loading OBJ works! woohoo
    // m_mesh = loadObj("models/chalet.obj");

    for (auto filename : filenames) {
        m_meshes.push_back(loadFbx(filename));
    }
}

Mesh
renderer::loadObj(std::string objpath) const
{
    tinyobj::attrib_t                attrib;
    std::vector<tinyobj::shape_t>    shapes;
    std::vector<tinyobj::material_t> materials;
    std::string                      err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, objpath.c_str())) {
        throw std::runtime_error("failed to load Obj!");
    }
    Mesh objMesh;

    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};

            vertex.pos = { attrib.vertices[3 * index.vertex_index + 0],
                           attrib.vertices[3 * index.vertex_index + 1],
                           attrib.vertices[3 * index.vertex_index + 2] };

            vertex.texcoord = { attrib.texcoords[2 * index.texcoord_index + 0],
                                1.0f - attrib.texcoords[2 * index.texcoord_index + 1] };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            vertices.push_back(vertex);
            size_t sze = indices.size();
            indices.push_back(static_cast<uint32_t>(indices.size()));
        }
    }
    return objMesh;
}

Mesh
renderer::loadFbx(std::string fbxpath)
{
    const aiScene* pScene =
      aiImportFile(fbxpath.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);

    if (!pScene) {
        throw std::runtime_error("failed to open fbx!");
    }

    // call parseFBX function or something here to save to my struct
    // TEMP HELPER FUNCTION LOCATION START
    Mesh newMesh;

    std::vector<float>    vertexBuffer;
    std::vector<uint32_t> indexBuffer;

    std::vector<Vertex>   newVertices;
    std::vector<uint32_t> newIndices;

    // int  mesh_count     = pScene->mNumMeshes; TODO: Uncomment this when we load more than the
    // first mesh
    int meshCount   = 1;
    int vertexCount = 0;
    int indexCount  = 0;

    // This for loop isn't actually needed, it's for the eventuality
    // that we load more than one Mesh from an fbx.
    for (int i = 0; i < meshCount; ++i) {
        // Step 1: Fetch all relevant data fields from the parsed file
        const aiMesh* pMesh = pScene->mMeshes[i];
        vertexCount += pMesh->mNumVertices;

        auto vertices = pMesh->mVertices;
        // auto colors   = pMesh->mColors;
        auto normals  = pMesh->mNormals;
        auto tangents = pMesh->mTangents;

        // Get material properties
        aiColor3D pColor(0.f, 0.f, 0.f);
        pScene->mMaterials[pMesh->mMaterialIndex]->Get(AI_MATKEY_COLOR_DIFFUSE, pColor);
        aiString path;
        pScene->mMaterials[pMesh->mMaterialIndex]->GetTexture(aiTextureType_DIFFUSE, 0, &path);
        if (path.length > 0) {
            newMesh.texture_filename = path.C_Str();
        }  // TODO: truncate the path to get just the filename. Maybe other fancy stuff for
           // subdirectories. But for now, I can get the filepath from the fbx.

        const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

        // Step 2: Check availability of info, then set values.
        for (int ii = 0; ii < vertexCount; ++ii) {
            Vertex vertex;
            // Set position of vertex
            vertex.pos = glm::vec3(vertices[ii].x, vertices[ii].y, vertices[ii].z);
            // Set color of vertex
            vertex.color = glm::vec4(pColor.r, pColor.g, pColor.b, 1.0f);
            /*if (pColor == nullptr) {
                vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            } else {
                vertex.color = glm::vec4(pColor.r, pColor.g, pColor.b, 1.0f);
            }*/

            // Set normal of vertex
            if (normals != nullptr) {
                vertex.normal = glm::vec3(normals[ii].x, normals[ii].y, normals[ii].z);
            }
            // Set uv of vertex
            auto uvs = (pMesh->HasTextureCoords(0)) ? &(pMesh->mTextureCoords[0][ii]) : &Zero3D;
            if (uvs != nullptr) {
                // We flip the Y coord because Vulkan origin is in the Top-Left, while Maya is
                // Bottom-Left
                vertex.texcoord = glm::vec2(uvs->x, -uvs->y);
            }

            if (tangents != nullptr) {
                vertex.tangent = glm::vec3(tangents[ii].x, tangents[ii].y, tangents[ii].z);
            } else {
                vertex.tangent = glm::vec3(0.0f);
            }

            // Save the vertex to Mesh
            newVertices.push_back(vertex);
        }

        uint32_t indexBase = static_cast<uint32_t>(newIndices.size());
        for (unsigned int ii = 0; ii < pMesh->mNumFaces; ++ii) {
            const aiFace& Face = pMesh->mFaces[ii];
            if (Face.mNumIndices != 3) {
                continue;
            }
            newIndices.push_back(indexBase + Face.mIndices[0]);
            newIndices.push_back(indexBase + Face.mIndices[1]);
            newIndices.push_back(indexBase + Face.mIndices[2]);
            indexCount += 3;
        }
    }
    // TEMP HELPER FUNCTION LOCATION END

    // Test the three buffer creations here
    newMesh.num_indices  = (uint32_t)newIndices.size();
    newMesh.num_vertices = (uint32_t)newVertices.size();
    createVertexBuffer(newMesh, newVertices);
    createIndexBuffer(newMesh, newIndices);
    createUniformBuffer(newMesh, sizeof(uniformbufferobject));
    newMesh.device = &m_device;

    return newMesh;
}


// COMMENT: Right now, I have the Textures embedded in the Mesh struct.
// This is a terrible idea for many reasons, but eventually what I want to do is to store the
// filename as a key for my texture cache. That way any mesh can switch textures by simply changing
// the string.
void
renderer::loadTextures(std::vector<std::string> filenames)
{
    int i = 0;
    for (auto& mesh : m_meshes) {
        // For now, we are deriving the texture filenames from hardcoded values.
        // I already have filepaths derived from the fbx loading, but they are not usable without
        // some altering. This I will get done overtime.
        createTextureImage(filenames[i], mesh.diffuse_tex);
        createTextureImageView(mesh.diffuse_tex);
        createTextureSampler(mesh.diffuse_tex);
        ++i;
    }
}

void
renderer::loadTextures(Mesh& mesh, std::string diffuseFilename, std::string normalFilename)
{
    // First create the diffuse Texture2D
    mesh.diffuse_tex.device = mesh.device;
    createTextureImage(diffuseFilename, mesh.diffuse_tex);
    createTextureImageView(mesh.diffuse_tex);
    createTextureSampler(mesh.diffuse_tex);

    // Second, create the Normal Texture2D
    mesh.normal_tex.device = mesh.device;
    createTextureImage(normalFilename, mesh.normal_tex);
    createTextureImageView(mesh.normal_tex);
    createTextureSampler(mesh.normal_tex);
}

vk::PipelineShaderStageCreateInfo
renderer::loadShader(const char* entrypoint, std::string fileName, vk::ShaderStageFlagBits stage)
{
    const std::string s = "main";
    const char*       c = s.c_str();

    auto shaderCode  = readFile(fileName);
    auto shaderStage = vk::PipelineShaderStageCreateInfo()
                         .setStage(stage)
                         .setModule(createShaderModule(shaderCode, m_device))
                         .setPName(entrypoint);

    if (!(shaderStage.module)) {
        std::runtime_error("Error creating Shader Module: " + fileName);
    }
    // m_shader_modules.push_back(shaderStage.module);
    return shaderStage;
}

/*
This is a helper function to create a buffer, allocate some memory for it, and bind them
together
*/
std::pair<vk::Buffer, vk::DeviceMemory>
renderer::createBuffer(vk::DeviceSize          size,
                       vk::BufferUsageFlags    usage,
                       vk::MemoryPropertyFlags properties) const
{
    auto bufferinfo =
      vk::BufferCreateInfo()
        // The first field of the struct is size, which specifies the size of the buffer in
        // bytes.
        .setSize(size)
        // The second field is usage, which indicates for which purposes the data in the buffer
        // is going to be used. It is possible to specify multiple purposes using a bitwise or.
        // Our use case will be a vertex buffer
        .setUsage(usage)
        // Just like the images in the swap chain, buffers can also be owned by a specific queue
        // family or be shared between multiple at the same time. The buffer will only be used
        // from the graphics queue, so we can stick to exclusive access.
        .setSharingMode(vk::SharingMode::eExclusive);

    auto buffer = m_device.createBuffer(bufferinfo);

    // The buffer has been created, but it doesn't actually have any memory assigned to it yet.
    // The first step of allocating memory for the buffer is to query its memory requirements
    // using the aptly named vkGetBufferMemoryRequirements function.
    vk::MemoryRequirements memrequirements = m_device.getBufferMemoryRequirements(buffer);

    // The VkMemoryRequirements struct has three fields:
    //  - size: The size of the required amount of memory in bytes, may differ from
    //  bufferInfo.size.
    //  - alignment: The offset in bytes where the buffer begins in the allocated region of
    //  memory,
    // depends on bufferInfo.usage and bufferInfo.flags.
    //  - memoryTypeBits: Bit field of the memory types that are suitable for the buffer.

    auto memallocinfo = vk::MemoryAllocateInfo()
                          .setAllocationSize(memrequirements.size)
                          .setMemoryTypeIndex(findMemoryType(
                            m_physical_device, memrequirements.memoryTypeBits, properties));

    auto buffermemory = m_device.allocateMemory(memallocinfo);

    // The first three parameters are self-explanatory and the fourth parameter is the offset
    // within the region of memory. Since this memory is allocated specifically for this the
    // vertex buffer, the offset is simply 0. If the offset is non-zero, then it is required to
    // be divisible by memRequirements.alignment.
    m_device.bindBufferMemory(buffer, buffermemory, 0);

    return { buffer, buffermemory };
}

std::pair<vk::Image, vk::DeviceMemory>
renderer::createImage(uint32_t                width,
                      uint32_t                height,
                      uint32_t                mipLevels,
                      vk::Format              format,
                      vk::ImageTiling         tiling,
                      vk::ImageUsageFlags     usage,
                      vk::MemoryPropertyFlags properties) const

{
    auto imageinfo =
      vk::ImageCreateInfo()
        // The image type, specified in the imageType field, tells Vulkan with what kind of
        // coordinate system the texels in the image are going to be addressed. It is possible
        // to create 1D, 2D and 3D images. One dimensional images can be used to store an array
        // of data or gradient, two dimensional images are mainly used for textures, and three
        // dimensional images can be used to store voxel volumes, for example.
        .setImageType(vk::ImageType::e2D)
        //  The extent field specifies the dimensions of the image, basically how many texels
        //  there are on each axis. That's why depth must be 1 instead of 0
        .setExtent(vk::Extent3D(width, height, 1))
        .setMipLevels(mipLevels)
        .setArrayLayers(1)
        // Vulkan supports many possible image formats, but we should use the same format for
        // the texels as the pixels in the buffer, otherwise the copy operation will fail.
        .setFormat(format)
        // The tiling field can have one of two values:
        //     VK_IMAGE_TILING_LINEAR: Texels are laid out in row-major order like our pixels
        //     array VK_IMAGE_TILING_OPTIMAL: Texels are laid out in an implementation defined
        //     order for optimal access
        // Unlike the layout of an image, the tiling mode cannot be changed at a later time. If
        // you want to be able to directly access texels in the memory of the image, then you
        // must use VK_IMAGE_TILING_LINEAR. We will be using a staging buffer instead of a
        // staging image, so this won't be necessary. We will be using VK_IMAGE_TILING_OPTIMAL
        // for efficient access from the shader.
        .setTiling(tiling)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        // The usage field has the same semantics as the one during buffer creation. The image
        // is going to be used as destination for the buffer copy, so it should be set up as a
        // transfer destination. We also want to be able to access the image from the shader to
        // color our mesh, so the usage should include VK_IMAGE_USAGE_SAMPLED_BIT.
        .setUsage(usage)
        // The samples flag is related to multisampling. This is only relevant for images that
        // will be used as attachments, so stick to one sample. There are some optional flags
        // for images that are related to sparse images. Sparse images are images where only
        // certain regions are actually backed by memory. If you were using a 3D texture for a
        // voxel terrain, for example, then you could use this to avoid allocating memory to
        // store large volumes of "air" values. We won't be using it in this tutorial, so leave
        // it to its default value of 0.
        .setSamples(vk::SampleCountFlagBits::e1)
        // The image will only be used by one queue family: the one that supports graphics (and
        // therefore also) transfer operations.
        .setSharingMode(vk::SharingMode::eExclusive);

    vk::Image image = m_device.createImage(imageinfo);

    vk::MemoryRequirements memrequirements = m_device.getImageMemoryRequirements(image);

    auto allocinfo = vk::MemoryAllocateInfo()
                       .setAllocationSize(memrequirements.size)
                       .setMemoryTypeIndex(findMemoryType(
                         m_physical_device, memrequirements.memoryTypeBits, properties));

    vk::DeviceMemory memory = m_device.allocateMemory(allocinfo);

    m_device.bindImageMemory(image, memory, 0);

    return { image, memory };
}

void
renderer::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const
{
    if (!buffer || !image)
        return;

    // Just like with buffer copies, you need to specify which part of the buffer is going to be
    // copied to which part of the image. This happens through vk::BufferImageCopy structs:
    // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkBufferImageCopy.html
    executeSingleTimeCommands([=](vk::CommandBuffer commandBuffer) {
        vk::BufferImageCopy region;
        // bufferOffset is the offset in bytes from the start of the buffer object where the
        // image data is copied from or to.
        region
          .setBufferOffset(0)
          // bufferRowLength and bufferImageHeight specify the data in buffer memory as a
          // subregion of a larger two- or three-dimensional image, and control the addressing
          // calculations of data in buffer memory. If either of these values is zero, that
          // aspect of the buffer memory is considered to be tightly packed according to the
          // imageExtent.
          .setBufferImageHeight(0)
          .setBufferRowLength(0)

          // imageSubresource is a VkImageSubresourceLayers used to specify the specific image
          // subresources of the image used for the source or destination image data.
          .imageSubresource
          // aspectMask is a combination of VkImageAspectFlagBits, selecting the color, depth
          // and/or stencil aspects to be copied.
          .setAspectMask(vk::ImageAspectFlagBits::eColor)
          // mipLevel is the mipmap level to copy from.
          .setMipLevel(0)
          // baseArrayLayer and layerCount are the starting layer and number of layers to copy.
          .setBaseArrayLayer(0)
          .setLayerCount(1);
        // imageOffset selects the initial x, y, z offsets in texels of the sub-region of the
        // source or destination image data.
        region.setImageOffset({ 0, 0, 0 });
        // imageExtent is the size in texels of the image to copy in width, height and depth.
        region.setImageExtent({ width, height, 1 });

        commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal,
                                        region);
    });
}

/*
Memory transfer operations are executed using command buffers, just like drawing commands.
Therefore we must first allocate a temporary command buffer. You may wish to create a separate
command pool for these kinds of short-lived buffers, because the implementation may be able to
apply memory allocation optimizations. You should use the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT
flag during command pool generation in that case.
*/
void
renderer::copyBuffer(vk::Buffer src, vk::Buffer* dst, vk::DeviceSize size) const
{
    if (!src || !dst)
        return;

    executeSingleTimeCommands([=](vk::CommandBuffer& commandbuf) {
        auto copyregion = vk::BufferCopy().setSize(size);
        commandbuf.copyBuffer(src, *dst, 1, &copyregion);
    });
}

void
renderer::transitionImageLayout(vk::Image       image,
                                vk::Format      format,
                                vk::ImageLayout oldLayout,
                                vk::ImageLayout newLayout,
                                uint32_t        mipLevels) const
{
    if (!image) {
        return;
    }

    // Begin lambda function
    executeSingleTimeCommands([=](vk::CommandBuffer commandBuffer) {
        // ImageMemoryBarrier struct with constructor argument descriptions in order of
        // appearance. For in depth descriptions of the purposes of each one, go to this web
        // address:
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkImageMemoryBarrier.html
        // 1) srcAccessMask is a bitmask of VkAccessFlagBits specifying a source access mask.
        // 2) dstAccessMask is a bitmask of VkAccessFlagBits specifying a destination access
        // mask. 3) oldLayout is the old layout in an image layout transition. 4) newLayout is
        // the new layout in an image layout transition. 5) srcQueueFamilyIndex is the source
        // queue family for a queue family ownership transfer. 6) dstQueueFamilyIndex is the
        // destination queue family for a queue family ownership
        //    transfer.
        // 7) image is a handle to the image affected by this barrier.
        // 8) subresourceRange describes the image subresource range within image that is
        // affected
        //    by this barrier.
        vk::ImageMemoryBarrier barrier(
          vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eTransferRead,
          vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eTransferSrcOptimal, 0, 0, image);
        // Initial Barrier settings
        barrier.setOldLayout(oldLayout)
          .setNewLayout(newLayout)
          .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
          .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
          .setImage(image);
        // subresourceRange stuff, very similar to the Buffer to Image stuff
        if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eDepth);
            if (hasStencilComponent(format)) {
                barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
            }
        } else {
            barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
        }
        barrier.subresourceRange.setBaseMipLevel(0)
          .setLevelCount(mipLevels)
          .setBaseArrayLayer(0)
          .setLayerCount(1);

        vk::PipelineStageFlags sourceStage;
        vk::PipelineStageFlags destStage;

        // We perform layout checks in order to derive which transition we are making.
        // For now, we are only considering two transitions:
        // e.g.          oldLayout -> newLayout
        // 1)            Undefined -> transfer destination
        // 2) Transfer destination -> shader reading
        // Later on, we can expand these condition checks, though I'm not sure how to make this
        // more sophisticated. - <Jason>
        /*The depth buffer will be read from to perform depth tests to see if a fragment is
         * visible, and will be written to when a new fragment is drawn. The reading happens in
         * the VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT stage and the writing in the
         * VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT. You should pick the earliest pipeline
         * stage that matches the specified operations, so that it is ready for usage as depth
         * attachment when it needs to be.*/
        if (oldLayout == vk::ImageLayout::eUndefined
            && newLayout == vk::ImageLayout::eTransferDstOptimal) {
            barrier.setSrcAccessMask(vk::AccessFlagBits());
            barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destStage   = vk::PipelineStageFlagBits::eTransfer;
        } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal
                   && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
            barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
            barrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
            sourceStage = vk::PipelineStageFlagBits::eTransfer;
            destStage   = vk::PipelineStageFlagBits::eFragmentShader;
        } else if (oldLayout == vk::ImageLayout::eUndefined
                   && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
            barrier.setSrcAccessMask(vk::AccessFlagBits());
            barrier.setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentRead
                                     | vk::AccessFlagBits::eDepthStencilAttachmentWrite);
            sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
            destStage   = vk::PipelineStageFlagBits::eEarlyFragmentTests;
        } else {
            throw std::invalid_argument("Unsupported layout transition!");
        }

        commandBuffer.pipelineBarrier(sourceStage, destStage, vk::DependencyFlagBits::eByRegion,
                                      (uint32_t)0, nullptr, (uint32_t)0, nullptr, 1, &barrier);
    });
}

vk::ImageView
renderer::createImageView(vk::Image               image,
                          vk::Format              format,
                          vk::ImageAspectFlagBits aspectflags,
                          uint32_t                mipLevels) const
{
    auto createinfo = vk::ImageViewCreateInfo()
                        .setImage(image)
                        .setViewType(vk::ImageViewType::e2D)
                        .setFormat(format)
                        .setComponents(vk::ComponentMapping()
                                         .setR(vk::ComponentSwizzle::eIdentity)
                                         .setG(vk::ComponentSwizzle::eIdentity)
                                         .setB(vk::ComponentSwizzle::eIdentity)
                                         .setA(vk::ComponentSwizzle::eIdentity))
                        .setSubresourceRange(vk::ImageSubresourceRange()
                                               .setAspectMask(aspectflags)
                                               .setBaseMipLevel(0)
                                               .setLevelCount(mipLevels)
                                               .setBaseArrayLayer(0)
                                               .setLayerCount(1));

    vk::ImageView imageView = m_device.createImageView(createinfo, nullptr);
    if (!imageView) {
        throw std::runtime_error("failed to create texture image view!");
    }

    return imageView;
}

void
renderer::createAttachment(vk::Format             format,
                           vk::ImageUsageFlagBits usage,
                           FrameBufferAttachment* attachment)
{
    vk::ImageAspectFlags aspectMask;
    vk::ImageLayout      imageLayout;

    attachment->format = format;

    if (usage == vk::ImageUsageFlagBits::eColorAttachment) {
        aspectMask  = vk::ImageAspectFlagBits::eColor;
        imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
    }
    if (usage == vk::ImageUsageFlagBits::eDepthStencilAttachment) {
        aspectMask  = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }

    vk::ImageCreateInfo image =
      vk::ImageCreateInfo()
        .setImageType(vk::ImageType::e2D)
        .setFormat(format)
        .setExtent(vk::Extent3D(m_offscreen_framebuffer.width, m_offscreen_framebuffer.height, 1))
        .setMipLevels(1)
        .setArrayLayers(1)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(usage | vk::ImageUsageFlagBits::eSampled);

    vk::MemoryAllocateInfo memAlloc = vk::MemoryAllocateInfo();
    vk::MemoryRequirements memReqs;

    m_device.createImage(&image, nullptr, &attachment->image);
    m_device.getImageMemoryRequirements(attachment->image, &memReqs);
    memAlloc.setAllocationSize(memReqs.size);
    memAlloc.setMemoryTypeIndex(
      getMemoryType(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    m_device.allocateMemory(&memAlloc, nullptr, &attachment->memory);
    m_device.bindImageMemory(attachment->image, attachment->memory, 0);

    // Create Image View
    auto imageView = vk::ImageViewCreateInfo()
                       .setFormat(format)
                       .setImage(attachment->image)
                       .setViewType(vk::ImageViewType::e2D);
    imageView.subresourceRange.setAspectMask(aspectMask)
      .setBaseMipLevel(0)
      .setLevelCount(1)
      .setBaseArrayLayer(0)
      .setLayerCount(1);

    m_device.createImageView(&imageView, nullptr, &attachment->image_view);
}

void
renderer::generateMipmaps(vk::Image  image,
                          vk::Format imageFormat,
                          int32_t    tex_width,
                          int32_t    tex_height,
                          uint32_t   mipLevels)
{
    executeSingleTimeCommands([=](vk::CommandBuffer commandBuffer) {
        auto barrier = vk::ImageMemoryBarrier()
                         .setImage(image)
                         .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                         .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        barrier.subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor)
          .setBaseArrayLayer(0)
          .setLayerCount(1)
          .setLevelCount(1);

        int32_t mipWidth  = tex_width;
        int32_t mipHeight = tex_height;

        for (uint32_t i = 1; i < mipLevels; ++i) {
            barrier.subresourceRange.setBaseMipLevel(i - 1);
            barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
              .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
              .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
              .setDstAccessMask(vk::AccessFlagBits::eTransferRead);

            commandBuffer.pipelineBarrier(
              vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
              vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &barrier);

            auto blit          = vk::ImageBlit();
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
              .setMipLevel(i - 1)
              .setBaseArrayLayer(0)
              .setLayerCount(1);
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1,
                                   mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
              .setMipLevel(i)
              .setBaseArrayLayer(0)
              .setLayerCount(1);

            commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, image,
                                    vk::ImageLayout::eTransferDstOptimal, 1, &blit,
                                    vk::Filter::eLinear);

            barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
              .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
              .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
              .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

            commandBuffer.pipelineBarrier(
              vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
              vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 1, &barrier);

            if (mipWidth > 1)
                mipWidth /= 2;
            if (mipHeight > 1)
                mipHeight /= 2;
        }
    });
}

/* This function allows us to define our own list of prioritized formats, and will return the
 * first successful find based on the list of candidates. */
vk::Format
renderer::findSupportedFormat(const std::vector<vk::Format>& candidates,
                              vk::ImageTiling                tiling,
                              vk::FormatFeatureFlagBits      features) const
{
    for (vk::Format format : candidates) {
        vk::FormatProperties properties = m_physical_device.getFormatProperties(format);

        if (tiling == vk::ImageTiling::eLinear
            && (properties.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal
                   && (properties.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    throw std::runtime_error("failed to find supported format!");
}

vk::Bool32
renderer::getSupportedDepthFormat(vk::Format* depthFormat)
{
    // Since all depth formats may be optional, we need to find a suitable depth format to use
    // Start with the highest precision packed format
    std::vector<vk::Format> depthFormats = { vk::Format::eD32SfloatS8Uint, vk::Format::eD32Sfloat,
                                             vk::Format::eD24UnormS8Uint,
                                             vk::Format::eD16UnormS8Uint, vk::Format::eD16Unorm };

    for (auto& format : depthFormats) {
        vk::FormatProperties formatProps = m_physical_device.getFormatProperties(format);
        // Format must support depth stencil attachment for optimal tiling
        if (formatProps.optimalTilingFeatures
            & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            *depthFormat = format;
            return true;
        }
    }

    return false;
}

uint32_t
renderer::getMemoryType(uint32_t                typeBits,
                        vk::MemoryPropertyFlags properties,
                        vk::Bool32*             memTypeFound)
{
    for (uint32_t i = 0; i < m_device_memory_properties.memoryTypeCount; ++i) {
        if ((typeBits & 1) == 1) {
            if ((m_device_memory_properties.memoryTypes[i].propertyFlags & properties)
                == properties) {
                if (memTypeFound) {
                    *memTypeFound = true;
                }
                return i;
            }
        }
        typeBits >>= 1;
    }
    // If we reach this point, no matching memory type was found
    if (memTypeFound) {
        *memTypeFound = false;
        return 0;
    } else {
        throw std::runtime_error("Could not find a matching memory type");
    }
}

void
renderer::findEnabledFeatures()
{
    // Check for Anisotropic filtering
    if (m_device_features.samplerAnisotropy) {
        m_enabled_features.setSamplerAnisotropy(true);
    }
    // Check for texture compression
    if (m_device_features.textureCompressionBC) {
        m_enabled_features.setTextureCompressionBC(true);
    } else if (m_device_features.textureCompressionASTC_LDR) {
        m_enabled_features.setTextureCompressionASTC_LDR(true);
    } else if (m_device_features.textureCompressionETC2) {
        m_enabled_features.setTextureCompressionETC2(true);
    }

    // Geometry shading
    if (m_device_features.geometryShader) {
        m_enabled_features.setGeometryShader(true);
    } else {
        // throw std::runtime_error("Selected GPU does not support geometry shader!\n");
    }

    if (m_device_features.multiViewport) {
        m_enabled_features.setMultiViewport(true);
    } else {
        // throw std::runtime_error("Selected GPU does not support multi viewports!\n");
    }
}

/*
The application we have now successfully draws a triangle, but there are some circumstances that
it isn't handling properly yet. It is possible for the window surface to change such that the
swap chain is no longer compatible with it. One of the reasons that could cause this to happen
is the size of the window changing. We have to catch these events and recreate the swap chain.
*/
void
renderer::recreateSwapChain()
{
    m_device.waitIdle();

    // glfwGetFramebufferSize(m_window, &m_win_width, &m_win_height);
    cleanupSwapChain();

    createSwapChain();
    // createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createDepthResources();
    createFramebuffers();
    createCommandBuffers();
}

/*
refer to renderer::recreateSwapChain()
*/
void
renderer::cleanupSwapChain()
{
    // Cleanup Swapchain Imageviews
    if (m_swapchain_struct.swapchain) {
        for (auto& buffer : m_swapchain_struct.buffers) {
            m_device.destroyImageView(buffer.view);
        }
    }

    // Cleanup Surface
    if (m_surface) {
        m_device.destroySwapchainKHR(m_swapchain_struct.swapchain);
        m_instance.destroySurfaceKHR(m_surface);
    }

    // Set both to VK_NULL_HANDLE
    // m_surface = VK_NULL_HANDLE;

    /*
// FrameBuffer
m_device.destroyFramebuffer(m_offscreen_framebuffer.frameBuffer);
// Depth
m_device.destroyImageView(m_offscreen_framebuffer.depth.image_view);
m_device.destroyImage(m_offscreen_framebuffer.depth.image);
m_device.freeMemory(m_offscreen_framebuffer.depth.memory);
// Albedo
m_device.destroyImageView(m_offscreen_framebuffer.albedo.image_view);
m_device.destroyImage(m_offscreen_framebuffer.albedo.image);
m_device.freeMemory(m_offscreen_framebuffer.albedo.memory);
// Normal
m_device.destroyImageView(m_offscreen_framebuffer.normal.image_view);
m_device.destroyImage(m_offscreen_framebuffer.normal.image);
m_device.freeMemory(m_offscreen_framebuffer.normal.memory);
// Position
m_device.destroyImageView(m_offscreen_framebuffer.position.image_view);
m_device.destroyImage(m_offscreen_framebuffer.position.image);
m_device.freeMemory(m_offscreen_framebuffer.position.memory);

for (auto& buffer : m_swapchain_struct.buffers) {
    m_device.destroyImage(buffer.image);
    m_device.destroyImageView(buffer.view);
}

m_device.freeCommandBuffers(m_command_pool, (uint32_t)m_command_buffers.size(),
                            m_command_buffers.data());

m_device.destroyPipelineCache(m_pipeline_cache);
m_device.destroyPipeline(m_graphics_pipelines.deferred);
m_device.destroyPipeline(m_graphics_pipelines.offscreen);
m_device.destroyPipelineLayout(m_pipeline_layouts.deferred);
m_device.destroyPipelineLayout(m_pipeline_layouts.offscreen);
m_device.destroyRenderPass(m_render_pass);

m_device.destroySwapchainKHR(m_swapchain_struct.swapchain);*/
}

vk::Extent2D
renderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);

        vk::Extent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };

        actualExtent.width =
          std::max(capabilities.minImageExtent.width,
                   std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height =
          std::max(capabilities.minImageExtent.height,
                   std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

vk::CompositeAlphaFlagBitsKHR
renderer::chooseSwapAlpha(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

    // Ordered list of composite alpha formats. We want the first one available.
    std::vector<vk::CompositeAlphaFlagBitsKHR> compositeAlphaFlags = {
        vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
        vk::CompositeAlphaFlagBitsKHR::ePostMultiplied, vk::CompositeAlphaFlagBitsKHR::eInherit
    };
    for (auto& compositeAlphaFlag : compositeAlphaFlags) {
        if (capabilities.supportedCompositeAlpha & compositeAlphaFlag) {
            compositeAlpha = compositeAlphaFlag;
            break;
        }
    }

    return compositeAlpha;
}

void
renderer::initVulkan()
{
    // Base Vulkan initialization
    createInstance();
    setupDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createCommandPool();
    createSwapChain();
    // createImageViews(); // NOTE: This function is now part of createSwapChain
    createCommandBuffers();
    createSemaphores();
    createFences();
    createDepthResources();
    createRenderPass();
    createPipelineCache();
    createFramebuffers();
}

void
renderer::prepare()
{
    // This is all called after vulkan is initialized
    loadAssets();
    generateQuads();
    prepareOffscreenFramebuffer();
    prepareUniformBuffers();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    // Setup Descriptors
    createDescriptorPool();
    createDescriptorSet();
    // Build Command Buffers
    buildCommandBuffers();
    buildDeferredCommandBuffer();
}

void
renderer::mainLoop()
{
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        drawFrame();
        updateUniformBuffer();
        // updateUniformBuffersScreen();
        updateUniformBufferDeferredMatrices();
        updateUniformBufferDeferredLights();
    }

    m_device.waitIdle();
}

void
renderer::cleanup()
{
    // TODO: Clean up everything else first

    // Samplers
    m_device.destroySampler(m_color_sampler);

    // Cleanup attachment stuff in this order
    // ImageViews -> Images -> Memory

    // Cleanup Position Framebuffer attachment
    m_device.destroyImageView(m_offscreen_framebuffer.position.image_view);
    m_device.destroyImage(m_offscreen_framebuffer.position.image);
    m_device.freeMemory(m_offscreen_framebuffer.position.memory);

    // Cleanup Normal Framebuffer attachment
    m_device.destroyImageView(m_offscreen_framebuffer.normal.image_view);
    m_device.destroyImage(m_offscreen_framebuffer.normal.image);
    m_device.freeMemory(m_offscreen_framebuffer.normal.memory);

    // Cleanup Albedo Framebuffer attachment
    m_device.destroyImageView(m_offscreen_framebuffer.albedo.image_view);
    m_device.destroyImage(m_offscreen_framebuffer.albedo.image);
    m_device.freeMemory(m_offscreen_framebuffer.albedo.memory);

    // Cleanup Depth Framebuffer attachment
    m_device.destroyImageView(m_offscreen_framebuffer.depth.image_view);
    m_device.destroyImage(m_offscreen_framebuffer.depth.image);
    m_device.freeMemory(m_offscreen_framebuffer.depth.memory);

    // Cleanup Offscreen Framebuffer
    m_device.destroyFramebuffer(m_offscreen_framebuffer.frameBuffer);

    // Destroy pipelines
    m_device.destroyPipeline(m_graphics_pipelines.deferred);
    m_device.destroyPipeline(m_graphics_pipelines.offscreen);
    m_device.destroyPipeline(m_graphics_pipelines.debug);

    // Destroy Pipeline Layouts
    m_device.destroyPipelineLayout(m_pipeline_layouts.deferred);
    m_device.destroyPipelineLayout(m_pipeline_layouts.offscreen);

    // Destroy Meshes
    for (auto& mesh : m_meshes) {
        mesh.destroy(m_device);
    }

    // Destroy UBOs

    // Free command buffers
    m_device.freeCommandBuffers(m_command_pool, 1, &m_offscreen_commandbuffer);

    // Destroy Render Pass

    // Destroy Mesh Textures

    // Destroy Offscreen semaphore
    m_device.destroySemaphore(m_offscreen_semaphore);

    // Cleanup the SwapChain
    cleanupSwapChain();

    // Destroy Descriptor Pool
    if (m_descriptor_pool) {
        m_device.destroyDescriptorPool(m_descriptor_pool);
    }

    // Free Command Buffers
    m_device.freeCommandBuffers(m_command_pool, static_cast<uint32_t>(m_command_buffers.size()),
                                m_command_buffers.data());

    // Destroy Renderpass
    m_device.destroyRenderPass(m_render_pass);

    // Destroy FrameBuffers
    for (uint32_t i = 0; i < m_framebuffers.size(); ++i) {
        m_device.destroyFramebuffer(m_framebuffers[i]);
    }

    // Destroy shader modules
    for (auto& shaderModule : m_shader_modules) {
        m_device.destroyShaderModule(shaderModule);
    }

    // TODO: Destroy Depthstencil

    // Destroy PipelineCache
    m_device.destroyPipelineCache(m_pipeline_cache);

    // Destroy Command Pool
    m_device.destroyCommandPool(m_command_pool);

    // TODO: Clean up Descriptor Set Layout

    // Clean up uniformbuffers?

    // Clean up Meshes (index and vertex buffers and memory)

    // Clean up Semaphores, and Fences
    for (auto& fence : m_wait_fences) {
        m_device.destroyFence(fence);
    }

    for (auto& semaphore : m_image_available_semaphores) {
        m_device.destroySemaphore(semaphore);
    }

    for (auto& semaphore : m_render_finished_semaphores) {
        m_device.destroySemaphore(semaphore);
    }

    m_device.destroyDescriptorSetLayout(m_descriptor_set_layout);
    // command buffers are implicitly deleted when their command pool is deleted
    // m_device.destroyCommandPool(m_command_pool);

    m_device.destroy();

    if (enableValidationLayers) {
        DestroyDebugReportCallbackEXT(m_instance, m_callback);
    }

    // vk::surfaceKHR objects do not have a destroy()
    // https://github.com/KhronosGroup/Vulkan-Hpp/issues/204
    // m_instance.destroySurfaceKHR(m_surface);
    m_instance.destroy();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

void
renderer::prepareFrame()
{
    auto result = m_device.acquireNextImageKHR(m_swapchain_struct.swapchain,
                                               std::numeric_limits<uint64_t>::max(),
                                               m_present_complete, nullptr, &m_current_frame);

    if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
        recreateSwapChain();
    } else {
        std::runtime_error("Invalid Image result!\n");
    }
}

void
renderer::submitFrame()
{
    // std::array<vk::Result, 5> sc_results;
    auto presentInfo = vk::PresentInfoKHR()
                         .setPNext(NULL)
                         .setSwapchainCount(1)
                         .setPSwapchains(&m_swapchain_struct.swapchain)
                         .setPImageIndices(&m_current_frame);
    //.setPResults(sc_results.data());

    // Check if a wait semaphore has been specified to wait for before presenting the image
    if (m_render_complete) {
        presentInfo.setPWaitSemaphores(&m_render_complete).setWaitSemaphoreCount(1);
    }

    auto result = m_presentation_queue.presentKHR(&presentInfo);
    if (!((result == vk::Result::eSuccess || (result == vk::Result::eSuboptimalKHR)))) {
        if (result == vk::Result::eErrorOutOfDateKHR) {
            // window resize();
            return;
        }
    }
    m_presentation_queue.waitIdle();
}

}  // namespace shiny
