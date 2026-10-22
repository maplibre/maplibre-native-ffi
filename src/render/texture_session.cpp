#include <any>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>

#include <mln/gfx/backend_scope.hpp>
#include <mln/util/image.hpp>

#include "render/texture_session.hpp"

#include "completion/completion_result.hpp"
#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "operation/operation.hpp"
#include "render/render_session_common.hpp"

namespace mln::core {

auto metal_owned_texture_descriptor_default() noexcept
  -> mln_metal_owned_texture_descriptor {
  return mln_metal_owned_texture_descriptor{
    .size = sizeof(mln_metal_owned_texture_descriptor),
    .extent =
      mln_logical_extent{
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context =
      mln_metal_context_descriptor{
        .device = nullptr,
      },
    .pixel_format = metal_rgba8_unorm,
  };
}

auto metal_borrowed_texture_descriptor_default() noexcept
  -> mln_metal_borrowed_texture_descriptor {
  return mln_metal_borrowed_texture_descriptor{
    .size = sizeof(mln_metal_borrowed_texture_descriptor),
    .extent =
      mln_logical_extent{
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .physical_width = 256,
    .physical_height = 256,
    .textures = nullptr,
    .texture_count = 0,
  };
}

auto vulkan_owned_texture_descriptor_default() noexcept
  -> mln_vulkan_owned_texture_descriptor {
  return mln_vulkan_owned_texture_descriptor{
    .size = sizeof(mln_vulkan_owned_texture_descriptor),
    .extent =
      mln_logical_extent{
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context = mln_vulkan_context_descriptor{
      .instance = nullptr,
      .physical_device = nullptr,
      .device = nullptr,
      .graphics_queue = nullptr,
      .graphics_queue_family_index = 0,
      .get_instance_proc_addr = nullptr,
      .get_device_proc_addr = nullptr,
    },
  };
}

auto vulkan_borrowed_texture_descriptor_default() noexcept
  -> mln_vulkan_borrowed_texture_descriptor {
  return mln_vulkan_borrowed_texture_descriptor{
    .size = sizeof(mln_vulkan_borrowed_texture_descriptor),
    .extent =
      mln_logical_extent{
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .physical_width = 256,
    .physical_height = 256,
    .context =
      mln_vulkan_context_descriptor{
        .instance = nullptr,
        .physical_device = nullptr,
        .device = nullptr,
        .graphics_queue = nullptr,
        .graphics_queue_family_index = 0,
        .get_instance_proc_addr = nullptr,
        .get_device_proc_addr = nullptr,
      },
    .textures = nullptr,
    .texture_count = 0,
    .format = 0,
    .initial_layout = 0,
    .final_layout = 5,
  };
}

auto webgpu_owned_texture_descriptor_default() noexcept
  -> mln_webgpu_owned_texture_descriptor {
  return mln_webgpu_owned_texture_descriptor{
    .size = sizeof(mln_webgpu_owned_texture_descriptor),
    .extent =
      mln_logical_extent{
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context = mln_webgpu_context_descriptor{
      .instance = nullptr,
      .device = nullptr,
      .queue = nullptr,
    },
  };
}

auto webgpu_borrowed_texture_descriptor_default() noexcept
  -> mln_webgpu_borrowed_texture_descriptor {
  return mln_webgpu_borrowed_texture_descriptor{
    .size = sizeof(mln_webgpu_borrowed_texture_descriptor),
    .extent =
      mln_logical_extent{
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .physical_width = 256,
    .physical_height = 256,
    .context =
      mln_webgpu_context_descriptor{
        .instance = nullptr,
        .device = nullptr,
        .queue = nullptr,
      },
    .textures = nullptr,
    .texture_count = 0,
    .format = 0,
  };
}

namespace {

// Two ring slots must never share a texture: rendering into one would overwrite
// a frame that the host holds from the other.
template <typename Texture, typename Key>
auto validate_borrowed_textures(
  const Texture* textures, size_t count, Key key, const char* null_message
) -> mln_status {
  if (textures == nullptr || count == 0) {
    set_thread_error("textures must name at least one texture");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (count > max_texture_ring_depth) {
    set_thread_error("texture_count exceeds MLN_TEXTURE_RING_MAX_DEPTH");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  for (size_t index = 0; index < count; ++index) {
    const auto identity = key(textures[index]);
    if (identity == decltype(identity){}) {
      set_thread_error(null_message);
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    for (size_t earlier = 0; earlier < index; ++earlier) {
      if (key(textures[earlier]) == identity) {
        set_thread_error("textures must name distinct textures");
        return MLN_STATUS_INVALID_ARGUMENT;
      }
    }
  }
  return MLN_STATUS_OK;
}

}  // namespace

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
  const auto extent_status = validate_logical_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  return validate_webgpu_context(descriptor->context);
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
  const auto extent_status = validate_logical_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status = validate_webgpu_context(descriptor->context);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  const auto textures_status = validate_borrowed_textures(
    descriptor->textures, descriptor->texture_count,
    [](const mln_webgpu_borrowed_texture& entry) { return entry.texture; },
    "WebGPU texture and texture_view must not be null"
  );
  if (textures_status != MLN_STATUS_OK) {
    return textures_status;
  }
  for (size_t index = 0; index < descriptor->texture_count; ++index) {
    if (descriptor->textures[index].texture_view == nullptr) {
      set_thread_error("WebGPU texture and texture_view must not be null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  // WGPUTextureFormat_Undefined is zero; the WebGPU build asserts that.
  if (descriptor->format == 0) {
    set_thread_error("WebGPU texture format must be specified");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_metal_owned_texture_descriptor(
  const mln_metal_owned_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_metal_owned_texture_descriptor)) {
    set_thread_error("mln_metal_owned_texture_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_logical_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  if (
    descriptor->pixel_format != metal_rgba8_unorm &&
    descriptor->pixel_format != metal_bgra8_unorm
  ) {
    set_thread_error(
      "Metal texture pixel format must be RGBA8Unorm or BGRA8Unorm"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return validate_metal_context(descriptor->context, true);
}

auto validate_metal_borrowed_texture_descriptor(
  const mln_metal_borrowed_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_metal_borrowed_texture_descriptor)) {
    set_thread_error("mln_metal_borrowed_texture_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_logical_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  return validate_borrowed_textures(
    descriptor->textures, descriptor->texture_count,
    [](const mln_metal_borrowed_texture& entry) { return entry.texture; },
    "Metal texture must not be null"
  );
}

auto validate_vulkan_owned_texture_descriptor(
  const mln_vulkan_owned_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_vulkan_owned_texture_descriptor)) {
    set_thread_error("mln_vulkan_owned_texture_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_logical_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  return validate_vulkan_context(
    descriptor->context, "Vulkan handles must not be null"
  );
}

auto validate_vulkan_borrowed_texture_descriptor(
  const mln_vulkan_borrowed_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_vulkan_borrowed_texture_descriptor)) {
    set_thread_error(
      "mln_vulkan_borrowed_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_logical_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status = validate_vulkan_context(
    descriptor->context, "Vulkan handles must not be null"
  );
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  const auto textures_status = validate_borrowed_textures(
    descriptor->textures, descriptor->texture_count,
    [](const mln_vulkan_borrowed_texture& entry) { return entry.image; },
    "Vulkan handles must not be null"
  );
  if (textures_status != MLN_STATUS_OK) {
    return textures_status;
  }
  for (size_t index = 0; index < descriptor->texture_count; ++index) {
    if (
      descriptor->textures[index].image_view ==
      MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL
    ) {
      set_thread_error("Vulkan handles must not be null");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
  }
  // VK_FORMAT_UNDEFINED and VK_IMAGE_LAYOUT_UNDEFINED are both zero; the Vulkan
  // build asserts that, since this file is built without the Vulkan headers.
  if (descriptor->format == 0 || descriptor->final_layout == 0) {
    set_thread_error("Vulkan format and final_layout must be specified");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

// A shared owned target exposes frames to a host context. A private owned
// target keeps every GL object on its core worker and exposes CPU readback
// instead. EGL can create that worker-local context from a display and config;
// transferred WebGL creates it from the canvas on the worker itself.
auto validate_owned_texture_ownership(
  const mln_opengl_context_descriptor& context
) -> mln_status {
  if (context.ownership == MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED) {
    return MLN_STATUS_OK;
  }
  if (context.platform == MLN_OPENGL_CONTEXT_PLATFORM_EGL) {
    return MLN_STATUS_OK;
  }
  if (
    context.platform == MLN_OPENGL_CONTEXT_PLATFORM_WEBGL &&
    context.data.webgl.kind == MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS
  ) {
    return MLN_STATUS_OK;
  }
  set_thread_error(
    "dedicated OpenGL owned textures require EGL or a transferred WebGL canvas"
  );
  return MLN_STATUS_UNSUPPORTED;
}

// A borrowed target must share objects with the host context that owns its
// texture. Dedicated ownership names no share group.
auto validate_borrowed_texture_ownership(
  const mln_opengl_context_descriptor& context
) -> mln_status {
  if (context.ownership == MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED) {
    return MLN_STATUS_OK;
  }
  set_thread_error(
    "a borrowed OpenGL texture requires a context shared with its host owner"
  );
  return MLN_STATUS_INVALID_ARGUMENT;
}

auto validate_opengl_owned_texture_descriptor(
  const mln_opengl_owned_texture_descriptor* descriptor,
  bool require_supported_provider
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_opengl_owned_texture_descriptor)) {
    set_thread_error("mln_opengl_owned_texture_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_logical_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status =
    validate_opengl_context(descriptor->context, require_supported_provider);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  return validate_owned_texture_ownership(descriptor->context);
}

auto validate_opengl_borrowed_texture_descriptor(
  const mln_opengl_borrowed_texture_descriptor* descriptor,
  bool require_supported_provider
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_opengl_borrowed_texture_descriptor)) {
    set_thread_error(
      "mln_opengl_borrowed_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_logical_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status =
    validate_opengl_context(descriptor->context, require_supported_provider);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  const auto ownership_status =
    validate_borrowed_texture_ownership(descriptor->context);
  if (ownership_status != MLN_STATUS_OK) {
    return ownership_status;
  }
  if (descriptor->target == 0) {
    set_thread_error("OpenGL texture and target must be specified");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return validate_borrowed_textures(
    descriptor->textures, descriptor->texture_count,
    [](const mln_opengl_borrowed_texture& entry) { return entry.texture; },
    "OpenGL texture and target must be specified"
  );
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

namespace {

// A readback that the backend finished before returning it.
class CompletedTextureReadback final : public TextureReadback {
 public:
  explicit CompletedTextureReadback(mln::PremultipliedImage image)
      : image_(std::move(image)) {}

  [[nodiscard]] auto ready() const noexcept -> bool override { return true; }

  auto take() -> mln::PremultipliedImage override {
    if (!image_.valid()) {
      set_thread_error("texture readback did not produce an image");
    }
    return std::move(image_);
  }

 private:
  mln::PremultipliedImage image_;
};

// Checks that `texture` can be read back now and describes the image that a
// readback produces.
auto prepare_readback(
  mln_render_session texture, mln_render_session_object*& out_live,
  mln_texture_image_info& out_info
) -> mln_status {
  mln_render_session_object* live = nullptr;
  const auto status = validate_live_attached_texture(texture, live);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (live->texture.mode != TextureSessionMode::Owned) {
    set_thread_error("texture session does not support CPU readback");
    return MLN_STATUS_UNSUPPORTED;
  }
  if (!live->texture.backend->supports_readback()) {
    set_thread_error("render backend does not support CPU readback");
    return MLN_STATUS_UNSUPPORTED;
  }
  if (live->rendered_target_generation != live->generation) {
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

  out_live = live;
  out_info = mln_texture_image_info{
    .width = live->physical_width,
    .height = live->physical_height,
    .stride = stride,
    .byte_length = static_cast<size_t>(stride) * live->physical_height
  };
  return MLN_STATUS_OK;
}

struct TextureReadbackResult {
  std::string bytes;
  mln_texture_image_info info{};
};

// Checks a finished readback against the image that `info` describes, which
// the readback captured when it started, and copies it into `out_result`.
auto readback_result(
  const mln_texture_image_info& info, const mln::PremultipliedImage& image,
  std::any& out_result
) -> mln_status {
  if (!image.valid()) {
    return MLN_STATUS_NATIVE_ERROR;
  }
  if (
    image.size.width != info.width || image.size.height != info.height ||
    image.stride() != info.stride || image.bytes() != info.byte_length
  ) {
    set_thread_error("texture readback image layout did not match the session");
    return MLN_STATUS_NATIVE_ERROR;
  }
  out_result = TextureReadbackResult{
    .bytes = std::string(
      reinterpret_cast<const char*>(image.data.get()), image.bytes()
    ),
    .info = info,
  };
  return MLN_STATUS_OK;
}

}  // namespace

auto TextureSessionBackend::start_readback(std::function<void()> on_ready)
  -> std::shared_ptr<TextureReadback> {
  (void)on_ready;
  return std::make_shared<CompletedTextureReadback>(
    headless_backend().readStillImage()
  );
}

auto texture_read_premultiplied_rgba8_start(
  mln_render_session texture, const mln_completion* completion
) -> mln_status {
  return enqueue_driver_result_operation(
    texture,
    [](mln_render_session_object& target, std::any& result) {
      mln_render_session_object* live = nullptr;
      auto info = mln_texture_image_info{};
      const auto status = prepare_readback(target.self, live, info);
      if (status != MLN_STATUS_OK) return status;

      auto* renderer_backend = live->texture.backend->renderer_backend();
      if (renderer_backend == nullptr) {
        set_thread_error("texture session renderer backend is not available");
        return MLN_STATUS_NATIVE_ERROR;
      }
      auto readback = std::shared_ptr<TextureReadback>{};
      {
        auto current = ScopedCurrentScheduler{live->scheduler};
        auto guard = mln::gfx::BackendScope{*renderer_backend};
        readback = live->texture.backend->start_readback(
          [session = live->weak_from_this()]() { resume_driver_work(session); }
        );
      }
      if (readback == nullptr) return MLN_STATUS_NATIVE_ERROR;
      if (readback->ready()) {
        return readback_result(info, readback->take(), result);
      }
      // The copy is already submitted, so a later generation cannot change
      // what it reads, and the finish checks the image against `info`.
      result = PendingDriverResult{
        .ready = [readback]() { return readback->ready(); },
        .finish =
          [readback, info](std::any& finished) {
            return readback_result(info, readback->take(), finished);
          },
      };
      return MLN_STATUS_OK;
    },
    completion,
    [](
      const std::shared_ptr<Completion>& state, mln_status status,
      std::string diagnostic, std::any result
    ) {
      auto* readback = std::any_cast<TextureReadbackResult>(&result);
      if (status == MLN_STATUS_OK && readback != nullptr) {
        state->resolve([bytes = std::move(readback->bytes),
                        info =
                          readback->info](const CompletionSink& descriptor) {
          const auto value = mln_texture_readback_result{
            .data = {.data = bytes.data(), .size = bytes.size()},
            .info = info,
          };
          CompletionValue<&mln_render_session_read_texture>::deliver(
            descriptor, value
          );
        });
      } else {
        complete_failure(
          state, status == MLN_STATUS_OK ? MLN_STATUS_NATIVE_ERROR : status,
          status == MLN_STATUS_OK
            ? "texture readback produced an invalid result"
            : std::move(diagnostic)
        );
      }
    }
  );
}

namespace {

template <typename Frame>
auto acquired_frame_get_backend(mln_acquired_frame handle, Frame* out_frame)
  -> mln_status {
  if (out_frame == nullptr || out_frame->size < sizeof(Frame)) {
    set_thread_error("out_frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto frame = std::shared_ptr<mln_acquired_frame_object>{};
  if (
    const auto status = lease_valid_acquired_frame(handle, frame);
    status != MLN_STATUS_OK
  ) {
    return status;
  }
  const auto* metadata = std::any_cast<Frame>(&frame->backend_metadata);
  if (metadata == nullptr) {
    set_thread_error("acquired frame belongs to a different render backend");
    return MLN_STATUS_UNSUPPORTED;
  }
  *out_frame = *metadata;
  return MLN_STATUS_OK;
}

}  // namespace

auto acquired_frame_get_metal_texture(
  mln_acquired_frame frame, mln_metal_texture_frame* out_frame
) -> mln_status {
  return acquired_frame_get_backend(frame, out_frame);
}

auto acquired_frame_get_vulkan_texture(
  mln_acquired_frame frame, mln_vulkan_texture_frame* out_frame
) -> mln_status {
  return acquired_frame_get_backend(frame, out_frame);
}

auto acquired_frame_get_opengl_texture(
  mln_acquired_frame frame, mln_opengl_texture_frame* out_frame
) -> mln_status {
  return acquired_frame_get_backend(frame, out_frame);
}

auto acquired_frame_get_webgpu_texture(
  mln_acquired_frame frame, mln_webgpu_texture_frame* out_frame
) -> mln_status {
  return acquired_frame_get_backend(frame, out_frame);
}

}  // namespace mln::core
