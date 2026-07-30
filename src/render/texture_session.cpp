#include <cstdint>
#include <cstring>
#include <limits>

#include <mbgl/gfx/backend_scope.hpp>
#include <mbgl/util/image.hpp>

#include "render/texture_session.hpp"

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

namespace mln::core {

auto texture_image_info_default() noexcept -> mln_texture_image_info {
  return mln_texture_image_info{
    .size = sizeof(mln_texture_image_info),
    .width = 0,
    .height = 0,
    .stride = 0,
    .byte_length = 0
  };
}

auto validate_texture(
  mln_render_session texture, mln_render_session_object*& out_texture
) -> mln_status {
  const auto status = validate_render_session(texture, out_texture);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_texture->kind != RenderSessionKind::Texture) {
    set_thread_error("render session is not a texture session");
    return MLN_STATUS_UNSUPPORTED;
  }
  return MLN_STATUS_OK;
}

auto validate_live_attached_texture(
  mln_render_session texture, mln_render_session_object*& out_texture
) -> mln_status {
  const auto status = validate_texture(texture, out_texture);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!out_texture->attached || out_texture->texture.backend == nullptr) {
    set_thread_error("render session is detached");
    return MLN_STATUS_INVALID_STATE;
  }
  return MLN_STATUS_OK;
}

auto validate_webgpu_owned_texture_descriptor(
  const mln_webgpu_owned_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_webgpu_owned_texture_descriptor)) {
    set_thread_error("mln_webgpu_owned_texture_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  return validate_webgpu_context(descriptor->context, true);
}

auto validate_webgpu_borrowed_texture_descriptor(
  const mln_webgpu_borrowed_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_webgpu_borrowed_texture_descriptor)) {
    set_thread_error(
      "mln_webgpu_borrowed_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto physical_status = validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  const auto context_status =
    validate_webgpu_context(descriptor->context, true);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->texture == nullptr || descriptor->texture_view == nullptr) {
    set_thread_error("WebGPU texture and texture_view must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->format == 0) {
    set_thread_error("WebGPU texture format must be specified");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto webgpu_borrowed_texture_set_target(
  mln_render_session session, const mln_webgpu_borrowed_texture_target* target
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_texture(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    live->texture.api_kind != TextureSessionApi::WebGPU ||
    live->texture.mode != TextureSessionMode::Borrowed
  ) {
    set_thread_error("render session is not a WebGPU borrowed-texture session");
    return MLN_STATUS_UNSUPPORTED;
  }
  if (
    target == nullptr ||
    target->size < sizeof(mln_webgpu_borrowed_texture_target)
  ) {
    set_thread_error("target must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (target->texture == nullptr || target->texture_view == nullptr) {
    set_thread_error("WebGPU texture and texture_view must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  const auto replace_status =
    live->texture.backend->set_webgpu_borrowed_target(*target);
  if (replace_status == MLN_STATUS_OK) {
    live->texture.borrowed_target_available = true;
    live->texture.rendered_native_texture = nullptr;
  }
  return replace_status;
}

auto webgpu_borrowed_texture_clear_target(mln_render_session session)
  -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_texture(session, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    live->texture.api_kind != TextureSessionApi::WebGPU ||
    live->texture.mode != TextureSessionMode::Borrowed
  ) {
    set_thread_error("render session is not a WebGPU borrowed-texture session");
    return MLN_STATUS_UNSUPPORTED;
  }
  if (!live->texture.borrowed_target_available) {
    return MLN_STATUS_OK;
  }

  const auto clear_status =
    live->texture.backend->clear_webgpu_borrowed_target();
  if (clear_status == MLN_STATUS_OK) {
    live->texture.borrowed_target_available = false;
    live->texture.rendered_native_texture = nullptr;
  }
  return clear_status;
}

auto texture_read_premultiplied_rgba8(
  mln_render_session texture, uint8_t* out_data, size_t out_data_capacity,
  mln_texture_image_info* out_info
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_texture(texture, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_info == nullptr || out_info->size < sizeof(mln_texture_image_info)) {
    set_thread_error("out_info must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (live->texture.acquired) {
    set_thread_error("cannot read while a texture frame is acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (live->texture.mode != TextureSessionMode::Owned) {
    set_thread_error("texture session does not support CPU readback");
    return MLN_STATUS_UNSUPPORTED;
  }
  if (!live->texture.backend->supports_readback()) {
    set_thread_error("texture backend does not support CPU readback");
    return MLN_STATUS_UNSUPPORTED;
  }
  if (live->rendered_generation != live->generation) {
    set_thread_error("no rendered frame is available for this generation");
    return MLN_STATUS_INVALID_STATE;
  }
  if (live->physical_width > std::numeric_limits<uint32_t>::max() / 4) {
    set_thread_error("texture readback stride is too large");
    return MLN_STATUS_INVALID_STATE;
  }

  const auto stride = live->physical_width * 4;
  if (
    live->physical_height != 0 &&
    stride > std::numeric_limits<size_t>::max() / live->physical_height
  ) {
    set_thread_error("texture readback byte length is too large");
    return MLN_STATUS_INVALID_STATE;
  }
  const auto byte_length = static_cast<size_t>(stride) * live->physical_height;

  *out_info = mln_texture_image_info{
    .size = sizeof(mln_texture_image_info),
    .width = live->physical_width,
    .height = live->physical_height,
    .stride = stride,
    .byte_length = byte_length
  };

  if (out_data == nullptr || out_data_capacity < byte_length) {
    set_thread_error("out_data capacity is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* renderer_backend = live->texture.backend->renderer_backend();
  if (renderer_backend == nullptr) {
    set_thread_error("texture session renderer backend is not available");
    return MLN_STATUS_NATIVE_ERROR;
  }
  auto current = ScopedCurrentScheduler{live->scheduler};
  auto guard = mbgl::gfx::BackendScope{*renderer_backend};
  auto image = live->texture.backend->headless_backend().readStillImage();
  if (!image.valid()) {
    set_thread_error("texture readback did not produce an image");
    return MLN_STATUS_NATIVE_ERROR;
  }
  if (
    image.size.width != live->physical_width ||
    image.size.height != live->physical_height || image.stride() != stride ||
    image.bytes() != byte_length
  ) {
    set_thread_error("texture readback image layout did not match the session");
    return MLN_STATUS_NATIVE_ERROR;
  }

  std::memcpy(out_data, image.data.get(), image.bytes());
  return MLN_STATUS_OK;
}

}  // namespace mln::core
