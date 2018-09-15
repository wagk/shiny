#include "stdafx.h"

#include <iostream>
#include <stdexcept>

#include <graphics/renderer.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define MAX_LOADSTRING 100

HINSTANCE hInst;  // current instance
WCHAR     szTitle[MAX_LOADSTRING];
WCHAR     szWindowClass[MAX_LOADSTRING];

// Forward declarations of functions included in this code module:
// ATOM    MyRegisterClass(HINSTANCE hInstance);
// BOOL    InitInstance(HINSTANCE, int);
// LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// This is the old main. Keeping this as a fallback until I get the rest setup right
int
main()
{
    shiny::graphics::renderer renderer;

    try {
        renderer.run();
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
