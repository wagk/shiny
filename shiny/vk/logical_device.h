/*
  Logical devices are interfaces to the physical device.

  I'm not sure why we're allowed to have multiple interfaces per physical
  device, but we are.
*/
#include <vulkan\vulkan.h>

#include <vector>

#include <vk\physical_device.h>
#include <vk\queue.h>

namespace shiny::vk {

class logical_device
{
public:
    explicit logical_device() = default;
    ~logical_device();

    void create(const physical_device&          device,
                const std::vector<const char*>* enabled_layers = nullptr);

    void destroy();

    /*
      Queues belong to the logical device, they automatically created along with
      the device, but we need to assign handles to them
    */
    queue get_queue() const;

private:
    VkDevice             m_device;
    queue_family_indices m_indices;
};

// device is the common term, not logical_device, even though logical_device is
// factually correct
using device = logical_device;

}  // namespace shiny::vk
