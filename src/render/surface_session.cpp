#include "render/surface_session.hpp"

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"
#include "render/render_session_common.hpp"

namespace mln::core {

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
  const auto size_status = validate_logical_size(
    descriptor->logical_size, "surface dimensions must be positive"
  );
  if (size_status != MLN_STATUS_OK) {
    return size_status;
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
  const auto size_status = validate_logical_size(
    descriptor->logical_size, "surface dimensions must be positive"
  );
  if (size_status != MLN_STATUS_OK) {
    return size_status;
  }
  const auto context_status = validate_vulkan_context(
    descriptor->context, "Vulkan surface handles must not be null"
  );
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  if (descriptor->surface == MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL) {
    set_thread_error("Vulkan surface handles must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_webgpu_surface_descriptor(
  const mln_web_gpu_surface_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_web_gpu_surface_descriptor)) {
    set_thread_error("mln_web_gpu_surface_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto size_status = validate_logical_size(
    descriptor->logical_size, "surface dimensions must be positive"
  );
  if (size_status != MLN_STATUS_OK) {
    return size_status;
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
  const mln_open_gl_surface_descriptor* descriptor,
  bool require_supported_provider
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_open_gl_surface_descriptor)) {
    set_thread_error("mln_open_gl_surface_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto size_status = validate_logical_size(
    descriptor->logical_size, "surface dimensions must be positive"
  );
  if (size_status != MLN_STATUS_OK) {
    return size_status;
  }
  const auto context_status =
    validate_opengl_context(descriptor->context, require_supported_provider);
  if (context_status != MLN_STATUS_OK) {
    return context_status;
  }
  // WGL and EGL need a surface to make a context current, so they carry one
  // alongside the context. A WebGL context already names its canvas, so the
  // field is rejected rather than ignored.
  if (descriptor->context.platform == MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL) {
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

namespace {

template <typename Replacement>
auto validate_surface_replacement(
  const Replacement* replacement, const char* size_message
) -> mln_status {
  if (replacement == nullptr) {
    set_thread_error("surface replacement must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (replacement->size < sizeof(Replacement)) {
    set_thread_error(size_message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return validate_logical_size(
    replacement->logical_size, "surface dimensions must be positive"
  );
}

// An Android target names its window by pointer or by Java Surface, never
// both.
template <typename Target>
auto validate_android_window(const Target& target) -> mln_status {
  if (target.window != nullptr) {
    if (target.jni_env == nullptr && target.surface == nullptr) {
      return MLN_STATUS_OK;
    }
    set_thread_error(
      "name the Android window by window, or by jni_env and surface"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (target.jni_env == nullptr || target.surface == nullptr) {
    set_thread_error(
      "an Android surface needs window, or both jni_env and surface"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

}  // namespace

auto validate_metal_surface_replacement(
  const mln_metal_surface_replacement* replacement
) -> mln_status {
  const auto status = validate_surface_replacement(
    replacement, "mln_metal_surface_replacement.size is too small"
  );
  if (status != MLN_STATUS_OK) return status;
  if (replacement->layer == nullptr) {
    set_thread_error("Metal surface layer must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_vulkan_surface_replacement(
  const mln_vulkan_surface_replacement* replacement
) -> mln_status {
  const auto status = validate_surface_replacement(
    replacement, "mln_vulkan_surface_replacement.size is too small"
  );
  if (status != MLN_STATUS_OK) return status;
  if (replacement->surface == MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL) {
    set_thread_error("Vulkan surface handles must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_opengl_surface_replacement(
  const mln_open_gl_surface_replacement* replacement
) -> mln_status {
  return validate_surface_replacement(
    replacement, "mln_open_gl_surface_replacement.size is too small"
  );
}

auto validate_webgpu_surface_replacement(
  const mln_web_gpu_surface_replacement* replacement
) -> mln_status {
  const auto status = validate_surface_replacement(
    replacement, "mln_web_gpu_surface_replacement.size is too small"
  );
  if (status != MLN_STATUS_OK) return status;
  if (replacement->surface == nullptr) {
    set_thread_error("WebGPU surface must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

auto validate_android_surface_descriptor(
  const mln_android_surface_descriptor* descriptor
) -> mln_status {
  if (descriptor == nullptr) {
    set_thread_error("surface descriptor must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (descriptor->size < sizeof(mln_android_surface_descriptor)) {
    set_thread_error("mln_android_surface_descriptor.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const auto size_status = validate_logical_size(
    descriptor->logical_size, "surface dimensions must be positive"
  );
  if (size_status != MLN_STATUS_OK) return size_status;
  return validate_android_window(*descriptor);
}

auto validate_android_surface_replacement(
  const mln_android_surface_replacement* replacement
) -> mln_status {
  const auto status = validate_surface_replacement(
    replacement, "mln_android_surface_replacement.size is too small"
  );
  if (status != MLN_STATUS_OK) return status;
  return validate_android_window(*replacement);
}

}  // namespace mln::core
