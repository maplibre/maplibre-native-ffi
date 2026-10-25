#pragma once

#include "maplibre_native_c.h"

struct mln_render_session_object;

namespace mln::core {

// Validate descriptor shape even in unsupported builds, so malformed input
// reports the same error on every backend. Provider support is checked only
// where an OpenGL context can be created.
auto validate_metal_surface_descriptor(
  const mln_metal_surface_descriptor* descriptor
) -> mln_status;
auto validate_vulkan_surface_descriptor(
  const mln_vulkan_surface_descriptor* descriptor
) -> mln_status;
auto validate_opengl_surface_descriptor(
  const mln_opengl_surface_descriptor* descriptor,
  bool require_supported_provider
) -> mln_status;
auto validate_webgpu_surface_descriptor(
  const mln_webgpu_surface_descriptor* descriptor
) -> mln_status;
auto metal_surface_attach_start(
  mln_map map, const mln_metal_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto vulkan_surface_attach_start(
  mln_map map, const mln_vulkan_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto opengl_surface_attach_start(
  mln_map map, const mln_opengl_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto webgpu_surface_attach_start(
  mln_map map, const mln_webgpu_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto metal_surface_set_target_start(
  mln_render_session session, const mln_metal_surface_descriptor* descriptor,
  const mln_completion* completion
) -> mln_status;
auto vulkan_surface_set_target_start(
  mln_render_session session, const mln_vulkan_surface_descriptor* descriptor,
  const mln_completion* completion
) -> mln_status;
auto opengl_surface_set_target_start(
  mln_render_session session, const mln_opengl_surface_descriptor* descriptor,
  const mln_completion* completion
) -> mln_status;
auto webgpu_surface_set_target_start(
  mln_render_session session, const mln_webgpu_surface_descriptor* descriptor,
  const mln_completion* completion
) -> mln_status;

}  // namespace mln::core
