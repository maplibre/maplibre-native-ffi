#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "render/surface_session.hpp"

auto mln_map_attach_metal_surface(
  mln_map map, const mln_metal_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::metal_surface_attach_start(
      map, descriptor, options, out_session, completion
    );
  });
}

auto mln_map_attach_vulkan_surface(
  mln_map map, const mln_vulkan_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::vulkan_surface_attach_start(
      map, descriptor, options, out_session, completion
    );
  });
}

auto mln_map_attach_open_gl_surface(
  mln_map map, const mln_open_gl_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::opengl_surface_attach_start(
      map, descriptor, options, out_session, completion
    );
  });
}

auto mln_map_attach_web_gpu_surface(
  mln_map map, const mln_web_gpu_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::webgpu_surface_attach_start(
      map, descriptor, options, out_session, completion
    );
  });
}

auto mln_render_session_set_metal_surface_target(
  mln_render_session session, const mln_metal_surface_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::metal_surface_set_target_start(
      session, descriptor, completion
    );
  });
}

auto mln_render_session_set_vulkan_surface_target(
  mln_render_session session, const mln_vulkan_surface_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::vulkan_surface_set_target_start(
      session, descriptor, completion
    );
  });
}

auto mln_render_session_set_open_gl_surface_target(
  mln_render_session session, const mln_open_gl_surface_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::opengl_surface_set_target_start(
      session, descriptor, completion
    );
  });
}

auto mln_render_session_set_web_gpu_surface_target(
  mln_render_session session, const mln_web_gpu_surface_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::webgpu_surface_set_target_start(
      session, descriptor, completion
    );
  });
}
