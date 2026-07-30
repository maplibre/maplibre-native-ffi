/**
 * @file maplibre_native_c/render_target.h
 * Shared public C API declarations for render target descriptors.
 */

#ifndef MAPLIBRE_NATIVE_C_RENDER_TARGET_H
#define MAPLIBRE_NATIVE_C_RENDER_TARGET_H

#include <stdint.h>

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Logical render target extent in UI pixels. */
typedef struct mln_render_target_extent {
  uint32_t size;
  /** Logical map width in UI pixels. */
  uint32_t width;
  /** Logical map height in UI pixels. */
  uint32_t height;
  /** UI-to-device pixel scale. Must be positive and finite. */
  double scale_factor;
} mln_render_target_extent;

/** Metal backend context fields shared by Metal render targets. */
typedef struct mln_metal_context_descriptor {
  uint32_t size;
  /** id<MTLDevice> / MTL::Device*. Retained when the target requires it. */
  void* device;
} mln_metal_context_descriptor;

/** Vulkan backend context fields shared by Vulkan render targets. */
typedef struct mln_vulkan_context_descriptor {
  uint32_t size;
  /** Borrowed VkInstance. Required. */
  void* instance;
  /** Borrowed VkPhysicalDevice. Required. */
  void* physical_device;
  /** Borrowed VkDevice. Required. */
  void* device;
  /** Borrowed graphics VkQueue. Required. */
  void* graphics_queue;
  /** Queue family index for graphics_queue. Must support graphics commands. */
  uint32_t graphics_queue_family_index;
  /** PFN_vkGetInstanceProcAddr for the loader that created the Vulkan handles.
   */
  void* get_instance_proc_addr;
  /** PFN_vkGetDeviceProcAddr for the loader that created the Vulkan device. */
  void* get_device_proc_addr;
} mln_vulkan_context_descriptor;

/** OpenGL context providers supported by this build. */
typedef enum mln_opengl_context_provider_flag : uint32_t {
  MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL = 1u << 0u,
  MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL = 1u << 1u,
  /** Browser WebGL context imported into an Emscripten module. */
  MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WEBGL = 1u << 2u,
} mln_opengl_context_provider_flag;

/** OpenGL platform context provider used by a context descriptor. */
typedef enum mln_opengl_context_platform : uint32_t {
  /** No OpenGL context provider is selected. */
  MLN_OPENGL_CONTEXT_PLATFORM_UNSPECIFIED = 0u,
  MLN_OPENGL_CONTEXT_PLATFORM_WGL = 1u,
  MLN_OPENGL_CONTEXT_PLATFORM_EGL = 2u,
  /** Emscripten WebGL context handle. */
  MLN_OPENGL_CONTEXT_PLATFORM_WEBGL = 3u,
} mln_opengl_context_platform;

/** WGL context fields shared by OpenGL render targets on Windows. */
typedef struct mln_wgl_context_descriptor {
  uint32_t size;
  /** Borrowed HDC used to create a shared session context. Required. */
  void* device_context;
  /** Borrowed HGLRC whose share group the session context joins. Required. */
  void* share_context;
  /** Optional wglGetProcAddress-compatible function for the host loader. */
  void* get_proc_address;
} mln_wgl_context_descriptor;

/** EGL context fields shared by OpenGL render targets. */
typedef struct mln_egl_context_descriptor {
  uint32_t size;
  /** Borrowed EGLDisplay. Required. */
  void* display;
  /**
   * Borrowed EGLConfig used to create a shared session context. Required.
   * OpenGL texture sessions require EGL_SURFACE_TYPE to include
   * EGL_PBUFFER_BIT.
   */
  void* config;
  /**
   * Borrowed EGLContext whose share group the session context joins. Required.
   */
  void* share_context;
  /** Optional eglGetProcAddress-compatible function for the host loader. */
  void* get_proc_address;
} mln_egl_context_descriptor;

/** WebGL context fields shared by OpenGL render targets in Emscripten. */
typedef struct mln_webgl_context_descriptor {
  uint32_t size;
  /** Borrowed EMSCRIPTEN_WEBGL_CONTEXT_HANDLE. Must be positive. */
  int32_t context;
} mln_webgl_context_descriptor;

/** OpenGL backend context fields shared by OpenGL render targets. */
typedef struct mln_opengl_context_descriptor {
  uint32_t size;
  /** WGL or EGL context provider. */
  mln_opengl_context_platform platform;
  union {
    mln_wgl_context_descriptor wgl;
    mln_egl_context_descriptor egl;
    mln_webgl_context_descriptor webgl;
  } data;
} mln_opengl_context_descriptor;

/** WebGPU backend context fields shared by WebGPU render targets. */
typedef struct mln_webgpu_context_descriptor {
  uint32_t size;
  /** Borrowed WGPUInstance. Optional for texture sessions. */
  void* instance;
  /** Borrowed WGPUDevice. Required. */
  void* device;
  /**
   * Borrowed WGPUQueue. Optional; null uses the device default queue. A
   * non-null queue must belong to device.
   */
  void* queue;
} mln_webgpu_context_descriptor;

/**
 * Computes the physical device-pixel size of a logical render target extent.
 *
 * Each dimension is ceil(logical * scale_factor). Session-owned texture targets
 * and surface targets are sized this way, so callers that allocate host
 * resources alongside them can use this instead of repeating the formula.
 *
 * Caller-owned borrowed texture targets state their physical size directly and
 * do not derive it; see texture.h. Not every physical size is reachable from a
 * logical extent, so a caller that starts from a physical size it does not
 * control states that size rather than solving for a logical extent.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when extent is null or invalid, out_width or
 *   out_height is null, or the scaled dimensions are too large.
 */
MLN_API mln_status mln_render_target_extent_physical_size(
  const mln_render_target_extent* extent, uint32_t* out_width,
  uint32_t* out_height
) MLN_NOEXCEPT;

/**
 * Returns OpenGL context providers supported by this build.
 */
MLN_API uint32_t mln_opengl_supported_context_provider_mask(void) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_RENDER_TARGET_H
