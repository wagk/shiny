#pragma once

#include <vulkan/vulkan.h>

namespace shiny::vk {

/*
  All GPU's support specific queue families. We use this to determine what kind
  of queues we can use from that GPU
*/
class queue_families
{
public:
    bool is_complete() const;

    int  graphics_family() const { return m_graphics_family; }
    void graphics_family(int index) { m_graphics_family = index; }
    int  presentation_family() const { return m_presentation_family; }
    void presentation_family(int index) { m_presentation_family = index; }

    VkQueueFamilyProperties raw_properties() const { return m_props; }
    void raw_properties(const VkQueueFamilyProperties& props) { m_props = props; }

private:
    int m_graphics_family     = -1;
    int m_presentation_family = -1;

    VkQueueFamilyProperties m_props;
};

}  // namespace shiny::vk
