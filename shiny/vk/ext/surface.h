#pragma once
#ifndef _EXT_SURFACE_h__
#define _EXT_SURFACE_h__

/*
  Vulkan surfaces are extensions because while the API is platform agnostic,
  window creation isn't.

  So we will be tossing this (and all extensions) into their own
  namespace/folder

  We'll be assuming windows compatitability for now, with linux coming later
  once we get through the tutorial and possibly done some refactoring
*/

#include <vulkan/vulkan.h>

namespace shiny::vk {
class instance;
}

namespace shiny::vk::ext {

class surface
{
public:
    surface(const instance* inst_ref, VkSurfaceKHR m_surface);
    ~surface();

    surface(const surface&) = delete;
    surface& operator=(const surface&) = delete;

    surface(surface&&);
    surface& operator=(surface&& rhs);

    operator VkSurfaceKHR() const { return m_surface; }

private:
    const instance* m_instance_ref = nullptr;
    VkSurfaceKHR    m_surface      = VK_NULL_HANDLE;
};

}  // namespace shiny::vk::ext

#endif /* _EXT_SURFACE_h__ */
