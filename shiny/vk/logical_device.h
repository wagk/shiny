#pragma once

#include <vector>
#include <vk\physical_device.h>
#include <vulkan\vulkan.h>

namespace shiny::vk {

class logical_device
{
public:
    explicit logical_device() = default;
    ~logical_device();

    void create(const physical_device&          device,
                const std::vector<const char*>* enabled_layers = nullptr);

    void destroy();

private:
    VkDevice m_device;
};

// device is the common term, not logical_device, even though logical_device is
// factually correct
using device = logical_device;

}  // namespace shiny::vk
