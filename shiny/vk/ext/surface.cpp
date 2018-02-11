#include <vk/ext/surface.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace shiny::vk::ext {

void
surface::create(const instance& instance, window& window)
{
    m_instance_ref = &instance;

    if (glfwCreateWindowSurface(*m_instance_ref, window, nullptr, &m_surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void
surface::destroy()
{
    vkDestroySurfaceKHR(*m_instance_ref, m_surface, nullptr);
    m_instance_ref = nullptr;
    m_surface      = VK_NULL_HANDLE;
}


}  // namespace shiny::vk::ext

// surface
