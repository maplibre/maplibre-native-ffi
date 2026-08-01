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
// The backend-independent half of surface descriptor validation: pointer, size,
// nested size, extent, and the handles the descriptor declares required. The
// backend that owns a descriptor adds whatever probing needs its own headers
// and loader on top; every other build reaches this through the unsupported
// stub, so both learn the same verdict from one definition.
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
auto metal_surface_attach(
  mln_map map, const mln_metal_surface_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status;
auto vulkan_surface_attach(
  mln_map map, const mln_vulkan_surface_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status;
auto opengl_surface_attach(
  mln_map map, const mln_opengl_surface_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status;

}  // namespace mln::core
