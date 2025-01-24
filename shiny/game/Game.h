#pragma once

namespace shiny {

class Game
{
public:
    Game();
    ~Game();

    // Main Loop functions
    void Run();

protected:
    class renderer* m_renderer;
};

}  // namespace shiny
