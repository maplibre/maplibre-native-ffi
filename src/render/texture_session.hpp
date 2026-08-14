#pragma once

#include <cstddef>
#include <cstdint>

#include "maplibre_native_c.h"

struct mln_render_session_object;

namespace mln::core {

auto supported_render_backend_mask() noexcept -> uint32_t;
auto metal_owned_texture_descriptor_default() noexcept
  -> mln_metal_owned_texture_descriptor;
auto metal_borrowed_texture_descriptor_default() noexcept
  -> mln_metal_borrowed_texture_descriptor;
auto vulkan_owned_texture_descriptor_default() noexcept
  -> mln_vulkan_owned_texture_descriptor;
auto vulkan_borrowed_texture_descriptor_default() noexcept
  -> mln_vulkan_borrowed_texture_descriptor;
auto texture_image_info_default() noexcept -> mln_texture_image_info;
auto validate_texture(
  mln_render_session texture, mln_render_session_object*& out_texture
) -> mln_status;
auto validate_live_attached_texture(
  mln_render_session texture, mln_render_session_object*& out_texture
) -> mln_status;
// The backend-independent half of texture descriptor validation: pointer, size,
// nested size, extent, and the handles the descriptor declares required.
// Builds without the backend reach this through the unsupported stub, so both
// report the same verdict.
//
// `require_supported_provider` is false wherever no OpenGL context can be
// created, so a host probing for OpenGL still hears about a malformed
// descriptor rather than an unsupported provider.
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
auto webgpu_owned_texture_descriptor_default() noexcept
  -> mln_webgpu_owned_texture_descriptor;
auto webgpu_borrowed_texture_descriptor_default() noexcept
  -> mln_webgpu_borrowed_texture_descriptor;
auto webgpu_owned_texture_attach_start(
  mln_map map, const mln_webgpu_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) -> mln_status;
auto webgpu_borrowed_texture_attach_start(
  mln_map map, const mln_webgpu_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) -> mln_status;
auto webgpu_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_webgpu_borrowed_texture_descriptor* descriptor,
  mln_operation* out_operation
) -> mln_status;
auto metal_owned_texture_attach_start(
  mln_map map, const mln_metal_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) -> mln_status;
auto metal_borrowed_texture_attach_start(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) -> mln_status;
auto vulkan_owned_texture_attach_start(
  mln_map map, const mln_vulkan_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) -> mln_status;
auto vulkan_borrowed_texture_attach_start(
  mln_map map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) -> mln_status;
auto opengl_owned_texture_attach_start(
  mln_map map, const mln_opengl_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) -> mln_status;
auto opengl_borrowed_texture_attach_start(
  mln_map map, const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
) -> mln_status;
auto metal_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_metal_borrowed_texture_descriptor* descriptor,
  mln_operation* out_operation
) -> mln_status;
auto vulkan_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_vulkan_borrowed_texture_descriptor* descriptor,
  mln_operation* out_operation
) -> mln_status;
auto opengl_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_opengl_borrowed_texture_descriptor* descriptor,
  mln_operation* out_operation
) -> mln_status;
auto texture_read_premultiplied_rgba8_start(
  mln_render_session texture, mln_operation* out_operation
) -> mln_status;
auto texture_read_premultiplied_rgba8_take_result(
  mln_operation operation, mln_buffer* out_data,
  mln_texture_image_info* out_info
) -> mln_status;
auto acquired_frame_get_metal_texture(
  mln_acquired_frame frame, mln_metal_owned_texture_frame* out_frame
) -> mln_status;
auto acquired_frame_get_vulkan_texture(
  mln_acquired_frame frame, mln_vulkan_owned_texture_frame* out_frame
) -> mln_status;
auto acquired_frame_get_opengl_texture(
  mln_acquired_frame frame, mln_opengl_owned_texture_frame* out_frame
) -> mln_status;
auto acquired_frame_get_webgpu_texture(
  mln_acquired_frame frame, mln_webgpu_owned_texture_frame* out_frame
) -> mln_status;

}  // namespace mln::core
