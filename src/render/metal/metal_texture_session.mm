#include <memory>

#include <mbgl/util/size.hpp>

#include <Metal/MTLDevice.hpp>
#include <Metal/MTLPixelFormat.hpp>
#include <Metal/MTLTexture.hpp>

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "render/metal/metal_texture_backend.inc"
#include "render/render_session_common.hpp"
#include "render/texture_session.hpp"

namespace {
// The shared validator cannot reach MTL::Texture, so the checks that read the
// texture itself live here, after it.
auto validate_borrowed_texture(
  const mln_metal_borrowed_texture_descriptor* descriptor
) -> mln_status {
  const auto descriptor_status =
    mln::core::validate_metal_borrowed_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  // Non-null, because the shared validator rejects a null texture.
  auto* metal_texture = static_cast<MTL::Texture*>(descriptor->texture);
  const auto physical_status = mln::core::validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  if (
    metal_texture->width() != descriptor->physical_width ||
    metal_texture->height() != descriptor->physical_height
  ) {
    mln::core::set_thread_error(
      "Metal texture dimensions must match descriptor physical size"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((metal_texture->usage() & MTL::TextureUsageRenderTarget) == 0) {
    mln::core::set_thread_error("Metal texture must allow render target usage");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

class MetalTextureSessionBackend final
    : public mln::core::TextureSessionBackend {
 public:
  MetalTextureSessionBackend(MTL::Device* host_device, mbgl::Size size)
      : backend_(host_device, size) {}

  MetalTextureSessionBackend(MTL::Texture* borrowed_texture, mbgl::Size size)
      : backend_(borrowed_texture, size) {}

  auto headless_backend() -> mbgl::gfx::HeadlessBackend& override {
    return backend_;
  }

  auto set_metal_borrowed_target(
    const mln_metal_borrowed_texture_descriptor& descriptor
  ) -> mln_status override {
    auto* texture = static_cast<MTL::Texture*>(descriptor.texture);
    if (!backend_.has_device(texture->device())) {
      mln::core::set_thread_error(
        "Metal texture target must belong to the device this session attached "
        "with"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (!backend_.has_borrowed_pixel_format(texture->pixelFormat())) {
      return mln::core::unsupported_retarget(
        "Metal texture target must have the pixel format this session's render "
        "pipeline states were built for; destroy the session and attach again "
        "to change it"
      );
    }
    backend_.set_borrowed_texture(
      texture, mbgl::Size{descriptor.physical_width, descriptor.physical_height}
    );
    return MLN_STATUS_OK;
  }

  auto after_render(mln_render_session_object& texture, bool& out_rendered)
    -> mln_status override {
    auto* rendered_texture = backend_.metal_texture();
    if (rendered_texture == nullptr) {
      // The Metal backend creates its texture on the first real draw; a
      // renderer pass can complete without one before content is ready.
      out_rendered = false;
      return MLN_STATUS_OK;
    }
    texture.texture.rendered_native_texture = rendered_texture;
    out_rendered = true;
    return MLN_STATUS_OK;
  }

 private:
  mln::core::MetalTextureBackend backend_;
};

auto fill_frame(
  mln_render_session_object* texture, mln_metal_owned_texture_frame* out_frame
) -> mln_status {
  auto* metal_texture =
    static_cast<MTL::Texture*>(texture->texture.rendered_native_texture);
  if (metal_texture == nullptr) {
    mln::core::set_thread_error("rendered Metal texture is not available");
    return MLN_STATUS_NATIVE_ERROR;
  }

  *out_frame = mln_metal_owned_texture_frame{
    .size = sizeof(mln_metal_owned_texture_frame),
    .generation = texture->generation,
    .width = texture->physical_width,
    .height = texture->physical_height,
    .scale_factor = texture->scale_factor,
    .frame_id = texture->texture.next_frame_id,
    .texture = metal_texture,
    .device = metal_texture->device(),
    .pixel_format = static_cast<uint64_t>(metal_texture->pixelFormat())
  };
  return MLN_STATUS_OK;
}
}  // namespace

namespace mln::core {

auto supported_render_backend_mask() noexcept -> uint32_t {
  return MLN_RENDER_BACKEND_FLAG_METAL;
}

auto metal_owned_texture_attach(
  mln_map map, const mln_metal_owned_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_metal_owned_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled texture dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_session_extent(*session, descriptor->extent);
  session->texture.api_kind = TextureSessionApi::Metal;
  session->texture.mode = TextureSessionMode::Owned;
  session->texture.backend = std::make_unique<MetalTextureSessionBackend>(
    static_cast<MTL::Device*>(descriptor->context.device),
    mbgl::Size{session->physical_width, session->physical_height}
  );
  return attach_render_session(
    std::move(session), out_session, RenderSessionKind::Texture,
    RenderSessionAttachMessages{
      .null_session = "texture session must not be null",
      .null_output = "out_session must not be null",
      .non_null_output = "out_session must point to a null handle"
    }
  );
}

auto metal_borrowed_texture_attach(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  mln_render_session* out_session
) -> mln_status {
  MapObject* live_map = nullptr;
  const auto map_status = validate_map_live(map, live_map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_borrowed_texture(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto output_status = validate_attach_output(
    out_session, "out_session must not be null",
    "out_session must point to a null handle"
  );
  if (output_status != MLN_STATUS_OK) {
    return output_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_borrowed_session_extent(
    *session, descriptor->extent, descriptor->physical_width,
    descriptor->physical_height
  );
  session->texture.api_kind = TextureSessionApi::Metal;
  session->texture.mode = TextureSessionMode::Borrowed;
  session->texture.backend = std::make_unique<MetalTextureSessionBackend>(
    static_cast<MTL::Texture*>(descriptor->texture),
    mbgl::Size{session->physical_width, session->physical_height}
  );
  return attach_render_session(
    std::move(session), out_session, RenderSessionKind::Texture,
    RenderSessionAttachMessages{
      .null_session = "texture session must not be null",
      .null_output = "out_session must not be null",
      .non_null_output = "out_session must point to a null handle"
    }
  );
}

auto metal_owned_texture_acquire_frame(
  mln_render_session texture, mln_metal_owned_texture_frame* out_frame
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_texture(texture, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (
    out_frame == nullptr ||
    out_frame->size < sizeof(mln_metal_owned_texture_frame)
  ) {
    set_thread_error("out_frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (live->texture.acquired) {
    set_thread_error("a texture frame is already acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (live->rendered_generation != live->generation) {
    set_thread_error("no rendered frame is available for this generation");
    return MLN_STATUS_INVALID_STATE;
  }
  if (
    live->texture.mode != TextureSessionMode::Owned ||
    live->texture.api_kind != TextureSessionApi::Metal
  ) {
    set_thread_error("texture session cannot expose a Metal texture frame");
    return MLN_STATUS_UNSUPPORTED;
  }

  const auto frame_status = fill_frame(live, out_frame);
  if (frame_status != MLN_STATUS_OK) {
    return frame_status;
  }
  live->texture.acquired_native_texture = live->texture.rendered_native_texture;
  live->texture.acquired = true;
  live->texture.acquired_frame_id = out_frame->frame_id;
  live->texture.acquired_frame_kind = TextureSessionFrameKind::MetalOwned;
  ++live->texture.next_frame_id;
  return MLN_STATUS_OK;
}

auto metal_owned_texture_release_frame(
  mln_render_session texture, const mln_metal_owned_texture_frame* frame
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_texture(texture, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (frame == nullptr || frame->size < sizeof(mln_metal_owned_texture_frame)) {
    set_thread_error("frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (
    !live->texture.acquired ||
    live->texture.acquired_frame_kind != TextureSessionFrameKind::MetalOwned
  ) {
    set_thread_error("no texture frame is currently acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (frame->generation != live->generation) {
    set_thread_error("frame generation does not match acquired frame");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (frame->frame_id != live->texture.acquired_frame_id) {
    set_thread_error("frame identity does not match acquired frame");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  live->texture.acquired = false;
  live->texture.acquired_frame_id = 0;
  live->texture.acquired_frame_kind = TextureSessionFrameKind::None;
  live->texture.acquired_native_texture = nullptr;
  return MLN_STATUS_OK;
}

auto metal_borrowed_texture_set_target(
  mln_render_session session,
  const mln_metal_borrowed_texture_descriptor* descriptor
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto session_status = validate_render_session_retarget(
    session, RetargetTargetKind::BorrowedTexture, live
  );
  if (session_status != MLN_STATUS_OK) {
    return session_status;
  }
  const auto descriptor_status =
    validate_metal_borrowed_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  return render_session_set_target(
    session, RetargetTargetKind::BorrowedTexture, descriptor->extent,
    descriptor->physical_width, descriptor->physical_height,
    [descriptor](mln_render_session_object& target_session) -> mln_status {
      return target_session.texture.backend->set_metal_borrowed_target(
        *descriptor
      );
    }
  );
}

}  // namespace mln::core
