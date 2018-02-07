#include "physical_device.h"

namespace shiny::vk {

     bool physical_device::select_physical_device(const instance& inst)
     {
          uint32_t device_count = 0;
          vkEnumeratePhysicalDevices(inst, &device_count, nullptr);

          if (device_count == 0) {
               throw std::runtime_error("Failed to find a GPU with vulkan support!");
          }

          std::vector<VkPhysicalDevice> devices(device_count);
          vkEnumeratePhysicalDevices(inst, &device_count, devices.data());

          for(const auto& device: devices) {
               if(is_device_suitable(device)){
                    m_device = device;
                    break;
               }
          }

          if(m_device == VK_NULL_HANDLE){
               throw std::runtime_error("Failed to find a suitable GPU!");
          }
     }

     bool physical_device::is_device_suitable(VkPhysicalDevice device) const
     {
          // VkPhysicalDeviceProperties device_properties;
          // vkGetPhysicalDeviceProperties(m_device, &device_properties);

          // VkPhysicalDeviceFeatures device_features;
          // vkGetPhysicalDeviceFeatures(m_device, &device_features);

          // return
          //      device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
          //      device_features.geometryShader;

          //currently just return true since we only need it to support vulkan,
          //not have fancy features
          return true;
     }

     physical_device::physical_device()
     {
     }

     physical_device::~physical_device()
     {
     }

}
