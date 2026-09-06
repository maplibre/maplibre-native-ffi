#include <any>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

#include <mln/gfx/backend_scope.hpp>
#include <mln/util/image.hpp>

#include "render/texture_session.hpp"

#include "bytes/buffer.hpp"
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
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context = mln_metal_context_descriptor{
      .size = sizeof(mln_metal_context_descriptor),
      .device = nullptr,
    },
  };
}

auto metal_borrowed_texture_descriptor_default() noexcept
  -> mln_metal_borrowed_texture_descriptor {
  return mln_metal_borrowed_texture_descriptor{
    .size = sizeof(mln_metal_borrowed_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .physical_width = 256,
    .physical_height = 256,
    .texture = nullptr,
  };
}

auto vulkan_owned_texture_descriptor_default() noexcept
  -> mln_vulkan_owned_texture_descriptor {
  return mln_vulkan_owned_texture_descriptor{
    .size = sizeof(mln_vulkan_owned_texture_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context = mln_vulkan_context_descriptor{
      .size = sizeof(mln_vulkan_context_descriptor),
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
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .physical_width = 256,
    .physical_height = 256,
    .context =
      mln_vulkan_context_descriptor{
        .size = sizeof(mln_vulkan_context_descriptor),
        .instance = nullptr,
        .physical_device = nullptr,
        .device = nullptr,
        .graphics_queue = nullptr,
        .graphics_queue_family_index = 0,
        .get_instance_proc_addr = nullptr,
        .get_device_proc_addr = nullptr,
      },
    .image = MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL,
    .image_view = MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL,
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
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context = mln_webgpu_context_descriptor{
      .size = sizeof(mln_webgpu_context_descriptor),
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
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .physical_width = 256,
    .physical_height = 256,
    .context =
      mln_webgpu_context_descriptor{
        .size = sizeof(mln_webgpu_context_descriptor),
        .instance = nullptr,
        .device = nullptr,
        .queue = nullptr,
      },
    .texture = nullptr,
    .texture_view = nullptr,
    .format = 0,
  };
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
  const auto extent_status = validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status = validate_webgpu_context(descriptor->context);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->texture == nullptr || descriptor->texture_view == nullptr) {
    set_thread_error("WebGPU texture and texture_view must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
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
  const auto extent_status = validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
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
  const auto extent_status = validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  if (descriptor->texture == nullptr) {
    set_thread_error("Metal texture must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
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
  const auto extent_status = validate_render_target_extent(
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
  const auto extent_status = validate_render_target_extent(
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
  if (
    descriptor->image == MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL ||
    descriptor->image_view == MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL
  ) {
    set_thread_error("Vulkan handles must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
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
  const auto extent_status = validate_render_target_extent(
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
  const auto extent_status = validate_render_target_extent(
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
  if (descriptor->texture == 0 || descriptor->target == 0) {
    set_thread_error("OpenGL texture and target must be specified");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

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
  const auto byte_length = static_cast<size_t>(stride) * live->physical_height;

  *out_info = mln_texture_image_info{
    .size = sizeof(mln_texture_image_info),
    .width = live->physical_width,
    .height = live->physical_height,
    .stride = stride,
    .byte_length = byte_length
  };

  // A null buffer with zero capacity is a size probe: out_info already carries
  // the required byte length, so report it and succeed.
  if (out_data == nullptr && out_data_capacity == 0) {
    return MLN_STATUS_OK;
  }
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
  auto guard = mln::gfx::BackendScope{*renderer_backend};
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

struct TextureReadbackResult {
  std::string bytes;
  mln_texture_image_info info{};
};

auto texture_read_premultiplied_rgba8_start(
  mln_render_session texture, const mln_completion* completion
) -> mln_status {
  return enqueue_driver_result_operation(
    texture,
    [](mln_render_session_object& target, std::any& result) {
      auto readback = TextureReadbackResult{
        .bytes = {},
        .info = texture_image_info_default(),
      };
      auto status = texture_read_premultiplied_rgba8(
        target.self, nullptr, 0, &readback.info
      );
      if (status != MLN_STATUS_OK) return status;
      readback.bytes.resize(readback.info.byte_length);
      status = texture_read_premultiplied_rgba8(
        target.self, reinterpret_cast<uint8_t*>(readback.bytes.data()),
        readback.bytes.size(), &readback.info
      );
      if (status == MLN_STATUS_OK) result = std::move(readback);
      return status;
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
                          readback->info](const mln_completion& descriptor) {
          const auto value = mln_texture_readback_result{
            .size = sizeof(mln_texture_readback_result),
            .reserved = 0,
            .data = {.data = bytes.data(), .size = bytes.size()},
            .info = info,
          };
          invoke_completion(
            descriptor, MLN_STATUS_OK, MLN_COMMAND_DISPOSITION_COMMITTED, 0, {},
            &value, 1
          );
        });
      } else {
        complete(
          state, status == MLN_STATUS_OK ? MLN_STATUS_NATIVE_ERROR : status,
          status == MLN_STATUS_OK
            ? "texture readback produced an invalid result"
            : std::move(diagnostic)
        );
      }
    }
  );
}

template <typename Frame>
auto acquired_frame_get_backend(mln_acquired_frame handle, Frame* out_frame)
  -> mln_status {
  if (out_frame == nullptr || out_frame->size < sizeof(Frame)) {
    set_thread_error("out_frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto frame = handle_table<mln_acquired_frame_object>().lease(handle);
  if (frame == nullptr || !frame->valid.load()) {
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  {
    const auto lock = std::scoped_lock{frame->session->control_mutex};
    if (
      frame->session->state == MLN_RENDER_SESSION_STATE_ABANDONED ||
      frame->session->state == MLN_RENDER_SESSION_STATE_TARGET_LOST
    ) {
      return MLN_STATUS_TARGET_LOST;
    }
  }
  const auto* metadata = std::any_cast<Frame>(&frame->backend_metadata);
  if (metadata == nullptr) {
    set_thread_error("acquired frame belongs to a different render backend");
    return MLN_STATUS_UNSUPPORTED;
  }
  *out_frame = *metadata;
  return MLN_STATUS_OK;
}

auto acquired_frame_get_metal_texture(
  mln_acquired_frame frame, mln_metal_owned_texture_frame* out_frame
) -> mln_status {
  return acquired_frame_get_backend(frame, out_frame);
}

auto acquired_frame_get_vulkan_texture(
  mln_acquired_frame frame, mln_vulkan_owned_texture_frame* out_frame
) -> mln_status {
  return acquired_frame_get_backend(frame, out_frame);
}

auto acquired_frame_get_opengl_texture(
  mln_acquired_frame frame, mln_opengl_owned_texture_frame* out_frame
) -> mln_status {
  return acquired_frame_get_backend(frame, out_frame);
}

auto acquired_frame_get_webgpu_texture(
  mln_acquired_frame frame, mln_webgpu_owned_texture_frame* out_frame
) -> mln_status {
  return acquired_frame_get_backend(frame, out_frame);
}

}  // namespace mln::core
