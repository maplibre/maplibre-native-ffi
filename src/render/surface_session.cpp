#include "render/surface_session.hpp"

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

namespace mln::core {

auto metal_surface_descriptor_default() noexcept
  -> mln_metal_surface_descriptor {
  return mln_metal_surface_descriptor{
    .size = sizeof(mln_metal_surface_descriptor),
    .extent =
      mln_render_target_extent{
        .size = sizeof(mln_render_target_extent),
        .width = 256,
        .height = 256,
        .scale_factor = 1.0,
      },
    .context =
      mln_metal_context_descriptor{
        .size = sizeof(mln_metal_context_descriptor),
        .device = nullptr,
      },
    .layer = nullptr,
  };
}

auto vulkan_surface_descriptor_default() noexcept
  -> mln_vulkan_surface_descriptor {
  return mln_vulkan_surface_descriptor{
    .size = sizeof(mln_vulkan_surface_descriptor),
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
    .surface = nullptr,
  };
}

auto validate_metal_surface_descriptor(
  const mln_metal_surface_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_metal_surface_descriptor)) {
    set_thread_error("mln_metal_surface_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_render_target_extent(
    descriptor->extent, "surface dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status =
    validate_metal_context(descriptor->context, false);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->layer == nullptr) {
    set_thread_error("Metal surface layer must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_vulkan_surface_descriptor(
  const mln_vulkan_surface_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_vulkan_surface_descriptor)) {
    set_thread_error("mln_vulkan_surface_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_render_target_extent(
    descriptor->extent, "surface dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status = validate_vulkan_context(
    descriptor->context, "Vulkan surface handles must not be null"
  );
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->surface == nullptr) {
    set_thread_error("Vulkan surface handles must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_webgpu_surface_descriptor(
  const mln_webgpu_surface_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_webgpu_surface_descriptor)) {
    set_thread_error("mln_webgpu_surface_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_render_target_extent(
    descriptor->extent, "surface dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status = validate_webgpu_context(descriptor->context);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->surface == nullptr) {
    set_thread_error("WebGPU surface must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  // Zero is WGPUTextureFormat_Undefined, which configure rejects.
  if (descriptor->format == 0) {
    set_thread_error("WebGPU surface format must be specified");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_opengl_surface_descriptor(
  const mln_opengl_surface_descriptor* descriptor,
  bool require_supported_provider
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_opengl_surface_descriptor)) {
    set_thread_error("mln_opengl_surface_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto extent_status = validate_render_target_extent(
    descriptor->extent, "surface dimensions and scale_factor must be positive"
  );
  if (extent_status != MLN_STATUS_OK) {
    return extent_status;
  }
  const auto context_status =
    validate_opengl_context(descriptor->context, require_supported_provider);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  // WGL and EGL need a surface to make a context current, so they carry one
  // alongside the context. A WebGL context already names its canvas, so the
  // field is rejected rather than ignored.
  if (descriptor->context.platform == MLN_OPENGL_CONTEXT_PLATFORM_WEBGL) {
    if (descriptor->surface != nullptr) {
      set_thread_error(
        "WebGL surface must be null; the context names its canvas"
      );
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return MLN_STATUS_OK;
  }
  if (descriptor->surface == nullptr) {
    set_thread_error("OpenGL surface must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

}  // namespace mln::core
