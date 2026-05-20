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

auto validate_texture(mln_render_session* texture) -> mln_status {
  const auto status = validate_render_session(texture);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (texture->kind != RenderSessionKind::Texture) {
    set_thread_error("render session is not a texture session");
    return MLN_STATUS_UNSUPPORTED;
  }
  return MLN_STATUS_OK;
}

auto validate_live_attached_texture(mln_render_session* texture) -> mln_status {
  const auto status = validate_texture(texture);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (!texture->attached || texture->texture.backend == nullptr) {
    set_thread_error("render session is detached");
    return MLN_STATUS_INVALID_STATE;
  }
  return MLN_STATUS_OK;
}

auto texture_read_premultiplied_rgba8(
  mln_render_session* texture, uint8_t* out_data, size_t out_data_capacity,
  mln_texture_image_info* out_info
) -> mln_status {
  const auto status = validate_live_attached_texture(texture);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_info == nullptr || out_info->size < sizeof(mln_texture_image_info)) {
    set_thread_error("out_info must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (texture->texture.acquired) {
    set_thread_error("cannot read while a texture frame is acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (texture->texture.mode != TextureSessionMode::Owned) {
    set_thread_error("texture session does not support CPU readback");
    return MLN_STATUS_UNSUPPORTED;
  }
  if (texture->rendered_generation != texture->generation) {
    set_thread_error("no rendered frame is available for this generation");
    return MLN_STATUS_INVALID_STATE;
  }
  if (texture->physical_width > std::numeric_limits<uint32_t>::max() / 4) {
    set_thread_error("texture readback stride is too large");
    return MLN_STATUS_INVALID_STATE;
  }

  const auto stride = texture->physical_width * 4;
  if (
    texture->physical_height != 0 &&
    stride > std::numeric_limits<size_t>::max() / texture->physical_height
  ) {
    set_thread_error("texture readback byte length is too large");
    return MLN_STATUS_INVALID_STATE;
  }
  const auto byte_length =
    static_cast<size_t>(stride) * texture->physical_height;

  *out_info = mln_texture_image_info{
    .size = sizeof(mln_texture_image_info),
    .width = texture->physical_width,
    .height = texture->physical_height,
    .stride = stride,
    .byte_length = byte_length
  };

  if (out_data == nullptr || out_data_capacity < byte_length) {
    set_thread_error("out_data capacity is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto* renderer_backend = texture->texture.backend->renderer_backend();
  if (renderer_backend == nullptr) {
    set_thread_error("texture session renderer backend is not available");
    return MLN_STATUS_INVALID_STATE;
  }
  auto guard = mbgl::gfx::BackendScope{
    *renderer_backend, mbgl::gfx::BackendScope::ScopeType::Implicit
  };
  auto image = texture->texture.backend->headless_backend().readStillImage();
  if (!image.valid()) {
    set_thread_error("texture readback did not produce an image");
    return MLN_STATUS_INVALID_STATE;
  }
  if (
    image.size.width != texture->physical_width ||
    image.size.height != texture->physical_height || image.stride() != stride ||
    image.bytes() != byte_length
  ) {
    set_thread_error("texture readback image layout did not match the session");
    return MLN_STATUS_INVALID_STATE;
  }

  std::memcpy(out_data, image.data.get(), image.bytes());
  return MLN_STATUS_OK;
}

}  // namespace mln::core
