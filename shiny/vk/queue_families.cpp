#include <vk/queue_families.h>

#include <vulkan/vulkan.h>

namespace shiny::vk {

bool
queue_families::is_complete() const
{
    return m_graphics_family >= 0 && m_presentation_family >= 0;
}

}  // namespace shiny::vk
