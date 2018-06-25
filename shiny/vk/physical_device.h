#pragma once

#include <vk/ext/surface.h>
#include <vk/logical_device.h>
#include <vk/queue_families.h>

#include <vulkan/vulkan.h>

#include <optional>
#include <set>
#include <vector>

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
                    const std::optional<ext::surface>& surface        = std::nullopt,
                    const std::vector<const char*>&    enabled_layers = {});

    operator VkPhysicalDevice() const { return m_device; }

    logical_device create_logical_device() const;

    bool is_device_suitable() const;

    queue_families find_queue_families(
      const std::optional<ext::surface>& surface = std::nullopt) const;

    void set_queue_families(const std::optional<ext::surface>& surface = std::nullopt);

    void enabled_layers(const std::vector<const char*>& layers) { m_enabled_layers = layers; };
    std::vector<const char*> enabled_layers() const { return m_enabled_layers; }

    queue_families device_queue_family() const { return m_indices; }

private:
    std::vector<queue::create_info> generate_queue_info() const;

    /*
      Helper function that we can use to define all inputs functionally, the
      public function then just does all the information passing
    */
    logical_device _create_logical_device(const VkPhysicalDevice&         phys_device,
                                          const queue_families&           queues,
                                          const std::vector<const char*>& enabled_layers,
                                          const std::set<int>&            queue_create_infos) const;

    /*
      Feeder function into _create_logical_device
    */
    const std::set<int> _generate_queue_indices(const queue_families& families) const;

    VkPhysicalDevice         m_device = VK_NULL_HANDLE;
    queue_families           m_indices;
    std::vector<const char*> m_enabled_layers;
};

}  // namespace shiny::vk
