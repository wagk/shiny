#include <vk/ext/surface.h>
#include <vk/instance.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <utility>

namespace shiny::vk::ext {

surface::surface(const shiny::vk::instance* inst, VkSurfaceKHR surface)
  : m_instance_ref(inst)
  , m_surface(surface)
{}

surface::~surface()
{
    vkDestroySurfaceKHR(*m_instance_ref, m_surface, nullptr);
}

surface::surface(const surface&& sur)
  : m_instance_ref(std::move(sur.m_instance_ref))
  , m_surface(std::move(sur.m_surface))
{}

surface&
surface::operator=(const surface&& sur)
{
    m_instance_ref = std::move(sur.m_instance_ref);
    m_surface      = std::move(sur.m_surface);
    return *this;
}


}  // namespace shiny::vk::ext

// surface
