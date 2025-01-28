//#include "Resource.h"
//#include "stdafx.h"

#include <iostream>
#include <stdexcept>

#include <game/Game.h>
#include <graphics/renderer.h>

// #define WINDOW_WIDTH 1280
// #define WINDOW_HEIGHT 720
// #define MAX_LOADSTRING 100

// This is the old main. Keeping this as a fallback until I get the rest setup right
int
main()
{
    // shiny::Game m_game;

    shiny::renderer renderer;

    try {
        renderer.run();
        // m_game.Run();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
