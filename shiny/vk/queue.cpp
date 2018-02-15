#include <vk/queue.h>

namespace shiny::vk {

queue::queue(const VkQueue& queue)
  : m_queue(queue)
{}

}  // namespace shiny::vk
