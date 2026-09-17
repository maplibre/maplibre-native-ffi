#pragma once

#include "maplibre_native_c.h"

struct mln_render_session_object;

namespace mln::core {

auto metal_surface_descriptor_default() noexcept
  -> mln_metal_surface_descriptor;
auto vulkan_surface_descriptor_default() noexcept
  -> mln_vulkan_surface_descriptor;
auto opengl_surface_descriptor_default() noexcept
  -> mln_opengl_surface_descriptor;
auto webgpu_surface_descriptor_default() noexcept
  -> mln_webgpu_surface_descriptor;
// The backend-independent half of surface descriptor validation: pointer, size,
// nested size, extent, and the handles the descriptor declares required.
// Builds without the backend reach this through the unsupported stub, so both
// report the same verdict.
//
// `require_supported_provider` is false wherever no OpenGL context can be
// created, so a host probing for OpenGL still hears about a malformed
// descriptor rather than an unsupported provider.
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
