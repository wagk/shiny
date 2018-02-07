#pragma once

#include "vulkan\vulkan.h"
#include "../renderer.h"
#include <vector>

// forward decl

namespace shiny::vk {

    class logical_device
    {
    public:
        logical_device(renderer* iRenderer);
        ~logical_device();

        void create_logical_device(bool enableValidationLayers, std::vector<const char*> validation_layers);

    private:
        renderer* mRenderer;

        VkDevice m_device;
    };
}