#include "graphics/texture.h"

void
Texture::updateDescriptor()
{
    descriptor.sampler     = sampler;
    descriptor.imageView   = image_view;
    descriptor.imageLayout = image_layout;
}

void
Texture::destroy(const vk::Device& device)
{
    device.destroyImageView(image_view);
    device.destroyImage(image);
    if (sampler) {
        device.destroySampler(sampler);
    }
    device.freeMemory(device_memory);
}

void
Texture2D::loadFromFile(std::string         filename,
                        vk::Format          format,
                        vk::Device*         device,
                        vk::Queue           copyQueue,
                        vk::ImageUsageFlags imageUsageFlags,
                        vk::ImageLayout     imageLayout,
                        bool                forceLinear)
{}

void
Texture2D::fromBuffer(void*          buffer,
                      vk::DeviceSize bufferSize,
                      vk::Format     format,
                      uint32_t       width,
                      uint32_t       height,
                      vk::Device*    device,
                      vk::Queue      copyQueue,
                      vk::Filter     filter,
                      vk::ImageUsageFlags,
                      vk::ImageLayout imageLayout)
{}
