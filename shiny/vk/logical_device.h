#pragma once

#include <vulkan\vulkan.h>
#include <vector>

#include <vk\physical_device.h>

namespace shiny::vk {

    class logical_device
    {
    public:

        explicit logical_device() = default;

        void create(const physical_device& device);

    private:

        VkDevice m_device;

    };

    //device is the common term, not logical_device, even though logical_device is factually correct
    using device = logical_device;
}