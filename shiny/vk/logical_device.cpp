#include <vk/logical_device.h>
#include <vk/physical_device.h>
#include <renderer.h>

namespace shiny {
    void vk::logical_device::create_logical_device(renderer* iRenderer, bool enableValidationLayers, std::vector<const char*> validation_layers) {
        VkPhysicalDevice mVPD = iRenderer->get_physical_device()->get_vk_physical_device();
        queue_family_indices indices = find_queue_families(mVPD);

        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphics_family;
        queueCreateInfo.queueCount = 1;

        float queuePriority = 1.0f;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        VkPhysicalDeviceFeatures deviceFeatures = {};

        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        createInfo.pQueueCreateInfos = &queueCreateInfo;
        createInfo.queueCreateInfoCount = 1;

        createInfo.pEnabledFeatures = &deviceFeatures;

        createInfo.enabledExtensionCount = 0;

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
            createInfo.ppEnabledLayerNames = validation_layers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(mVPD, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(m_device, indices.graphics_family, 0, &(iRenderer->get_graphics_queue()));
    }

    vk::logical_device::logical_device() {
    }

    vk::logical_device::~logical_device() {
    }
}