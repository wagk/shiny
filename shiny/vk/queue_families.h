#pragma once

#include <vulkan/vulkan.h>

namespace shiny::vk {

/*
    TODO: Fix this:
    We currently let external parties set the graphics_family index, which should not be the case.
*/
struct queue_families
{
    int graphics_family = -1;

    bool is_complete() const { return graphics_family >= 0; }

    VkQueueFamilyProperties m_props;
};

}  // namespace shiny::vk
