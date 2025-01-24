#include "Game.h"
#include "graphics/renderer.h"

namespace shiny {

Game::Game()
{
    m_renderer = new shiny::renderer();
}


Game::~Game()
{}

void
Game::Run()
{
    m_renderer->run();
}

}  // namespace shiny
