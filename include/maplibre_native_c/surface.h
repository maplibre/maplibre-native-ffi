/**
 * @file maplibre_native_c/surface.h
 * Public C API declarations for surface render targets.
 */

#ifndef MAPLIBRE_NATIVE_C_SURFACE_H
#define MAPLIBRE_NATIVE_C_SURFACE_H

#include <stdint.h>

#include "base.h"
#include "completion.h"
#include "render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Metal native surface session attachment options. */
typedef struct mln_metal_surface_descriptor {
  uint32_t size;
  /** Logical surface extent. */
  mln_render_target_extent extent;
  /** Metal backend context. device is optional for Metal surfaces. */
  mln_metal_context_descriptor context;
  /** CAMetalLayer* / CA::MetalLayer* retained by the session. Required. */
  void* layer;
} mln_metal_surface_descriptor;

/** Vulkan native surface session attachment options. */
typedef struct mln_vulkan_surface_descriptor {
  uint32_t size;
  /** Logical surface extent. */
  mln_render_target_extent extent;
  /**
   * Borrowed Vulkan context. All handles are required. The device must support
   * VK_KHR_swapchain, and the queue family must support graphics and
   * presentation to this descriptor's surface.
   */
  mln_vulkan_context_descriptor context;
  /** Borrowed VkSurfaceKHR. Required. */
  void* surface;
} mln_vulkan_surface_descriptor;

/** WebGPU native surface session attachment options. */
typedef struct mln_webgpu_surface_descriptor {
  uint32_t size;
  /** Logical surface extent. */
  mln_render_target_extent extent;
  /** Borrowed WebGPU context. device is required. */
  mln_webgpu_context_descriptor context;
  /**
   * Borrowed WGPUSurface. Required, and must stay alive for the session. The
   * session configures it for this device and extent, and unconfigures it when
   * the session ends.
   */
  void* surface;
  /**
   * WGPUTextureFormat to configure the surface with. Required. A browser host
   * takes it from navigator.gpu.getPreferredCanvasFormat().
   */
  uint32_t format;
} mln_webgpu_surface_descriptor;

/** OpenGL native surface session attachment options. */
typedef struct mln_opengl_surface_descriptor {
  uint32_t size;
  /** Logical surface extent. */
  mln_render_target_extent extent;
  /** Borrowed OpenGL context provider data. */
  mln_opengl_context_descriptor context;
  /**
   * Borrowed platform surface handle: an HDC for WGL and an EGLSurface for EGL,
   * both required. Null for WebGL, whose context carries its canvas binding.
   */
  void* surface;
} mln_opengl_surface_descriptor;

/**
 * Returns Metal surface descriptor defaults for this C API version.
 */
MLN_API mln_metal_surface_descriptor
mln_metal_surface_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Returns Vulkan surface descriptor defaults for this C API version.
 */
MLN_API mln_vulkan_surface_descriptor
mln_vulkan_surface_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Returns OpenGL surface descriptor defaults for this C API version.
 */
MLN_API mln_opengl_surface_descriptor
mln_opengl_surface_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Returns WebGPU surface descriptor defaults for this C API version.
 */
MLN_API mln_webgpu_surface_descriptor
mln_webgpu_surface_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Starts attachment of a Metal surface target.
 *
 * The descriptor and options are copied before return. The completion runs
 * only after the selected driver initializes the target. A core-worker driver
 * retains the Metal layer and device on its worker. A caller driver performs
 * initialization when the host services driver work with the Metal context
 * usable on that thread.
 */
MLN_API mln_status mln_metal_surface_attach(
  mln_map map, const mln_metal_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts attachment of a Vulkan surface target. */
MLN_API mln_status mln_vulkan_surface_attach(
  mln_map map, const mln_vulkan_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts attachment of an OpenGL surface target.
 *
 * WGL, EGL, and existing WebGL contexts require
 * MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD. A transferred WebGL canvas supports
 * MLN_RENDER_DRIVER_CORE_WORKER. Context ownership remains independent.
 */
MLN_API mln_status mln_opengl_surface_attach(
  mln_map map, const mln_opengl_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts attachment of a WebGPU surface target.
 *
 * Browser targets require MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD because
 * WebGPU objects remain in their creating agent.
 */
MLN_API mln_status mln_webgpu_surface_attach(
  mln_map map, const mln_webgpu_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts an ordered Metal surface replacement. */
MLN_API mln_status mln_metal_surface_set_target(
  mln_render_session session, const mln_metal_surface_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts an ordered Vulkan surface replacement. */
MLN_API mln_status mln_vulkan_surface_set_target(
  mln_render_session session, const mln_vulkan_surface_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts an ordered OpenGL surface replacement. */
MLN_API mln_status mln_opengl_surface_set_target(
  mln_render_session session, const mln_opengl_surface_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts an ordered WebGPU surface replacement. */
MLN_API mln_status mln_webgpu_surface_set_target(
  mln_render_session session, const mln_webgpu_surface_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_SURFACE_H
