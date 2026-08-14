#include <algorithm>
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
  // Metal requires one sample count across a render pass, and both the depth
  // and stencil attachments the session builds and every pipeline mbgl creates
  // are single-sample.
  if (metal_texture->sampleCount() != 1) {
    mln::core::set_thread_error("Metal texture must be single-sample");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

class MetalTextureSessionBackend final
    : public mln::core::TextureSessionBackend {
 public:
  MetalTextureSessionBackend(
    MTL::Device* host_device, mbgl::Size size, std::size_t ring_depth
  )
      : backend_(host_device, size, ring_depth) {}

  MetalTextureSessionBackend(MTL::Texture* borrowed_texture, mbgl::Size size)
      : backend_(borrowed_texture, size) {}

  auto headless_backend() -> mbgl::gfx::HeadlessBackend& override {
    return backend_;
  }
  void resize(mbgl::Size size) override { backend_.set_ring_size(size); }

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
  auto select_render_slot(std::size_t slot) -> mln_status override {
    return backend_.select_slot(slot) ? MLN_STATUS_OK
                                      : MLN_STATUS_INVALID_ARGUMENT;
  }
  auto copy_slot_metadata(
    const mln_render_session_object& texture, std::size_t slot,
    std::any& out_metadata
  ) -> mln_status override {
    auto* metal_texture = backend_.metal_texture_at(slot);
    if (metal_texture == nullptr) {
      mln::core::set_thread_error("rendered Metal texture is not available");
      return MLN_STATUS_NOT_READY;
    }
    out_metadata = mln_metal_owned_texture_frame{
      .size = sizeof(mln_metal_owned_texture_frame),
      .generation = texture.generation,
      .width = static_cast<uint32_t>(metal_texture->width()),
      .height = static_cast<uint32_t>(metal_texture->height()),
      .scale_factor = texture.scale_factor,
      .frame_id = texture.frame_generation,
      .texture = metal_texture,
      .device = metal_texture->device(),
      .pixel_format = static_cast<uint64_t>(metal_texture->pixelFormat())
    };
    return MLN_STATUS_OK;
  }

 private:
  mln::core::MetalTextureBackend backend_;
};

}  // namespace

namespace mln::core {

auto supported_render_backend_mask() noexcept -> uint32_t {
  return MLN_RENDER_BACKEND_FLAG_METAL;
}

auto metal_owned_texture_attach_start(
  mln_map map, const mln_metal_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
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
  const auto physical_status = validate_physical_size(
    descriptor->extent.width, descriptor->extent.height,
    descriptor->extent.scale_factor, "scaled texture dimensions are too large"
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  const auto request_status =
    validate_render_session_attach_request(options, out_session, out_operation);
  if (request_status != MLN_STATUS_OK) {
    return request_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_session_extent(*session, descriptor->extent);
  session->texture.api_kind = TextureSessionApi::Metal;
  session->texture.mode = TextureSessionMode::Owned;
  auto device =
    NS::RetainPtr(static_cast<MTL::Device*>(descriptor->context.device));
  const auto ring_depth = std::clamp(
    options == nullptr ? 1u : options->requested_texture_ring_depth, 1u, 3u
  );
  session->initialize_backend = [device = std::move(device), ring_depth](
                                  mln_render_session_object& target
                                ) mutable {
    target.texture.backend = std::make_unique<MetalTextureSessionBackend>(
      device.get(), mbgl::Size{target.physical_width, target.physical_height},
      ring_depth
    );
    return MLN_STATUS_OK;
  };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = options == nullptr ? 0u : options->driver,
    .texture_ring_depth = ring_depth,
    .flags = MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION |
             MLN_RENDER_SESSION_CAPABILITY_READBACK |
             MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, out_operation
  );
}

auto metal_borrowed_texture_attach_start(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, mln_operation* out_operation
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
  const auto request_status =
    validate_render_session_attach_request(options, out_session, out_operation);
  if (request_status != MLN_STATUS_OK) {
    return request_status;
  }

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  set_borrowed_session_extent(
    *session, descriptor->extent, descriptor->physical_width,
    descriptor->physical_height
  );
  session->texture.api_kind = TextureSessionApi::Metal;
  session->texture.mode = TextureSessionMode::Borrowed;
  auto* const borrowed_texture =
    static_cast<MTL::Texture*>(descriptor->texture);
  session->initialize_backend =
    [borrowed_texture](mln_render_session_object& target) {
      target.texture.backend = std::make_unique<MetalTextureSessionBackend>(
        borrowed_texture,
        mbgl::Size{target.physical_width, target.physical_height}
      );
      return MLN_STATUS_OK;
    };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = options == nullptr ? 0u : options->driver,
    .texture_ring_depth = 0,
    .flags = MLN_RENDER_SESSION_CAPABILITY_READBACK
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, out_operation
  );
}

auto metal_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_metal_borrowed_texture_descriptor* descriptor,
  mln_operation* out_operation
) -> mln_status {
  const auto descriptor_status = validate_borrowed_texture(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  const auto copied = *descriptor;
  return enqueue_driver_operation(
    session, RENDER_OPERATION_MAINTENANCE,
    [copied](mln_render_session_object& target) {
      return render_session_set_target(
        target.self, RetargetTargetKind::BorrowedTexture, copied.extent,
        copied.physical_width, copied.physical_height,
        [&copied](mln_render_session_object& live) {
          return live.texture.backend->set_metal_borrowed_target(copied);
        }
      );
    },
    out_operation
  );
}

}  // namespace mln::core
