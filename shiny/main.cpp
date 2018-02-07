#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <FreeImage.h>

#include "vk/instance.h"

#include "window.h"
#include "renderer.h"

#include <iostream>
#include <stdexcept>

int main() {

    try {
        shiny::window window; window.init();
        shiny::renderer renderer; renderer.init();

        while (window.close_window() == false) {
            window.poll_events();
        }
    }
    catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}