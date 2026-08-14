#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "render/surface_session.hpp"

auto mln_metal_surface_descriptor_default(void) noexcept
  -> mln_metal_surface_descriptor {
  return mln::core::metal_surface_descriptor_default();
}

auto mln_vulkan_surface_descriptor_default(void) noexcept
  -> mln_vulkan_surface_descriptor {
  return mln::core::vulkan_surface_descriptor_default();
}

auto mln_opengl_surface_descriptor_default(void) noexcept
  -> mln_opengl_surface_descriptor {
  return mln::core::opengl_surface_descriptor_default();
}

auto mln_webgpu_surface_descriptor_default(void) noexcept
  -> mln_webgpu_surface_descriptor {
  return mln::core::webgpu_surface_descriptor_default();
}

auto mln_metal_surface_attach_start(
  mln_map map, const mln_metal_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::metal_surface_attach_start(
      map, descriptor, options, out_session, out_operation
    );
  });
}

auto mln_vulkan_surface_attach_start(
  mln_map map, const mln_vulkan_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::vulkan_surface_attach_start(
      map, descriptor, options, out_session, out_operation
    );
  });
}

auto mln_opengl_surface_attach_start(
  mln_map map, const mln_opengl_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::opengl_surface_attach_start(
      map, descriptor, options, out_session, out_operation
    );
  });
}

auto mln_webgpu_surface_attach_start(
  mln_map map, const mln_webgpu_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::webgpu_surface_attach_start(
      map, descriptor, options, out_session, out_operation
    );
  });
}

auto mln_metal_surface_set_target_start(
  mln_render_session session, const mln_metal_surface_descriptor* descriptor,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::metal_surface_set_target_start(
      session, descriptor, out_operation
    );
  });
}

auto mln_vulkan_surface_set_target_start(
  mln_render_session session, const mln_vulkan_surface_descriptor* descriptor,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::vulkan_surface_set_target_start(
      session, descriptor, out_operation
    );
  });
}

auto mln_opengl_surface_set_target_start(
  mln_render_session session, const mln_opengl_surface_descriptor* descriptor,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::opengl_surface_set_target_start(
      session, descriptor, out_operation
    );
  });
}

auto mln_webgpu_surface_set_target_start(
  mln_render_session session, const mln_webgpu_surface_descriptor* descriptor,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::webgpu_surface_set_target_start(
      session, descriptor, out_operation
    );
  });
}
