#pragma once

#include "vulkan\vulkan.h"
#include <vector>

namespace shiny {
    class renderer;
}

namespace shiny::vk {
    // forward decl
    class logical_device
    {
    public:
        logical_device();
        ~logical_device();

        void create_logical_device(renderer* iRenderer, bool enableValidationLayers, std::vector<const char*> validation_layers);

    private:
        VkDevice m_device;
    };
}