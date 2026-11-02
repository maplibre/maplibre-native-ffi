/**
 * @file maplibre_native_c/surface.h
 * Render a map to a window's surface.
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

/**
 * A Metal layer to render to, for mln_map_attach_metal_surface() and
 * mln_render_session_set_metal_surface_target().
 *
 * The session sets the layer's device, pixel format, and drawable size. It uses
 * the layer until a replacement or release of the target completes, a detach
 * or disposal of the session completes, or an abandon of it returns.
 */
typedef struct mln_metal_surface_descriptor {
  uint32_t size;
  mln_logical_extent extent MLN_BINDING("required=true");
  /** The MTLDevice to render with. A null device picks the system's default. */
  mln_metal_context_descriptor context;
  /** Your CAMetalLayer. */
  void* layer MLN_BINDING("required=true");
} mln_metal_surface_descriptor;

/**
 * A Vulkan surface to present to, for mln_map_attach_vulkan_surface() and
 * mln_render_session_set_vulkan_surface_target().
 *
 * The device must have VK_KHR_swapchain enabled, and the queue family of
 * `graphics_queue_family_index` must support presenting to the surface. The
 * session creates the swapchain. It uses the surface until a replacement or
 * release of the target completes, a detach or disposal of the session
 * completes, or an abandon of it returns.
 */
typedef struct mln_vulkan_surface_descriptor {
  uint32_t size;
  mln_logical_extent extent MLN_BINDING("required=true");
  mln_vulkan_context_descriptor context;
  /** Your VkSurfaceKHR. */
  mln_vulkan_non_dispatchable_handle surface MLN_BINDING("required=true");
} mln_vulkan_surface_descriptor;

/**
 * A WebGPU surface to present to, for mln_map_attach_web_gpu_surface() and
 * mln_render_session_set_web_gpu_surface_target().
 *
 * The session configures the surface. It uses the surface until a replacement
 * or release of the target completes, a detach or disposal of the session
 * completes, or an abandon of it returns.
 */
typedef struct mln_web_gpu_surface_descriptor {
  uint32_t size;
  mln_logical_extent extent MLN_BINDING("required=true");
  mln_web_gpu_context_descriptor context;
  /** Your WGPUSurface. */
  void* surface MLN_BINDING("required=true");
  /** The WGPUTextureFormat to configure the surface with. */
  uint32_t format MLN_BINDING("required=true");
} mln_web_gpu_surface_descriptor;

/**
 * An OpenGL surface to present to, for mln_map_attach_open_gl_surface() and
 * mln_render_session_set_open_gl_surface_target().
 *
 * The session uses the surface until a replacement or release of the target
 * completes, a detach or disposal of the session completes, or an abandon of it
 * returns.
 */
typedef struct mln_open_gl_surface_descriptor {
  uint32_t size;
  mln_logical_extent extent MLN_BINDING("required=true");
  mln_open_gl_context_descriptor context;
  /** Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it. */
  void* surface;
} mln_open_gl_surface_descriptor;

/**
 * Creates a session that renders the map to a Metal layer.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when mln_supported_render_backend_mask() lacks
 *   MLN_RENDER_BACKEND_FLAG_METAL.
 */
MLN_BINDING("execution=lifecycle;platforms=ios,macos,tvos,visionos")
MLN_API mln_status mln_map_attach_metal_surface(
  mln_map map, const mln_metal_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Creates a session that renders the map to a Vulkan surface.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when mln_supported_render_backend_mask() lacks
 *   MLN_RENDER_BACKEND_FLAG_VULKAN.
 */
MLN_BINDING(
  "execution=lifecycle;platforms=android,freebsd,linux,macos,ohos,windows"
)
MLN_API mln_status mln_map_attach_vulkan_surface(
  mln_map map, const mln_vulkan_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Creates a session that renders the map to an OpenGL surface.
 *
 * The session needs MLN_RENDER_DRIVER_CORE_WORKER for a transferred WebGL
 * canvas, and MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD for any other context.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when mln_open_gl_supported_context_provider_mask()
 *   lacks the context's platform.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_attach_open_gl_surface(
  mln_map map, const mln_open_gl_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Creates a session that renders the map to a WebGPU surface.
 *
 * The session needs MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when mln_supported_render_backend_mask() lacks
 *   MLN_RENDER_BACKEND_FLAG_WEB_GPU.
 */
MLN_BINDING("execution=lifecycle;platforms=web")
MLN_API mln_status mln_map_attach_web_gpu_surface(
  mln_map map, const mln_web_gpu_surface_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the Metal layer that the session renders to.
 *
 * Use it to give the session a layer again after
 * mln_render_session_release_target(), or to move it to another layer. The
 * new layer must use the same device. Once the replacement completes, the
 * session no longer uses the old layer.
 */
MLN_BINDING("execution=operation;platforms=ios,macos,tvos,visionos")
MLN_API mln_status mln_render_session_set_metal_surface_target(
  mln_render_session session, const mln_metal_surface_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the Vulkan surface that the session presents to.
 *
 * Use it to give the session a surface again after
 * mln_render_session_release_target(), or to move it to another surface. The
 * new surface must use the same context. Once the replacement completes, the
 * session no longer uses the old surface.
 *
 * Completes with:
 * - MLN_STATUS_UNSUPPORTED when the new surface needs a different color format
 *   or pre-rotation support than the old one. Attach a new session instead.
 */
MLN_BINDING(
  "execution=operation;platforms=android,freebsd,linux,macos,ohos,windows"
)
MLN_API mln_status mln_render_session_set_vulkan_surface_target(
  mln_render_session session, const mln_vulkan_surface_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the OpenGL surface that the session presents to.
 *
 * Use it to give the session a surface again after
 * mln_render_session_release_target(), or to move it to another surface. The
 * new surface must use the same context. Once the replacement completes, the
 * session no longer uses the old surface.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_set_open_gl_surface_target(
  mln_render_session session, const mln_open_gl_surface_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the WebGPU surface that the session presents to.
 *
 * Use it to give the session a surface again after
 * mln_render_session_release_target(), or to move it to another surface. The
 * new surface must use the same context. Once the replacement completes, the
 * session no longer uses the old surface.
 *
 * Completes with:
 * - MLN_STATUS_UNSUPPORTED when the new `format` differs from the old one.
 *   Attach a new session to change it.
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
