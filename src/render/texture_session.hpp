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
// nested size, extent, and the handles the descriptor declares required. The
// backend that owns a descriptor adds whatever probing needs its own headers on
// top; every other build reaches this through the unsupported stub, so both
// learn the same verdict from one definition.
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
auto metal_owned_texture_attach(
  mln_map map, const mln_metal_owned_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status;
auto metal_borrowed_texture_attach(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status;
auto vulkan_owned_texture_attach(
  mln_map map, const mln_vulkan_owned_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status;
auto vulkan_borrowed_texture_attach(
  mln_map map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status;
auto opengl_owned_texture_attach(
  mln_map map, const mln_opengl_owned_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status;
auto opengl_borrowed_texture_attach(
  mln_map map, const mln_opengl_borrowed_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status;
auto metal_borrowed_texture_set_target(
  mln_render_session session,
  const mln_metal_borrowed_texture_descriptor* descriptor
) -> mln_status;
auto vulkan_borrowed_texture_set_target(
  mln_render_session session,
  const mln_vulkan_borrowed_texture_descriptor* descriptor
) -> mln_status;
auto opengl_borrowed_texture_set_target(
  mln_render_session session,
  const mln_opengl_borrowed_texture_descriptor* descriptor
) -> mln_status;
auto texture_read_premultiplied_rgba8(
  mln_render_session texture, uint8_t* out_data, size_t out_data_capacity,
  mln_texture_image_info* out_info
) -> mln_status;
auto metal_owned_texture_acquire_frame(
  mln_render_session texture, mln_metal_owned_texture_frame* out_frame
) -> mln_status;
auto metal_owned_texture_release_frame(
  mln_render_session texture, const mln_metal_owned_texture_frame* frame
) -> mln_status;
auto vulkan_owned_texture_acquire_frame(
  mln_render_session texture, mln_vulkan_owned_texture_frame* out_frame
) -> mln_status;
auto vulkan_owned_texture_release_frame(
  mln_render_session texture, const mln_vulkan_owned_texture_frame* frame
) -> mln_status;
auto opengl_owned_texture_acquire_frame(
  mln_render_session texture, mln_opengl_owned_texture_frame* out_frame
) -> mln_status;
auto opengl_owned_texture_release_frame(
  mln_render_session texture, const mln_opengl_owned_texture_frame* frame
) -> mln_status;

}  // namespace mln::core
