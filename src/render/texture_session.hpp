#pragma once

#include <cstddef>
#include <cstdint>

#include "maplibre_native_c.h"

struct mln_render_session_object;

namespace mln::core {

// The MTLPixelFormat values a session-owned Metal ring accepts. Builds without
// Metal validate them too; the Metal build asserts that they match.
inline constexpr uint64_t metal_rgba8_unorm = 70;
inline constexpr uint64_t metal_bgra8_unorm = 80;

auto supported_render_backend_mask() noexcept -> uint32_t;
auto validate_texture(
  mln_render_session texture, mln_render_session_object*& out_texture
) -> mln_status;
auto validate_live_attached_texture(
  mln_render_session texture, mln_render_session_object*& out_texture
) -> mln_status;
// Validate descriptor shape even in unsupported builds, so malformed input
// reports the same error on every backend. Provider support is checked only
// where an OpenGL context can be created.
auto validate_metal_owned_texture_descriptor(
  const mln_metal_owned_texture_descriptor* descriptor
) -> mln_status;
auto validate_metal_borrowed_texture_descriptor(
  const mln_metal_borrowed_texture_descriptor* descriptor
) -> mln_status;
auto validate_vulkan_owned_texture_descriptor(
  const mln_vulkan_owned_texture_descriptor* descriptor
) -> mln_status;
auto validate_vulkan_borrowed_texture_descriptor(
  const mln_vulkan_borrowed_texture_descriptor* descriptor
) -> mln_status;
auto validate_opengl_owned_texture_descriptor(
  const mln_opengl_owned_texture_descriptor* descriptor,
  bool require_supported_provider
) -> mln_status;
auto validate_opengl_borrowed_texture_descriptor(
  const mln_opengl_borrowed_texture_descriptor* descriptor,
  bool require_supported_provider
) -> mln_status;
auto validate_webgpu_owned_texture_descriptor(
  const mln_webgpu_owned_texture_descriptor* descriptor
) -> mln_status;
auto validate_webgpu_borrowed_texture_descriptor(
  const mln_webgpu_borrowed_texture_descriptor* descriptor
) -> mln_status;
auto webgpu_owned_texture_attach_start(
  mln_map map, const mln_webgpu_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto webgpu_borrowed_texture_attach_start(
  mln_map map, const mln_webgpu_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto webgpu_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_webgpu_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) -> mln_status;
auto metal_owned_texture_attach_start(
  mln_map map, const mln_metal_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto metal_borrowed_texture_attach_start(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto vulkan_owned_texture_attach_start(
  mln_map map, const mln_vulkan_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto vulkan_borrowed_texture_attach_start(
  mln_map map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto opengl_owned_texture_attach_start(
  mln_map map, const mln_opengl_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto opengl_borrowed_texture_attach_start(
  mln_map map, const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) -> mln_status;
auto metal_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) -> mln_status;
auto vulkan_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) -> mln_status;
auto opengl_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) -> mln_status;
auto texture_read_premultiplied_rgba8_start(
  mln_render_session texture, const mln_completion* completion
) -> mln_status;
auto acquired_frame_get_metal_texture(
  mln_acquired_frame frame, mln_metal_texture_frame* out_frame
) -> mln_status;
auto acquired_frame_get_vulkan_texture(
  mln_acquired_frame frame, mln_vulkan_texture_frame* out_frame
) -> mln_status;
auto acquired_frame_get_opengl_texture(
  mln_acquired_frame frame, mln_opengl_texture_frame* out_frame
) -> mln_status;
auto acquired_frame_get_webgpu_texture(
  mln_acquired_frame frame, mln_webgpu_texture_frame* out_frame
) -> mln_status;

}  // namespace mln::core
