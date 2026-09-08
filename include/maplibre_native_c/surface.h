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
  /** Borrowed VkSurfaceKHR bit pattern. Required. */
  mln_vulkan_non_dispatchable_handle surface;
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
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; out_session is null or does not point to the null handle; or the
 *   requested driver kind is unknown.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Metal backend.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_metal_surface_attach(
  mln_map map, const mln_metal_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts attachment of a Vulkan surface target.
 *
 * The descriptor and options are copied before return. The completion runs only
 * after the selected driver creates the swapchain.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; out_session is null or does not point to the null handle; or the
 *   requested driver kind is unknown.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Vulkan backend.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_vulkan_surface_attach(
  mln_map map, const mln_vulkan_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts attachment of an OpenGL surface target.
 *
 * WGL, EGL, and existing WebGL contexts require
 * MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD. A transferred WebGL canvas requires
 * MLN_RENDER_DRIVER_CORE_WORKER. Context ownership remains independent.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; out_session is null or does not point to the null handle; or the
 *   requested driver kind is unknown.
 * - MLN_STATUS_UNSUPPORTED when this build carries no OpenGL backend, its
 *   context provider is unavailable, or the requested driver does not match the
 *   context placement.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
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
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; out_session is null or does not point to the null handle; or the
 *   requested driver kind is unknown.
 * - MLN_STATUS_UNSUPPORTED when this build carries no WebGPU backend, or the
 *   requested driver is not MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_webgpu_surface_attach(
  mln_map map, const mln_webgpu_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered Metal surface replacement.
 *
 * The descriptor is copied before return, and the session retains the
 * replacement layer and device.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live; descriptor or
 *   completion is null or undersized; or a required backend handle is null.
 * - MLN_STATUS_INVALID_STATE when the session is not attached, or a texture
 *   frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Metal backend, or the
 *   session does not render through a native surface.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders through the new target.
 * - MLN_STATUS_UNSUPPORTED when the backend cannot keep the GPU state it
 *   compiled for the old one.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_metal_surface_set_target(
  mln_render_session session, const mln_metal_surface_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered Vulkan surface replacement.
 *
 * The descriptor is copied before return. The replacement must name the context
 * this session attached with.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live; descriptor or
 *   completion is null or undersized; or a required backend handle is null.
 * - MLN_STATUS_INVALID_STATE when the session is not attached, or a texture
 *   frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Vulkan backend, the
 *   session does not render through a native surface, or the replacement
 * surface does not report the color format and transform this session compiled
 * for.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders through the new target.
 * - MLN_STATUS_UNSUPPORTED when the backend cannot keep the GPU state it
 *   compiled for the old one.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_vulkan_surface_set_target(
  mln_render_session session, const mln_vulkan_surface_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered OpenGL surface replacement.
 *
 * The descriptor is copied before return. The replacement must name the share
 * group this session attached with.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live; descriptor or
 *   completion is null or undersized; or a required backend handle is null.
 * - MLN_STATUS_INVALID_STATE when the session is not attached, or a texture
 *   frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no OpenGL backend, or the
 *   session does not render through a native surface.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders through the new target.
 * - MLN_STATUS_UNSUPPORTED when the backend cannot keep the GPU state it
 *   compiled for the old one.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_opengl_surface_set_target(
  mln_render_session session, const mln_opengl_surface_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered WebGPU surface replacement.
 *
 * The descriptor is copied before return. The replacement must name the device
 * and queue this session attached with.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live; descriptor or
 *   completion is null or undersized; or a required backend handle is null.
 * - MLN_STATUS_INVALID_STATE when the session is not attached, or a texture
 *   frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no WebGPU backend, or the
 *   session does not render through a native surface.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders through the new target.
 * - MLN_STATUS_UNSUPPORTED when the backend cannot keep the GPU state it
 *   compiled for the old one.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_webgpu_surface_set_target(
  mln_render_session session, const mln_webgpu_surface_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_SURFACE_H
