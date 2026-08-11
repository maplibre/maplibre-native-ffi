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
    .image = nullptr,
    .image_view = nullptr,
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
  if (descriptor->image == nullptr || descriptor->image_view == nullptr) {
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

// A texture session exists to hand its texture to the host, which needs the
// session context in the host's share group. Dedicated ownership names no share
// group, so it has no meaning here.
auto validate_shared_texture_ownership(
  const mln_opengl_context_descriptor& context
) -> mln_status {
  if (context.ownership == MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED) {
    set_thread_error(
      "an OpenGL texture session shares its context with the host that samples "
      "the texture"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
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
  return validate_shared_texture_ownership(descriptor->context);
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
    validate_shared_texture_ownership(descriptor->context);
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
  if (live->texture.acquired) {
    set_thread_error("cannot read while a texture frame is acquired");
    return MLN_STATUS_INVALID_STATE;
  }
  if (live->texture.mode != TextureSessionMode::Owned) {
    set_thread_error("texture session does not support CPU readback");
    return MLN_STATUS_UNSUPPORTED;
  }
  if (!live->texture.backend->supports_readback()) {
    set_thread_error("render backend does not support CPU readback");
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
