#include <cstddef>

#include "diagnostics/diagnostics.hpp"
#include "map/map.hpp"
#include "maplibre_native_c/base.h"
#include "render/render_session_common.hpp"
#include "render/texture_session.hpp"

namespace {
auto validate_metal_owned_descriptor(
  const mln_metal_owned_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_metal_owned_texture_descriptor)) {
    mln::core::set_thread_error(
      "mln_metal_owned_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  return mln::core::validate_metal_context(descriptor->context, true);
}

auto validate_metal_borrowed_descriptor(
  const mln_metal_borrowed_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_metal_borrowed_texture_descriptor)) {
    mln::core::set_thread_error(
      "mln_metal_borrowed_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  if (descriptor->texture == nullptr) {
    mln::core::set_thread_error("Metal texture must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_vulkan_owned_descriptor(
  const mln_vulkan_owned_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_vulkan_owned_texture_descriptor)) {
    mln::core::set_thread_error(
      "mln_vulkan_owned_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  return mln::core::validate_vulkan_context(
    descriptor->context, "Vulkan handles must not be null"
  );
}

auto validate_vulkan_borrowed_descriptor(
  const mln_vulkan_borrowed_texture_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    mln::core::set_thread_error("texture descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_vulkan_borrowed_texture_descriptor)) {
    mln::core::set_thread_error(
      "mln_vulkan_borrowed_texture_descriptor.size is too small"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = mln::core::validate_render_target_extent(
    descriptor->extent, "texture dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status = mln::core::validate_vulkan_context(
    descriptor->context, "Vulkan handles must not be null"
  );
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->image == nullptr || descriptor->image_view == nullptr) {
    mln::core::set_thread_error("Vulkan handles must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->format == 0 || descriptor->final_layout == 0) {
    mln::core::set_thread_error(
      "Vulkan format and final_layout must be specified"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_frame(
  mln_render_session* texture, const void* frame, size_t frame_size,
  const char* message
) -> mln_status {
  const auto status = mln::core::validate_texture(texture);
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (frame == nullptr || frame_size == 0) {
    mln::core::set_thread_error(message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}
}  // namespace

namespace mln::core {

auto supported_render_backend_mask() noexcept -> uint32_t {
  return MLN_RENDER_BACKEND_FLAG_OPENGL;
}

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

auto metal_owned_texture_attach(
  mln_map* map, const mln_metal_owned_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_metal_owned_descriptor(descriptor);
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
  set_thread_error("Metal texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto metal_borrowed_texture_attach(
  mln_map* map, const mln_metal_borrowed_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_metal_borrowed_descriptor(descriptor);
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
  set_thread_error("Metal texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_owned_texture_attach(
  mln_map* map, const mln_vulkan_owned_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status = validate_vulkan_owned_descriptor(descriptor);
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
  set_thread_error("Vulkan texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_borrowed_texture_attach(
  mln_map* map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  mln_render_session** out_session
) -> mln_status {
  const auto map_status = validate_map(map);
  if (map_status != MLN_STATUS_OK) {
    return map_status;
  }
  const auto descriptor_status =
    validate_vulkan_borrowed_descriptor(descriptor);
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
  set_thread_error("Vulkan texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto metal_owned_texture_acquire_frame(
  mln_render_session* texture, mln_metal_owned_texture_frame* out_frame
) -> mln_status {
  const auto status = validate_frame(
    texture, out_frame, out_frame == nullptr ? 0 : out_frame->size,
    "out_frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_frame->size < sizeof(mln_metal_owned_texture_frame)) {
    set_thread_error("out_frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  set_thread_error("Metal texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto metal_owned_texture_release_frame(
  mln_render_session* texture, const mln_metal_owned_texture_frame* frame
) -> mln_status {
  const auto status = validate_frame(
    texture, frame, frame == nullptr ? 0 : frame->size,
    "frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (frame->size < sizeof(mln_metal_owned_texture_frame)) {
    set_thread_error("frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  set_thread_error("Metal texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_owned_texture_acquire_frame(
  mln_render_session* texture, mln_vulkan_owned_texture_frame* out_frame
) -> mln_status {
  const auto status = validate_frame(
    texture, out_frame, out_frame == nullptr ? 0 : out_frame->size,
    "out_frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (out_frame->size < sizeof(mln_vulkan_owned_texture_frame)) {
    set_thread_error("out_frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  set_thread_error("Vulkan texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

auto vulkan_owned_texture_release_frame(
  mln_render_session* texture, const mln_vulkan_owned_texture_frame* frame
) -> mln_status {
  const auto status = validate_frame(
    texture, frame, frame == nullptr ? 0 : frame->size,
    "frame must not be null and must have a valid size"
  );
  if (status != MLN_STATUS_OK) {
    return status;
  }
  if (frame->size < sizeof(mln_vulkan_owned_texture_frame)) {
    set_thread_error("frame must not be null and must have a valid size");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  set_thread_error("Vulkan texture sessions are not supported by this build");
  return MLN_STATUS_UNSUPPORTED;
}

}  // namespace mln::core
