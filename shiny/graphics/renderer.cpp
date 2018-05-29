#include "graphics/renderer.h"

#include <cstring>
#include <iostream>
#include <vector>

#define UNREFERENCED_PARAMETER(P) (P)

namespace {

const int WIDTH  = 800;
const int HEIGHT = 600;

const std::vector<const char*> validationLayers = { "VK_LAYER_LUNARG_standard_validation" };

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
    int m_graphicsFamily = -1;

public:
    bool isComplete() { return graphicsFamily() >= 0; }

    int  graphicsFamily() const { return m_graphicsFamily; }
    void graphicsFamily(int i) { m_graphicsFamily = i; }
};

QueueFamilyIndices
findQueueFamilies(const vk::PhysicalDevice& device)
{
    QueueFamilyIndices indices;

    std::vector<vk::QueueFamilyProperties> families = device.getQueueFamilyProperties();

    for (size_t i = 0; i < families.size(); i++) {
        auto queuefamily = families[i];
        if (queuefamily.queueCount > 0 && queuefamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            indices.graphicsFamily((int)i);
        }

        if (indices.isComplete()) {
            break;
        }
    }

    return indices;
}

bool
isDeviceSuitable(const vk::PhysicalDevice& device)
{
    return findQueueFamilies(device).isComplete();
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
A physical device usually represents a single complete implementation of Vulkan (excluding
instance-level functionality) available to the host, of which there are a finite number.

https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#devsandqueues-physical-device-enumeration
*/
void
renderer::pickPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> physical_devices = m_instance->enumeratePhysicalDevices();

    if (physical_devices.empty())
        throw std::runtime_error("Failed to find a GPU with Vulkan support!");

    for (const auto& device : physical_devices) {
        if (isDeviceSuitable(device)) {
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
    QueueFamilyIndices indices = findQueueFamilies(m_physical_device);

    float queuepriority = 1.f;

    auto queuecreateinfo = vk::DeviceQueueCreateInfo()
                             .setQueueFamilyIndex(indices.graphicsFamily())
                             .setQueueCount(1)
                             .setPQueuePriorities(&queuepriority);

    auto devicefeatures = vk::PhysicalDeviceFeatures();

    auto createinfo = vk::DeviceCreateInfo()
                        .setPQueueCreateInfos(&queuecreateinfo)
                        .setQueueCreateInfoCount(1)
                        .setPEnabledFeatures(&devicefeatures)
                        .setEnabledExtensionCount(0);

    if (enableValidationLayers) {
        createinfo.setEnabledLayerCount((uint32_t)validationLayers.size());
        createinfo.setPpEnabledLayerNames(validationLayers.data());
    }

    m_physical_device.createDevice(createinfo);
}

void
renderer::initVulkan()
{
    createInstance();
    setupDebugCallback();
    pickPhysicalDevice();
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

    glfwDestroyWindow(m_window);
    glfwTerminate();
}

}  // namespace shiny::graphics
