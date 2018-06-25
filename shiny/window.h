#pragma once

struct GLFWwindow;

namespace shiny {

class window
{
public:
    window(int width = 800, int height = 600);
    ~window();

    operator const GLFWwindow*() const { return m_window; }
    operator GLFWwindow*() { return m_window; }

    void init();
    void poll_events();
    bool close_window();

private:
    int         m_width;
    int         m_height;
    GLFWwindow* m_window = nullptr;
};

}  // namespace shiny
