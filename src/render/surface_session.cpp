#include "render/surface_session.hpp"

#include "maplibre_native_c.h"

namespace mln::core {

auto metal_surface_descriptor_default() noexcept
  -> mln_metal_surface_descriptor {
  return mln_metal_surface_descriptor{
    .size = sizeof(mln_metal_surface_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context =
      mln_metal_context_descriptor{
        .size = sizeof(mln_metal_context_descriptor),
        .device = nullptr,
      },
    .layer = nullptr,
  };
}

auto vulkan_surface_descriptor_default() noexcept
  -> mln_vulkan_surface_descriptor {
  return mln_vulkan_surface_descriptor{
    .size = sizeof(mln_vulkan_surface_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context =
      mln_vulkan_context_descriptor{
        .size = sizeof(mln_vulkan_context_descriptor),
        .instance = nullptr,
        .physical_device = nullptr,
        .device = nullptr,
        .graphics_queue = nullptr,
        .graphics_queue_family_index = 0,
      },
    .surface = nullptr,
  };
}

}  // namespace mln::core
