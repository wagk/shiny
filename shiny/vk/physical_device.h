#pragma once

#include <vk/ext/surface.h>
#include <vk/logical_device.h>
#include <vk/queue_families.h>

#include <vulkan/vulkan.h>

#include <optional>

namespace shiny::vk {

/*
  Each physical device maps to one physical GPU mounted to the computer.

  Use this to query the type of queue families and other GPU characteristics

  Programmatic interface to the device lies with logical_device.
*/
class physical_device
{
public:
    physical_device(const VkPhysicalDevice&            device,
                    const std::optional<ext::surface>& surface = std::nullopt);

    operator VkPhysicalDevice() const { return m_device; }

    logical_device create_logical_device(
      const std::vector<const char*>* enabled_layers = nullptr) const;

    queue_families find_queue_families(
      const std::optional<ext::surface>& surface = std::nullopt) const;

    void set_queue_families(const std::optional<ext::surface>& surface = std::nullopt);

    bool is_device_suitable() const;

private:
    VkPhysicalDevice m_device = VK_NULL_HANDLE;
    queue_families   m_indices;
};

}  // namespace shiny::vk
