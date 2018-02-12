#include <vk/instance.h>
#include <vk/physical_device.h>
#include <vk/utils.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW\glfw3.h>
#include <algorithm>
#include <iostream>

namespace {

// Finds the debugcallback extension if it exists, and returns the pointer to
// that extension function
VkResult
create_debug_report_callback_ext(VkInstance                                instance,
                                 const VkDebugReportCallbackCreateInfoEXT* p_create_info,
                                 VkDebugReportCallbackEXT*                 p_callback,
                                 const VkAllocationCallbacks*              p_allocator = nullptr)
{
    auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugReportCallbackEXT");

    if (func != nullptr) {
        return func(instance, p_create_info, p_allocator, p_callback);
    }

    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void
destroy_debug_report_callback_ext(VkInstance                   instance,
                                  VkDebugReportCallbackEXT     callback,
                                  const VkAllocationCallbacks* p_allocator = nullptr)
{
    auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugReportCallbackEXT");
    if (func != nullptr) {
        func(instance, callback, p_allocator);
    }
}


// checks if given layers are supported by the instance
bool
check_validation_layer_support(const std::vector<const char*>& layers)
{
    using shiny::vk::collect;

    auto available_layers = collect<VkLayerProperties>(vkEnumerateInstanceLayerProperties);

    for (const std::string& layer_name : layers) {
        bool available = false;
        for (const auto& elem : available_layers) {
            if (elem.layerName == layer_name) {
                available = true;
            }
        }
        if (available == false) {
            return false;
        }
    }

    return true;
}

std::vector<const char*>
get_required_extensions(bool enable_validation_layer)
{
    uint32_t     glfw_extension_count = 0;
    const char** glfw_extensions      = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    std::vector<const char*> extensions(glfw_extensions, glfw_extensions + glfw_extension_count);

    // TODO: make this optional/debug only
    extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);

    return extensions;
}

}  // namespace

namespace shiny::vk {

VKAPI_ATTR VkBool32 VKAPI_CALL
                    debug_callback(VkDebugReportFlagsEXT      flags,
                                   VkDebugReportObjectTypeEXT obj_type,
                                   uint64_t                   obj,
                                   size_t                     location,
                                   int32_t                    code,
                                   const char*                layer_prefix,
                                   const char*                msg,
                                   void*                      user_data)
{
    std::cerr << "validation layer: " << msg << std::endl;

    return VK_FALSE;
}

VkApplicationInfo
default_appinfo()
{
    VkApplicationInfo app_info = {};

    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName   = "Hello Triangle";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = "No Engine";
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion         = VK_API_VERSION_1_0;

    return app_info;
}

instance::instance(const std::vector<const char*>* enabled_layers)
  : m_instance(VK_NULL_HANDLE)
  , m_result(VK_SUCCESS)
{
    if (enabled_layers && check_validation_layer_support(*enabled_layers) == false) {
        throw std::runtime_error("validation layers are requested but not available!");
    }


    uint32_t     glfwExtensionCount = 0;
    const char** glfwExtensions     = nullptr;
    // consider replacing with get_required_extensions()
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    // check which extensions does glfw need that isn't supported
    auto instance_ext_names = extension_names();
    for (size_t i = 0; i < glfwExtensionCount; i++) {
        bool exists = false;
        for (const std::string& elem : instance_ext_names) {
            if (elem == glfwExtensions[i]) {
                exists = true;
            }
        }
        if (exists == false) {
            std::cerr << glfwExtensions[i] << " is not supported." << std::endl;
        }
    }

    VkApplicationInfo app_info = default_appinfo();

    VkInstanceCreateInfo create_info = {};

    create_info.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo = &app_info;

    auto extensions = get_required_extensions(enabled_layers ? true : false);

    create_info.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
    create_info.ppEnabledExtensionNames = extensions.data();
    create_info.enabledLayerCount       = 0;

    if (enabled_layers) {
        create_info.enabledLayerCount   = static_cast<uint32_t>(enabled_layers->size());
        create_info.ppEnabledLayerNames = enabled_layers->data();
    } else {
        create_info.enabledLayerCount = 0;
    }

    // https://www.khronos.org/registry/vulkan/specs/1.0/man/html/vkCreateInstance.html
    m_result = vkCreateInstance(&create_info, nullptr, &m_instance);
}

instance::~instance()
{
    if (m_callback)
        disable_debug_reporting();
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
    }
}

// Registers a callback for debugging, saves the opaque handle
void
instance::enable_debug_reporting()
{
    VkDebugReportCallbackCreateInfoEXT create_info = {};

    // TODO: Implement this
    create_info.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
    create_info.flags       = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    create_info.pfnCallback = debug_callback;

    if (create_debug_report_callback_ext(m_instance, &create_info, &m_callback) != VK_SUCCESS) {
        throw std::runtime_error("Failed to setup debug callback!");
    }
}

// deregisters the callback, and resets the state
void
instance::disable_debug_reporting()
{
    destroy_debug_report_callback_ext(m_instance, m_callback);
    m_callback = nullptr;
}

physical_device
instance::select_physical_device() const
{
    auto devices = collect<VkPhysicalDevice>(vkEnumeratePhysicalDevices, m_instance);

    if (devices.empty()) {
        throw std::runtime_error("Failed to find a GPU with vulkan support!");
    }

    for (physical_device device : devices) {
        if (device.is_device_suitable()) {
            return device;
        }
    }

    throw std::runtime_error("Failed to find a suitable GPU!");
}

// does not need created instance to be called
std::vector<VkExtensionProperties>
instance::extensions() const
{
    return collect<VkExtensionProperties>(vkEnumerateInstanceExtensionProperties, nullptr);
}

std::vector<std::string>
instance::extension_names() const
{
    auto extensions = this->extensions();

    std::vector<std::string> names(extensions.size());

    std::transform(extensions.begin(), extensions.end(), names.begin(),
                   [](const VkExtensionProperties& p) { return p.extensionName; });

    return names;
}

}  // namespace shiny::vk
