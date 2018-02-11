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

#include <vk/instance.h>
#include <vulkan/vulkan.h>
#include <window.h>

namespace shiny::vk::ext {

class surface
{
public:
    surface()        = default;
    ~surface()       = default;
    surface& operator=(const surface& rhs) = default;

    /*
      We must pass in a non-const window since the glfw implementation demands a
      non-const pointer
    */
    void create(const instance& instance, window& window);
    void destroy();

private:
    const instance* m_instance_ref = nullptr;
    VkSurfaceKHR    m_surface      = VK_NULL_HANDLE;
};

}  // namespace shiny::vk::ext

#endif /* _EXT_SURFACE_h__ */
