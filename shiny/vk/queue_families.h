#pragma once

#include <vk/ext/surface.h>

#include <vulkan/vulkan.h>

#include <optional>

namespace shiny::vk {

/*
  All GPU's support specific queue families. We use this to determine what kind
  of queues we can use from that GPU
*/
class queue_families
{
public:
    queue_families() = default;  // TODO: Remove this function once the migration is complete
    queue_families(const VkPhysicalDevice&            device,
                   const std::optional<ext::surface>& surface = std::nullopt);

    // TODO: Replace `is_complete` with a better name, and a more parameterised
    // function This function is possibly misnamed, since what it does is check
    // if the indices contained within this struct is non-zero (ie, returned by
    // the physical device query)
    //
    // It's functionality is probably too simplistically modelled, and hence
    // causing some architectural pain points.
    //
    // We should rename it to something like `suitable_for` and provide some
    // enum flags for checking or something similar
    bool is_complete() const;

    int  graphics_family_index() const { return m_graphics_family; }
    void graphics_family_index(int index) { m_graphics_family = index; }
    int  presentation_family_index() const { return m_presentation_family; }
    void presentation_family_index(int index) { m_presentation_family = index; }

    VkQueueFamilyProperties raw_properties() const { return m_props; }
    void raw_properties(const VkQueueFamilyProperties& props) { m_props = props; }

private:
    int m_graphics_family     = -1;
    int m_presentation_family = -1;

    VkQueueFamilyProperties m_props;
};

}  // namespace shiny::vk
