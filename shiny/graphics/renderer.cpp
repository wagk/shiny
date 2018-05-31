#include "graphics/renderer.h"

#include <cstring>
#include <iostream>
#include <set>
#include <vector>

#define UNREFERENCED_PARAMETER(P) (P)

namespace {

const int WIDTH  = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = { "VK_LAYER_LUNARG_standard_validation" };

const std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

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
    return vk::PresentModeKHR::eFifo;
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

std::vector<const char*>
getRequiredExtensions()
{
    uint32_t     glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
    }

    return extensions;
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

    uint32_t     glfwExtensionCount = 0;
    const char** glfwExtensions     = nullptr;

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

    m_instance.reset(vk::createInstance(createinfo, nullptr));

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

    if (CreateDebugReportCallbackEXT(m_instance.get(), createinfo, m_callback) != VK_SUCCESS) {
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
    if (glfwCreateWindowSurface(m_instance.get(), m_window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
    m_surface.reset(surface);
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
    std::vector<vk::PhysicalDevice> physical_devices = m_instance->enumeratePhysicalDevices();

    if (physical_devices.empty())
        throw std::runtime_error("Failed to find a GPU with Vulkan support!");

    for (const auto& device : physical_devices) {
        if (isDeviceSuitable(device, m_surface.get())) {
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
*/
void
renderer::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies(m_physical_device, m_surface.get());

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

    m_device.reset(m_physical_device.createDevice(createinfo));

    m_graphics_queue     = m_device->getQueue(indices.graphicsFamily(), 0);
    m_presentation_queue = m_device->getQueue(indices.presentFamily(), 0);
}

void
renderer::initVulkan()
{
    createInstance();
    setupDebugCallback();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
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

    if (enableValidationLayers) {
        DestroyDebugReportCallbackEXT(m_instance.get(), m_callback);
    }

    // vk::surfaceKHR objects do not have a destroy()
    // https://github.com/KhronosGroup/Vulkan-Hpp/issues/204
    m_instance->destroySurfaceKHR(m_surface.release());

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

}  // namespace shiny::graphics
