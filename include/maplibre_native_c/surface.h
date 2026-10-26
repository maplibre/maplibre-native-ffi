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

/** Metal attachment options for a native surface. */
typedef struct mln_metal_surface_descriptor {
  uint32_t size;
  /**
   * Logical surface extent. Required. A scale_factor that differs from the
   * map's is accepted and logged as a warning.
   */
  mln_logical_extent extent MLN_BINDING("required=true");
  /** Metal backend context. device is optional for Metal surfaces. */
  mln_metal_context_descriptor context;
  /** `CAMetalLayer*` / `CA::MetalLayer*` retained by the session. Required. */
  void* layer MLN_BINDING("required=true");
} mln_metal_surface_descriptor;

/** Vulkan attachment options for a native surface. */
typedef struct mln_vulkan_surface_descriptor {
  uint32_t size;
  /**
   * Logical surface extent. Required. A scale_factor that differs from the
   * map's is accepted and logged as a warning.
   */
  mln_logical_extent extent MLN_BINDING("required=true");
  /**
   * Borrowed Vulkan context. All handles are required. The device must support
   * VK_KHR_swapchain, and the queue family must support graphics and
   * presentation to this descriptor's surface.
   */
  mln_vulkan_context_descriptor context;
  /** Borrowed VkSurfaceKHR bit pattern. Required. */
  mln_vulkan_non_dispatchable_handle surface MLN_BINDING("required=true");
} mln_vulkan_surface_descriptor;

/** WebGPU attachment options for a native surface. */
typedef struct mln_web_gpu_surface_descriptor {
  uint32_t size;
  /**
   * Logical surface extent. Required. A scale_factor that differs from the
   * map's is accepted and logged as a warning.
   */
  mln_logical_extent extent MLN_BINDING("required=true");
  /** Borrowed WebGPU context. device is required. */
  mln_web_gpu_context_descriptor context;
  /**
   * Borrowed WGPUSurface. Required, and must stay alive for the session. The
   * session configures it for this device and extent, and unconfigures it when
   * the session ends.
   */
  void* surface MLN_BINDING("required=true");
  /**
   * WGPUTextureFormat to configure the surface with. Required. A browser host
   * takes it from navigator.gpu.getPreferredCanvasFormat().
   */
  uint32_t format MLN_BINDING("required=true");
} mln_web_gpu_surface_descriptor;

/** OpenGL attachment options for a native surface. */
typedef struct mln_open_gl_surface_descriptor {
  uint32_t size;
  /**
   * Logical surface extent. Required. A scale_factor that differs from the
   * map's is accepted and logged as a warning.
   */
  mln_logical_extent extent MLN_BINDING("required=true");
  /** Borrowed OpenGL context provider data. */
  mln_open_gl_context_descriptor context;
  /**
   * Borrowed platform surface handle: an HDC for WGL and an EGLSurface for EGL,
   * both required. Null for WebGL, whose context carries its canvas binding.
   */
  void* surface;
} mln_open_gl_surface_descriptor;

/**
 * Starts attachment of a Metal surface target.
 *
 * The descriptor and options are copied before return. An accepted attachment
 * retains the Metal layer and device before return. The completion reports
 * target initialization. A caller driver requires service on a thread where
 * the Metal context is usable.
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; descriptor->extent has a zero width or height, a scale_factor that
 *   is not finite and positive, or a scaled dimension past UINT32_MAX;
 *   out_session is null or does not point to the null handle; or the
 *   requested driver kind is unknown, or options carry a malformed wake or
 *   queue lock.
 * - MLN_STATUS_INVALID_STATE when map has been released, or its creation has
 *   not succeeded.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Metal backend, or options
 *   enable a queue lock.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle;platforms=ios,macos,tvos")
MLN_API mln_status mln_map_attach_metal_surface(
  mln_map map, const mln_metal_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts attachment of a Vulkan surface target.
 *
 * The descriptor and options are copied before return. The completion runs only
 * after the selected driver creates the swapchain.
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; descriptor->extent has a zero width or height, a scale_factor that
 *   is not finite and positive, or a scaled dimension past UINT32_MAX;
 *   out_session is null or does not point to the null handle; or the
 *   requested driver kind is unknown, or options carry a malformed wake or
 *   queue lock.
 * - MLN_STATUS_INVALID_STATE when map has been released, or its creation has
 *   not succeeded.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Vulkan backend.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_INVALID_ARGUMENT when the driver finds the context
 *   inconsistent, such as a physical device of another instance or a
 *   graphics_queue_family_index that names no graphics queue family.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle;platforms=android,linux,macos,ohos,windows")
MLN_API mln_status mln_map_attach_vulkan_surface(
  mln_map map, const mln_vulkan_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts attachment of an OpenGL surface target.
 *
 * WGL, EGL, and existing WebGL contexts require
 * MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD. A transferred WebGL canvas requires
 * MLN_RENDER_DRIVER_CORE_WORKER. Context ownership remains independent.
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; descriptor->extent has a zero width or height, a scale_factor that
 *   is not finite and positive, or a scaled dimension past UINT32_MAX;
 *   out_session is null or does not point to the null handle; or the
 *   requested driver kind is unknown, or options carry a malformed wake or
 *   queue lock.
 * - MLN_STATUS_INVALID_STATE when map has been released, or its creation has
 *   not succeeded.
 * - MLN_STATUS_UNSUPPORTED when this build carries no OpenGL backend, its
 *   context provider is unavailable; the requested driver does not match the
 *   context placement; or options enable a queue lock.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_attach_open_gl_surface(
  mln_map map, const mln_open_gl_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts attachment of a WebGPU surface target.
 *
 * Browser targets require MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD in the
 * agent that created the WebGPU objects.
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; descriptor->extent has a zero width or height, a scale_factor that
 *   is not finite and positive, or a scaled dimension past UINT32_MAX;
 *   out_session is null or does not point to the null handle; or the
 *   requested driver kind is unknown, or options carry a malformed wake or
 *   queue lock.
 * - MLN_STATUS_INVALID_STATE when map has been released, or its creation has
 *   not succeeded.
 * - MLN_STATUS_UNSUPPORTED when this build carries no WebGPU backend, or the
 *   requested driver is not MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, or
 *   options enable a queue lock.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle;platforms=web")
MLN_API mln_status mln_map_attach_web_gpu_surface(
  mln_map map, const mln_web_gpu_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered Metal surface replacement.
 *
 * The descriptor is copied before return, and the session retains the
 * replacement layer and device.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle; descriptor
 *   or completion is null or undersized; a required backend handle is null; or
 *   descriptor->extent has a zero width or height, a scale_factor that is not
 *   finite and positive, or a scaled dimension past UINT32_MAX.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached,
 *   or a texture frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Metal backend, or the
 *   session does not render through a native surface.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders through the new target.
 * - MLN_STATUS_INVALID_ARGUMENT when the replacement names another device.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=operation;platforms=ios,macos,tvos")
MLN_API mln_status mln_render_session_set_metal_surface_target(
  mln_render_session session, const mln_metal_surface_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered Vulkan surface replacement.
 *
 * The descriptor is copied before return. The replacement must name the context
 * this session attached with.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle; descriptor
 *   or completion is null or undersized; a required backend handle is null; or
 *   descriptor->extent has a zero width or height, a scale_factor that is not
 *   finite and positive, or a scaled dimension past UINT32_MAX.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached,
 *   or a texture frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Vulkan backend, or the
 *   session does not render through a native surface.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders through the new target.
 * - MLN_STATUS_INVALID_ARGUMENT when the replacement names another context, or
 *   its surface cannot present from the context's graphics queue.
 * - MLN_STATUS_UNSUPPORTED when the replacement surface's color format or
 *   transform differs from those the session attached with.
 * - MLN_STATUS_NATIVE_ERROR when querying the replacement surface fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=operation;platforms=android,linux,macos,ohos,windows")
MLN_API mln_status mln_render_session_set_vulkan_surface_target(
  mln_render_session session, const mln_vulkan_surface_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered OpenGL surface replacement.
 *
 * The descriptor is copied before return. The replacement must name the share
 * group this session attached with.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle; descriptor
 *   or completion is null or undersized; a required backend handle is null; or
 *   descriptor->extent has a zero width or height, a scale_factor that is not
 *   finite and positive, or a scaled dimension past UINT32_MAX.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached,
 *   or a texture frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no OpenGL backend, or the
 *   session does not render through a native surface.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders through the new target.
 * - MLN_STATUS_INVALID_ARGUMENT when the replacement names another share group.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_set_open_gl_surface_target(
  mln_render_session session, const mln_open_gl_surface_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered WebGPU surface replacement.
 *
 * The descriptor is copied before return. The replacement must name the device
 * and queue this session attached with.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle; descriptor
 *   or completion is null or undersized; a required backend handle is null; or
 *   descriptor->extent has a zero width or height, a scale_factor that is not
 *   finite and positive, or a scaled dimension past UINT32_MAX.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached,
 *   or a texture frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no WebGPU backend, or the
 *   session does not render through a native surface.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders through the new target.
 * - MLN_STATUS_INVALID_ARGUMENT when the replacement names another device or
 *   queue.
 * - MLN_STATUS_UNSUPPORTED when the replacement's format differs from the one
 *   the session attached with.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=operation;platforms=web")
MLN_API mln_status mln_render_session_set_web_gpu_surface_target(
  mln_render_session session, const mln_web_gpu_surface_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_SURFACE_H
