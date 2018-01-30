#pragma once

#include "vk/instance.h"

struct GLFWwindow;

namespace shiny {

     class renderer
     {
     public:
          renderer();
          ~renderer();

          void init();
          void draw();

     private:

          // used in place of throwing exceptions
          bool m_bad_init;

          vk::instance m_instance;

     };

}
