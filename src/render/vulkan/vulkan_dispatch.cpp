// The dispatch header sets the vulkan.hpp configuration this translation unit
// has to agree on, so it stays the first include.
#include "render/vulkan/vulkan_dispatch.hpp"

namespace mln::core {

auto vulkan_system_get_instance_proc_addr() noexcept
  -> PFN_vkGetInstanceProcAddr {
  // The loader stays mapped for the process lifetime, so the pointer is
  // resolved once and cached.
  static PFN_vkGetInstanceProcAddr get_instance_proc_addr =
    []() noexcept -> PFN_vkGetInstanceProcAddr {
    try {
      static const vk::detail::DynamicLoader loader;
      return loader.getProcAddress<PFN_vkGetInstanceProcAddr>(
        "vkGetInstanceProcAddr"
      );
    } catch (...) {
      return nullptr;
    }
  }();
  return get_instance_proc_addr;
}

}  // namespace mln::core
