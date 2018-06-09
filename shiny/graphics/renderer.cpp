/*
Brief overview of the graphics pipeline

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
#include "graphics/renderer.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <vector>

#define UNREFERENCED_PARAMETER(P) (P)

namespace {

const uint32_t WIDTH  = 800;
const uint32_t HEIGHT = 600;

using VulkanExtensionName = const char*;
using VulkanLayerName     = const char*;

const std::vector<VulkanLayerName> validationLayers = { "VK_LAYER_LUNARG_standard_validation" };

const std::vector<VulkanExtensionName> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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

VkResult
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
        return result;
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
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

    return extensionsSupported && queueFamilyComplete && swapChainAdequate;
}

/*
if the swapChainAdequate conditions were met then the support is definitely sufficient, but there
may still be many different modes of varying optimality. We'll now write a couple of functions to
find the right settings for the best possible swap chain. There are three types of settings to
determine:

    - Surface format (color depth)
    - Presentation mode (conditions for "swapping" images to the screen)
    - Swap extent (resolution of images in swap chain)
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
*/
vk::Extent2D
chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    vk::Extent2D actual_extent(
      std::clamp(WIDTH, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
      std::clamp(HEIGHT, capabilities.minImageExtent.height, capabilities.maxImageExtent.height));

    return actual_extent;
}

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
createShaderModule(const shiny::graphics::SpirvBytecode& code, const vk::Device& device)
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
shiny::graphics::SpirvBytecode
readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for reading!");
    }

    size_t            filesize = (size_t)file.tellg();
    std::vector<char> buffer(filesize);

    file.seekg(0);
    file.read(buffer.data(), filesize);

    return shiny::graphics::SpirvBytecode(buffer);
}

}  // namespace

namespace shiny::graphics {

void
renderer::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void
renderer::initWindow()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
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
                     .setPApplicationName("Hello Triangle")
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
                        .setFlags(DebugFlags::eError | DebugFlags::eWarning);

    if (CreateDebugReportCallbackEXT(m_instance, createinfo, m_callback) != VK_SUCCESS) {
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

    auto devicefeatures = vk::PhysicalDeviceFeatures();

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
    SwapChainSupportDetails support = querySwapChainSupport(m_physical_device, m_surface);

    vk::SurfaceFormatKHR surfaceformat = chooseSwapSurfaceFormat(support.formats);
    vk::PresentModeKHR   presentmode   = chooseSwapPresentMode(support.presentModes);
    vk::Extent2D         extent        = chooseSwapExtent(support.capabilities);

    uint32_t imagecount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imagecount > support.capabilities.maxImageCount) {
        imagecount = support.capabilities.maxImageCount;
    }

    auto createinfo = vk::SwapchainCreateInfoKHR()
                        .setSurface(m_surface)
                        .setMinImageCount(imagecount)
                        .setImageFormat(surfaceformat.format)
                        .setImageColorSpace(surfaceformat.colorSpace)
                        .setImageExtent(extent)
                        .setImageArrayLayers(1)
                        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

    QueueFamilyIndices indices = findQueueFamilies(m_physical_device, m_surface);

    if (indices.graphicsFamily() != indices.presentFamily()) {
        std::array<uint32_t, 2> queuefamilyindices = { (uint32_t)indices.graphicsFamily(),
                                                       (uint32_t)indices.presentFamily() };
        createinfo.setImageSharingMode(vk::SharingMode::eConcurrent)
          .setQueueFamilyIndexCount(2)
          .setPQueueFamilyIndices(queuefamilyindices.data());
    } else {
        createinfo.setImageSharingMode(vk::SharingMode::eExclusive);
    }

    createinfo.setPreTransform(support.capabilities.currentTransform)
      .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
      .setPresentMode(presentmode)
      .setClipped(true)
      .setOldSwapchain(nullptr);

    // m_swapchain.reset(m_device->createSwapchainKHR(createinfo));

    m_swapchain = m_device.createSwapchainKHR(createinfo);

    {
        // auto images = m_device->getSwapchainImagesKHR(m_swapchain.get());
        auto images = m_device.getSwapchainImagesKHR(m_swapchain);
        m_swapchain_images.reserve(images.size());

        for (const auto& image : images) {
            m_swapchain_images.emplace_back(image);
        }
    }

    m_swapchain_image_format = surfaceformat.format;
    m_swapchain_extent       = extent;
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
    m_swapchain_image_views.reserve(m_swapchain_images.size());

    for (const vk::Image& image : m_swapchain_images) {
        auto createinfo = vk::ImageViewCreateInfo()
                            .setImage(image)
                            .setViewType(vk::ImageViewType::e2D)
                            .setFormat(m_swapchain_image_format)
                            .setComponents(vk::ComponentMapping()
                                             .setR(vk::ComponentSwizzle::eIdentity)
                                             .setG(vk::ComponentSwizzle::eIdentity)
                                             .setB(vk::ComponentSwizzle::eIdentity)
                                             .setA(vk::ComponentSwizzle::eIdentity))
                            .setSubresourceRange(vk::ImageSubresourceRange()
                                                   .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                   .setBaseMipLevel(0)
                                                   .setLevelCount(1)
                                                   .setBaseArrayLayer(0)
                                                   .setLayerCount(1));

        m_swapchain_image_views.emplace_back(m_device.createImageView(createinfo));
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
        .setFormat(m_swapchain_image_format)
        // we're not doing anything with multisampling
        .setSamples(vk::SampleCountFlagBits::e1)
        // The loadOp determines what to do with the data in the attachment before rendering
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        // The storeOp determines what to do with the data in the attachment after
        // rendering.
        .setStoreOp(vk::AttachmentStoreOp::eStore)
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
                     .setPColorAttachments(&colorattachmentref);

    auto renderpasscreateinfo = vk::RenderPassCreateInfo()
                                  .setAttachmentCount(1)
                                  .setPAttachments(&colorattachment)
                                  .setSubpassCount(1)
                                  .setPSubpasses(&subpass);

    m_render_pass = m_device.createRenderPass(renderpasscreateinfo);
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
    auto vertshadercode = readFile("shaders/vert.spv");
    auto fragshadercode = readFile("shaders/frag.spv");

    m_vertex_shader_module   = createShaderModule(vertshadercode, m_device);
    m_fragment_shader_module = createShaderModule(fragshadercode, m_device);

    auto vertex_stagecreateinfo =
      vk::PipelineShaderStageCreateInfo()
        .setStage(vk::ShaderStageFlagBits::eVertex)
        .setModule(m_vertex_shader_module)
        .setPName("main");  // this is the main entry point of the shader

    auto fragment_stagecreateinfo =
      vk::PipelineShaderStageCreateInfo()
        .setStage(vk::ShaderStageFlagBits::eFragment)
        .setModule(m_fragment_shader_module)
        .setPName("main");  // this is the main entry point of the shader

    auto shaderstages =
      std::array<vk::PipelineShaderStageCreateInfo, 2>{ vertex_stagecreateinfo,
                                                        fragment_stagecreateinfo };

    // vertex input
    // The VkPipelineVertexInputStateCreateInfo structure describes the format of the vertex data
    // that will be passed to the vertex shader. It describes this in roughly two ways:
    //  - Bindings: spacing between data and whether the data is per-vertex or per-instance (see
    //    instancing)
    //  - Attribute descriptions: type of the attributes passed to the vertex shader,
    //    which binding to load them from and at which offset
    auto vertexinputinfo = vk::PipelineVertexInputStateCreateInfo();

    // The VkPipelineInputAssemblyStateCreateInfo struct describes two things: what kind of geometry
    // will be drawn from the vertices and if primitive restart should be enabled.
    auto inputassembly = vk::PipelineInputAssemblyStateCreateInfo()
                           .setTopology(vk::PrimitiveTopology::eTriangleList)
                           .setPrimitiveRestartEnable(false);

    // A viewport basically describes the region of the framebuffer that the output will be rendered
    // to. This will almost always be (0, 0) to (width, height)
    auto viewport = vk::Viewport()
                      .setX(0.f)
                      .setY(0.f)
                      .setWidth((float)m_swapchain_extent.width)
                      .setHeight((float)m_swapchain_extent.height)
                      .setMinDepth(0.f)
                      .setMaxDepth(1.f);

    // While viewports define the transformation from the image to the framebuffer, scissor
    // rectangles define in which regions pixels will actually be stored. Any pixels outside the
    // scissor rectangles will be discarded by the rasterizer. They function like a filter rather
    // than a transformation.
    auto scissor = vk::Rect2D{ vk::Offset2D{ 0, 0 }, m_swapchain_extent };

    // Now this viewport and scissor rectangle need to be combined into a viewport state using the
    // VkPipelineViewportStateCreateInfo struct. It is possible to use multiple viewports and
    // scissor rectangles on some graphics cards, so its members reference an array of them. Using
    // multiple requires enabling a GPU feature (see logical device creation).
    auto viewportstate = vk::PipelineViewportStateCreateInfo()
                           .setViewportCount(1)
                           .setPViewports(&viewport)
                           .setScissorCount(1)
                           .setPScissors(&scissor);

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

    // The VkPipelineMultisampleStateCreateInfo struct configures multisampling, which is one of the
    // ways to perform anti-aliasing. It works by combining the fragment shader results of multiple
    // polygons that rasterize to the same pixel. This mainly occurs along edges, which is also
    // where the most noticeable aliasing artifacts occur. Because it doesn't need to run the
    // fragment shader multiple times if only one polygon maps to a pixel, it is significantly less
    // expensive than simply rendering to a higher resolution and then downscaling. Enabling it
    // requires enabling a GPU feature (We don't).
    auto multisampling = vk::PipelineMultisampleStateCreateInfo()
                           .setSampleShadingEnable(false)
                           .setRasterizationSamples(vk::SampleCountFlagBits::e1);

    // After a fragment shader has returned a color, it needs to be combined with the color that is
    // already in the framebuffer. This transformation is known as color blending and there are two
    // ways to do it:
    //  - Mix the old and new value to produce a final color
    //  - Combine the old and new value using a bitwise operation

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
                           .setLogicOpEnable(false)
                           .setAttachmentCount(1)
                           .setPAttachments(&colorblendattachment);

    // A limited amount of the state that we've specified in the previous structs can actually be
    // changed without recreating the pipeline. Examples are the size of the viewport, line width
    // and blend constants. If you want to do that, then you'll have to fill in a
    // VkPipelineDynamicStateCreateInfo
    // This will cause the configuration of these values to be ignored and you will be required to
    // specify the data at drawing time.

    // You can use uniform values in shaders, which are globals similar to dynamic state variables
    // that can be changed at drawing time to alter the behavior of your shaders without having to
    // recreate them. They are commonly used to pass the transformation matrix to the vertex shader,
    // or to create texture samplers in the fragment shader.
    // These uniform values need to be specified during pipeline creation by creating a
    // VkPipelineLayout object.
    auto pipelinelayout = vk::PipelineLayoutCreateInfo();

    m_pipeline_layout = m_device.createPipelineLayout(pipelinelayout);

    auto pipelinecreateinfo =
      vk::GraphicsPipelineCreateInfo()
        .setStageCount(2)
        .setPStages(shaderstages.data())
        .setPVertexInputState(&vertexinputinfo)
        .setPInputAssemblyState(&inputassembly)
        .setPViewportState(&viewportstate)
        .setPRasterizationState(&rasterizer)
        .setPMultisampleState(&multisampling)
        .setPColorBlendState(&colorblending)
        .setLayout(m_pipeline_layout)
        // And finally we have the reference to the render pass. It is also possible to use other
        // render passes with this pipeline instead of this specific instance, but they have to be
        // compatible with renderPass. The requirements for compatibility are described
        // https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#renderpass-compatibility
        .setRenderPass(m_render_pass)
        // The index of the sub pass where this graphics pipeline will be used.
        .setSubpass(0);

    // We pass a nullptr into the argument that requests for a vk::PipelineCache, since we don't
    // need one
    m_graphics_pipeline = m_device.createGraphicsPipeline(nullptr, pipelinecreateinfo);
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
    m_swapchain_framebuffers.reserve(m_swapchain_image_views.size());

    for (const auto& view : m_swapchain_image_views) {
        auto framebufferinfo = vk::FramebufferCreateInfo()
                                 .setRenderPass(m_render_pass)
                                 // The attachmentCount and pAttachments parameters specify the
                                 // VkImageView objects that should be bound to the respective
                                 // attachment descriptions in the render pass pAttachment array.
                                 .setAttachmentCount(1)
                                 .setPAttachments(&view)
                                 .setWidth(m_swapchain_extent.width)
                                 .setHeight(m_swapchain_extent.height)
                                 // layers refers to the number of layers in image arrays. Our swap
                                 // chain images are single images, so the number of layers is 1.
                                 .setLayers(1);

        auto framebuffer = m_device.createFramebuffer(framebufferinfo);
        m_swapchain_framebuffers.emplace_back(framebuffer);
    }
}

/*
Commands in Vulkan, like drawing operations and memory transfers, are not executed directly using
function calls. You have to record all of the operations you want to perform in command buffer
objects. The advantage of this is that all of the hard work of setting up the drawing commands can
be done in advance and in multiple threads. After that, you just have to tell Vulkan to execute the
commands in the main loop.

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
        .setQueueFamilyIndex(indices.graphicsFamily());
}

void
renderer::initVulkan()
{
    createInstance();
    setupDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
}

void
renderer::mainLoop()
{
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
    }
}

void
renderer::cleanup()
{
    for (auto& framebuffer : m_swapchain_framebuffers) {
        m_device.destroyFramebuffer(framebuffer);
    }
    for (auto& imageview : m_swapchain_image_views) {
        m_device.destroyImageView(imageview);
    }

    m_device.destroyPipeline(m_graphics_pipeline);
    m_device.destroyPipelineLayout(m_pipeline_layout);
    m_device.destroyRenderPass(m_render_pass);

    m_device.destroySwapchainKHR(m_swapchain);

    m_device.destroyShaderModule(m_vertex_shader_module);
    m_device.destroyShaderModule(m_fragment_shader_module);

    m_device.destroy();

    if (enableValidationLayers) {
        DestroyDebugReportCallbackEXT(m_instance, m_callback);
    }
    // vk::surfaceKHR objects do not have a destroy()
    // https://github.com/KhronosGroup/Vulkan-Hpp/issues/204
    m_instance.destroySurfaceKHR(m_surface);
    m_instance.destroy();

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

}  // namespace shiny::graphics
