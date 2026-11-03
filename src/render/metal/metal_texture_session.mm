#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include <mln/util/logging.hpp>
#include <mln/util/size.hpp>

#include <Metal/MTLCommandBuffer.hpp>
#include <Metal/MTLCommandQueue.hpp>
#include <Metal/MTLDevice.hpp>
#include <Metal/MTLPixelFormat.hpp>
#include <Metal/MTLTexture.hpp>

#include "render/metal/metal_texture_session.hpp"

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "render/metal/metal_texture_backend.inc"
#include "render/render_session_common.hpp"
#include "render/texture_session.hpp"

namespace {

auto metal_textures(std::span<const mln_metal_borrowed_texture> entries)
  -> std::vector<MTL::Texture*> {
  auto textures = std::vector<MTL::Texture*>{};
  textures.reserve(entries.size());
  for (const auto& entry : entries) {
    textures.push_back(static_cast<MTL::Texture*>(entry.texture));
  }
  return textures;
}

// The shared validators cannot reach MTL::Texture, so the checks that read the
// textures themselves live here, after them. `Target` is an attach descriptor
// or a replacement, which share these members.
template <typename Target>
auto validate_borrowed_textures(const Target* descriptor) -> mln_status {
  const auto physical_status = mln::core::validate_borrowed_physical_size(
    descriptor->physical_width, descriptor->physical_height
  );
  if (physical_status != MLN_STATUS_OK) {
    return physical_status;
  }
  const auto textures =
    metal_textures({descriptor->textures, descriptor->texture_count});
  for (auto* metal_texture : textures) {
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
      mln::core::set_thread_error(
        "Metal texture must allow render target usage"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    // Metal requires one sample count across a render pass, and both the
    // depth and stencil attachments the session builds and every pipeline mbgl
    // creates are single-sample.
    if (metal_texture->sampleCount() != 1) {
      mln::core::set_thread_error("Metal texture must be single-sample");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    // Every slot renders with the same device and the same cached pipeline
    // states, whose key omits the color format.
    if (
      metal_texture->device() != textures.front()->device() ||
      metal_texture->pixelFormat() != textures.front()->pixelFormat()
    ) {
      mln::core::set_thread_error(
        "Metal textures of one ring must share a device and a pixel format"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  return MLN_STATUS_OK;
}

auto validate_borrowed_texture(
  const mln_metal_borrowed_texture_descriptor* descriptor
) -> mln_status {
  const auto descriptor_status =
    mln::core::validate_metal_borrowed_texture_descriptor(descriptor);
  if (descriptor_status != MLN_STATUS_OK) {
    return descriptor_status;
  }
  return validate_borrowed_textures(descriptor);
}

auto validate_borrowed_texture(
  const mln_metal_borrowed_texture_replacement* replacement
) -> mln_status {
  const auto replacement_status =
    mln::core::validate_metal_borrowed_texture_replacement(replacement);
  if (replacement_status != MLN_STATUS_OK) {
    return replacement_status;
  }
  return validate_borrowed_textures(replacement);
}

// A failed frame keeps its map update pending. A device that was removed, or
// that revoked this process's access, fails every later frame too, so the
// session loses its target; any other failure leaves the next demand to retry.
auto report_command_buffer_failure(
  const mln::core::MetalTextureBackend::CommandBufferFailure& failure
) -> mln_status {
  const auto message = "Metal command buffer failed with error " +
                       std::to_string(failure.code) + ": " +
                       failure.description;
  mln::Log::Error(mln::Event::Render, message);
  switch (failure.code) {
    case MTL::CommandBufferErrorDeviceRemoved:
    case MTL::CommandBufferErrorAccessRevoked:
      mln::core::set_thread_error(message.c_str());
      return MLN_STATUS_TARGET_LOST;
    default:
      throw mln::core::RenderTargetNotReady(message);
  }
}

class MetalTextureSessionBackend final
    : public mln::core::TextureSessionBackend {
 public:
  MetalTextureSessionBackend(
    MTL::Device* host_device, MTL::CommandQueue* host_queue, mln::Size size,
    std::size_t ring_depth, MTL::PixelFormat pixel_format
  )
      : backend_(host_device, host_queue, size, ring_depth, pixel_format) {}

  MetalTextureSessionBackend(
    std::vector<MTL::Texture*> borrowed_textures, MTL::CommandQueue* host_queue,
    mln::Size size
  )
      : backend_(std::move(borrowed_textures), host_queue, size) {}

  void set_producer_sync_kind(mln_gpu_sync_kind kind) {
    backend_.set_producer_sync_kind(kind);
  }

  auto producer_sync() const -> mln_gpu_sync override {
    return backend_.producer_sync();
  }

  auto release_consumer_sync(const mln_gpu_sync& sync) -> mln_status override {
    if (sync.kind != MLN_GPU_SYNC_METAL_SHARED_EVENT)
      return MLN_STATUS_UNSUPPORTED;
    backend_.wait_event(
      // The C ABI carries the event as the bit pattern of its pointer.
      // NOLINTNEXTLINE(performance-no-int-to-ptr)
      reinterpret_cast<MTL::SharedEvent*>(static_cast<uintptr_t>(sync.handle)),
      sync.value
    );
    return MLN_STATUS_OK;
  }

  auto headless_backend() -> mln::gfx::HeadlessBackend& override {
    return backend_;
  }
  void resize(mln::Size size) override { backend_.set_ring_size(size); }

  // Command buffers retain the objects they use, so their GPU work keeps
  // those objects alive and destruction needs no wait.
  [[nodiscard]] auto allows_off_thread_teardown() const noexcept
    -> bool override {
    return true;
  }

  auto set_metal_borrowed_target(const mln::core::MetalBorrowedTextures& target)
    -> mln_status override {
    auto textures = metal_textures(target.textures);
    if (!backend_.has_device(textures.front()->device())) {
      mln::core::set_thread_error(
        "Metal texture target must belong to the device this session attached "
        "with"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (!backend_.has_borrowed_pixel_format(textures.front()->pixelFormat())) {
      return mln::core::unsupported_retarget(
        "Metal texture target must have the pixel format this session's render "
        "pipeline states were built for; destroy the session and attach again "
        "to change it"
      );
    }
    backend_.set_borrowed_textures(std::move(textures), target.physical_size);
    return MLN_STATUS_OK;
  }

  auto release_borrowed_target() -> mln_status override {
    backend_.release_borrowed_textures();
    return MLN_STATUS_OK;
  }

  auto after_render(mln_render_session_object&, bool& out_rendered)
    -> mln_status override {
    if (auto failure = backend_.take_command_buffer_failure()) {
      return report_command_buffer_failure(*failure);
    }
    // The Metal backend creates its texture on the first real draw; a renderer
    // pass can complete without one before content is ready.
    out_rendered = backend_.metal_texture() != nullptr;
    return MLN_STATUS_OK;
  }
  auto select_render_slot(std::size_t slot) -> mln_status override {
    return backend_.select_slot(slot) ? MLN_STATUS_OK
                                      : MLN_STATUS_INVALID_ARGUMENT;
  }
  auto record_frame_metadata(
    const mln::core::RenderFrameMetadata& frame, std::any& out_metadata
  ) -> mln_status override {
    auto* metal_texture = backend_.metal_texture();
    if (metal_texture == nullptr) {
      throw mln::core::RenderTargetNotReady(
        "rendered Metal texture is not available"
      );
    }
    out_metadata = mln_metal_texture_frame{
      .size = sizeof(mln_metal_texture_frame),
      .width = static_cast<uint32_t>(metal_texture->width()),
      .height = static_cast<uint32_t>(metal_texture->height()),
      .scale_factor = frame.scale_factor,
      .slot = frame.slot,
      .texture = metal_texture,
      .device = metal_texture->device(),
      .pixel_format = static_cast<uint64_t>(metal_texture->pixelFormat())
    };
    return MLN_STATUS_OK;
  }

 private:
  mln::core::MetalTextureBackend backend_;
};

auto retain_host_queue(
  void* queue, const MTL::Device* device,
  NS::SharedPtr<MTL::CommandQueue>& out_queue
) -> mln_status {
  if (queue == nullptr) return MLN_STATUS_OK;
  auto* const command_queue = static_cast<MTL::CommandQueue*>(queue);
  if (command_queue->device() != device) {
    mln::core::set_thread_error(
      "Metal command queue must belong to the target's device"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  out_queue = NS::RetainPtr(command_queue);
  return MLN_STATUS_OK;
}

}  // namespace

namespace mln::core {

auto supported_render_backend_mask() noexcept -> uint32_t {
  return MLN_RENDER_BACKEND_FLAG_METAL;
}

auto create_metal_offscreen_texture_backend(MTL::Device* device, mln::Size size)
  -> std::unique_ptr<TextureSessionBackend> {
  return std::make_unique<MetalTextureSessionBackend>(
    device, nullptr, size, 1, MTL::PixelFormatRGBA8Unorm
  );
}

auto metal_borrowed_texture_attach_start(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
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

  auto session = std::make_shared<mln_render_session_object>();
  session->map = map;
  size_new_borrowed_session(
    *session, descriptor->logical_size, descriptor->physical_width,
    descriptor->physical_height
  );
  session->target_identity = render_target_identity(
    std::span{descriptor->textures, descriptor->texture_count}
  );
  session->texture.mode = TextureSessionMode::Borrowed;
  auto textures =
    metal_textures({descriptor->textures, descriptor->texture_count});
  auto queue = NS::SharedPtr<MTL::CommandQueue>{};
  const auto queue_status = retain_host_queue(
    descriptor->command_queue, textures.front()->device(), queue
  );
  if (queue_status != MLN_STATUS_OK) return queue_status;
  if (queue) session->producer_sync_kinds |= MLN_GPU_SYNC_KIND_FLAG_QUEUE_ORDER;
  session->producer_sync_kinds |= MLN_GPU_SYNC_KIND_FLAG_METAL_SHARED_EVENT;
  session->initialize_backend = [textures = std::move(textures),
                                 queue](mln_render_session_object& target) {
    auto backend = std::make_unique<MetalTextureSessionBackend>(
      textures, queue.get(),
      mln::Size{target.physical_width, target.physical_height}
    );
    backend->set_producer_sync_kind(target.producer_sync_kind);
    target.texture.backend = std::move(backend);
    return MLN_STATUS_OK;
  };
  const auto capabilities = mln_render_session_capabilities{
    .size = sizeof(mln_render_session_capabilities),
    .driver = {},
    .texture_ring_depth = static_cast<uint32_t>(descriptor->texture_count),
    .flags = MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION,
    .producer_sync_kind = {},
    .consumer_sync_kinds = MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE |
                           MLN_GPU_SYNC_KIND_FLAG_METAL_SHARED_EVENT |
                           (queue ? MLN_GPU_SYNC_KIND_FLAG_QUEUE_ORDER : 0U),
  };
  return start_attach_render_session(
    std::move(session), RenderSessionKind::Texture, options, capabilities,
    out_session, completion,
    valueless_completion<&mln_map_attach_metal_borrowed_texture>()
  );
}

auto metal_borrowed_texture_set_target_start(
  mln_render_session session,
  const mln_metal_borrowed_texture_replacement* replacement,
  const mln_completion* completion
) -> mln_status {
  auto checked_completion = std::shared_ptr<Completion>{};
  const auto submission_status = validate_render_session_retarget_submission(
    session, RetargetTargetKind::BorrowedTexture, completion, checked_completion
  );
  if (submission_status != MLN_STATUS_OK) {
    return submission_status;
  }
  const auto replacement_status = validate_borrowed_texture(replacement);
  if (replacement_status != MLN_STATUS_OK) {
    return replacement_status;
  }
  auto target = MetalBorrowedTextures{*replacement};
  auto identity = render_target_identity(std::span{target.textures});
  return enqueue_borrowed_texture_retarget(
    session, replacement->texture_count, replacement->logical_size,
    target.physical_size, std::move(identity),
    [target = std::move(target)](mln_render_session_object& live, mln::Size) {
      return live.texture.backend->set_metal_borrowed_target(target);
    },
    std::move(checked_completion),
    valueless_completion<
      &mln_render_session_set_metal_borrowed_texture_target>()
  );
}

}  // namespace mln::core
