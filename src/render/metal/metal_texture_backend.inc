#pragma once

#include <optional>
#include <string>
#include <vector>

#include <mln/gfx/headless_backend.hpp>
#include <mln/mtl/headless_backend.hpp>

#include <Metal/MTLCommandBuffer.hpp>
#include <Metal/MTLCommandQueue.hpp>
#include <Metal/MTLDevice.hpp>
#include <Metal/MTLEvent.hpp>
#include <Metal/MTLTexture.hpp>

#include "render/render_session_common.hpp"

namespace mln::core {

class MetalTextureBackend final : public mln::mtl::RendererBackend,
                                  public mln::gfx::HeadlessBackend {
 private:
  class MetalTextureRenderableResource;

 public:
  struct CommandBufferFailure {
    // An MTLCommandBufferError, or 0 when Metal reported no error object.
    NS::Integer code = 0;
    std::string description;
  };

  // A null host_queue makes the backend create its own queue on the device.
  MetalTextureBackend(
    MTL::Device* host_device, MTL::CommandQueue* host_queue, mln::Size size,
    std::size_t ring_depth, MTL::PixelFormat pixel_format
  );
  MetalTextureBackend(
    std::vector<MTL::Texture*> borrowed_textures, MTL::CommandQueue* host_queue,
    mln::Size size
  );
  MetalTextureBackend(const MetalTextureBackend&) = delete;
  auto operator=(const MetalTextureBackend&) -> MetalTextureBackend& = delete;
  MetalTextureBackend(MetalTextureBackend&&) = delete;
  auto operator=(MetalTextureBackend&&) -> MetalTextureBackend& = delete;
  ~MetalTextureBackend() override;

  auto getDefaultRenderable() -> mln::gfx::Renderable& override;
  auto readStillImage() -> mln::PremultipliedImage override;
  auto getRendererBackend() -> mln::gfx::RendererBackend* override;
  void activate() override;
  void deactivate() override;

  auto metal_texture() -> MTL::Texture*;
  // Selects how each frame is handed over. Any kind but CPU completion makes
  // a frame acquirable once committed rather than once complete.
  void set_producer_sync_kind(mln_gpu_sync_kind kind) {
    producer_sync_kind_ = kind;
  }
  // The synchronization of the last frame committed.
  [[nodiscard]] auto producer_sync() const -> mln_gpu_sync;
  // Makes the next frame wait for `event` to reach `value`.
  void wait_event(MTL::SharedEvent* event, uint64_t value);
  // Waits for the frame a queue-ordered render left in flight, and records its
  // result.
  void finish_frame();
  auto select_slot(std::size_t slot) -> bool;
  void set_ring_size(mln::Size size);

  [[nodiscard]] auto has_device(const MTL::Device* other) const -> bool;
  // Whether a replacement texture has the pixel format this session's cached
  // render pipeline states were built against.
  [[nodiscard]] auto has_borrowed_pixel_format(MTL::PixelFormat format) const
    -> bool;
  void set_borrowed_textures(
    std::vector<MTL::Texture*> textures, mln::Size new_size
  );
  // Lets go of the borrowed textures until set_borrowed_textures() hands over
  // the next ring.
  void release_borrowed_textures();
  // The failure of the last command buffer a render submitted, if it failed,
  // which the next call no longer returns.
  auto take_command_buffer_failure() -> std::optional<CommandBufferFailure>;

 private:
  void record_command_buffer_result(const MTL::CommandBuffer& buffer);
  void commit(NS::SharedPtr<MTL::CommandBuffer> buffer);

  // The caller's texture for each slot, or empty for a session-owned ring.
  std::vector<MTL::Texture*> borrowed_textures_;
  // Recorded when this session took borrowed_textures_, which it does not
  // retain and the host may already have released.
  MTL::PixelFormat borrowed_pixel_format_ = MTL::PixelFormatInvalid;
  // The format of a session-owned ring's textures.
  MTL::PixelFormat owned_pixel_format_ = MTL::PixelFormatInvalid;
  RenderableSlotRing ring_;
  std::optional<CommandBufferFailure> command_buffer_failure_;
  mln_gpu_sync_kind producer_sync_kind_ = MLN_GPU_SYNC_CPU_COMPLETE;
  NS::SharedPtr<MTL::CommandBuffer> in_flight_;
  // The producer event, made when the first frame signals it, and the value
  // its last frame signaled.
  NS::SharedPtr<MTL::SharedEvent> event_;
  uint64_t event_value_ = 0;
  // Consumer events that the next frame waits for.
  struct EventWait {
    NS::SharedPtr<MTL::SharedEvent> event;
    uint64_t value = 0;
  };
  std::vector<EventWait> event_waits_;
};

}  // namespace mln::core
