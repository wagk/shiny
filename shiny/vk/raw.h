#pragma once

#include <vulkan/vulkan.h>

/*
  This namespace is to perform thin wrapping of Vk api handles, until someone
  manages to go the extra mile and wrap them into classes proper
*/
namespace shiny::vk::raw {

using result = VkResult;

namespace handle {

    using instance = VkInstance;

}  // namespace handle

namespace debug {
    using report_flags       = VkDebugReportFlagsEXT;
    using report_object_type = VkDebugReportObjectTypeEXT;
    using report_callback    = VkDebugReportCallbackEXT;
}  // namespace debug

}  // namespace shiny::vk::raw
