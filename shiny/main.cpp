#define GLFW_INCLUDE_VULKAN
#include <FreeImage.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>

#include <renderer.h>
#include <vk/instance.h>
#include <window.h>

int
main()
{
    try {
        shiny::window window;
        window.init();

        shiny::renderer renderer;
        renderer.init();

        while (window.close_window() == false) {
            window.poll_events();
        }
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
