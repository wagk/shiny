#pragma once

#include <fstream>
#include <stdlib.h>
#include <string>
#include <vector>

//#define STB_IMAGE_IMPLEMENTATION
//#include <include/stb_image.h>

#include "vulkan/vulkan.hpp"

// Base texture class
class Texture
{
public:
    vk::Device*             device;
    vk::Image               image;
    vk::ImageLayout         image_layout;
    vk::DeviceMemory        device_memory;
    vk::ImageView           image_view;
    uint32_t                width, height;
    uint32_t                mip_levels;
    uint32_t                layer_count;
    vk::DescriptorImageInfo descriptor;

    // Optional Sampler
    vk::Sampler sampler;

    // Update image descriptor from current sampler, view and image layout.
    void updateDescriptor();

    // Release resources
    void destroy(const vk::Device& device);
};


// 2D texture
class Texture2D : public Texture
{
public:
    /**
     * Load a 2D texture including all mip levels
     *
     * @param filename File to load (supports .ktx and .dds)
     * @param format Vulkan format of the image data stored in the file
     * @param device Vulkan device to create the texture on
     * @param copyQueue Queue used for the texture staging copy commands (must support transfer)
     * @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to
     * VK_IMAGE_USAGE_SAMPLED_BIT)
     * @param (Optional) imageLayout Usage layout for the texture (defaults
     * VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
     * @param (Optional) forceLinear Force linear tiling (not advised, defaults to false)
     *
     */
    void loadFromFile(std::string         filename,
                      vk::Format          format,
                      vk::Device*         device,
                      vk::Queue           copyQueue,
                      vk::ImageUsageFlags imageUsageFlags = vk::ImageUsageFlagBits::eSampled,
                      vk::ImageLayout     imageLayout     = vk::ImageLayout::eShaderReadOnlyOptimal,
                      bool                forceLinear     = false);

    void fromBuffer(void*          buffer,
                    vk::DeviceSize bufferSize,
                    vk::Format     format,
                    uint32_t       width,
                    uint32_t       height,
                    vk::Device*    device,
                    vk::Queue      copyQueue,
                    vk::Filter     filter       = vk::Filter::eLinear,
                    vk::ImageUsageFlags         = vk::ImageUsageFlagBits::eSampled,
                    vk::ImageLayout imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal);
};
