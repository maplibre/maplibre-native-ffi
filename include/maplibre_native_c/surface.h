/**
 * @file maplibre_native_c/surface.h
 * Public C API declarations for surface render targets.
 */

#ifndef MAPLIBRE_NATIVE_C_SURFACE_H
#define MAPLIBRE_NATIVE_C_SURFACE_H

#include <stdint.h>

#include "base.h"
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
 * Attaches a Metal native surface render target to a map.
 *
 * The map may have at most one live render session. The calling thread becomes
 * the session's owner thread, and every surface-session call is affine to it.
 * The map need only be live, so a host may attach on the thread that drives its
 * render loop while runtime and map operations continue on the runtime's native
 * worker. Attach creates the session's graphics resources on the calling
 * thread, so the host resources named by descriptor must be usable there. The
 * session retains descriptor->layer and optional descriptor->context.device,
 * and renders into and presents through the layer. On success, *out_session
 * receives a handle the caller destroys with mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_UNSUPPORTED when Metal surface sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_metal_surface_attach(
  mln_map map, const mln_metal_surface_descriptor* descriptor,
  mln_render_session* out_session
) MLN_NOEXCEPT;

/**
 * Attaches a Vulkan native surface render target to a map.
 *
 * The map may have at most one live render session. The calling thread becomes
 * the session's owner thread, and every surface-session call is affine to it.
 * The map need only be live, so a host may attach on the thread that drives its
 * render loop while runtime and map operations continue on the runtime's native
 * worker. Attach creates the session's graphics resources on the calling
 * thread, so the host resources named by descriptor must be usable there. The
 * session renders to descriptor->surface and presents through it. The Vulkan
 * device must support VK_KHR_swapchain, and the queue family must support
 * graphics and presentation to descriptor->surface. Vulkan handles are borrowed
 * and must remain valid until the session is detached or destroyed. On success,
 * *out_session receives a handle the caller destroys with
 * mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_UNSUPPORTED when Vulkan surface sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_vulkan_surface_attach(
  mln_map map, const mln_vulkan_surface_descriptor* descriptor,
  mln_render_session* out_session
) MLN_NOEXCEPT;

/**
 * Attaches an OpenGL native surface render target to a map.
 *
 * The map may have at most one live render session. The calling thread becomes
 * the session's owner thread, and every surface-session call is affine to it.
 * The map need only be live, so a host may attach on the thread that drives its
 * render loop while runtime and map operations continue on the runtime's native
 * worker. Attach creates the session's graphics resources on the calling
 * thread, so the host resources named by descriptor must be usable there.
 *
 * descriptor->context.ownership decides what the session does with the thread's
 * current context. A shared session builds its context in the share group of
 * the descriptor's share context, which must be current on this thread, and
 * every render restores whatever was current before it. A dedicated session
 * builds its context from the display or device context alone, makes it
 * current, and keeps it current between renders, so this session holds the
 * thread's context from attach until detach or destroy, either of which
 * releases it.
 *
 * The session renders to descriptor->surface and presents through the selected
 * context provider. WGL surfaces present with SwapBuffers(HDC), and EGL
 * surfaces present with eglSwapBuffers(EGLDisplay, EGLSurface). OpenGL context
 * handles are borrowed and must remain valid until detach or destroy. On
 * success, *out_session receives a handle the caller destroys with
 * mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_UNSUPPORTED when OpenGL surface sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_opengl_surface_attach(
  mln_map map, const mln_opengl_surface_descriptor* descriptor,
  mln_render_session* out_session
) MLN_NOEXCEPT;

/**
 * Attaches a WebGPU native surface render target to a map.
 *
 * The map may have at most one live render session. The calling thread becomes
 * the session's owner thread, and every surface-session call is affine to it.
 * The map need only be live, so a host may attach on the thread that drives its
 * render loop while runtime and map operations continue on the runtime's native
 * worker. Attach creates the session's graphics resources on the calling
 * thread, so the host resources named by descriptor must be usable there; a
 * WebGPU object belongs to the agent that created it. The session configures
 * descriptor->surface for this device and extent, takes its current texture
 * each frame, and presents through it. The surface, device, and instance are
 * borrowed and must remain valid until detach or destroy. On success,
 * *out_session receives a handle the caller destroys with
 * mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_UNSUPPORTED when WebGPU surface sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_webgpu_surface_attach(
  mln_map map, const mln_webgpu_surface_descriptor* descriptor,
  mln_render_session* out_session
) MLN_NOEXCEPT;

/**
 * Presents an attached Metal surface session through a new surface.
 *
 * Use this when a host destroys and recreates its surface while the map lives
 * on. The presentation surface is replaced in place, so the session keeps its
 * renderer along with the tile pyramid, glyph and image atlases, symbol
 * placement, and feature state set through
 * mln_render_session_set_feature_state().
 *
 * descriptor->context must name the graphics context or device the session
 * attached with; a null Metal device names none and is accepted. A target on a
 * different context, or a lost graphics context, requires destroying the
 * session with mln_render_session_destroy() and attaching again.
 *
 * The new extent applies exactly as mln_render_session_resize() applies one,
 * including how the next mln_render_session_render_update() waits for the map
 * to catch up to it. A scale_factor that differs from the session's current
 * value rebuilds the renderer, whose shaders are compiled for a fixed pixel
 * ratio; the surface is replaced either way.
 *
 * Every failure status but MLN_STATUS_NATIVE_ERROR is reported before the
 * target is touched and leaves the session rendering into the one it had.
 * MLN_STATUS_NATIVE_ERROR may mean a replacement was already under way, which
 * cannot be unwound; destroy the session with mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, descriptor is
 *   null or invalid, or descriptor->context names a device other than the
 *   session's.
 * - MLN_STATUS_UNSUPPORTED when descriptor->format differs from the session's;
 *   destroy the session and attach again to change it.
 * - MLN_STATUS_INVALID_STATE when the session is detached.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when the session does not render through a Metal
 *   surface, or when Metal surface sessions are not supported by this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_metal_surface_set_target(
  mln_render_session session, const mln_metal_surface_descriptor* descriptor
) MLN_NOEXCEPT;

/**
 * Presents an attached Vulkan surface session through a new surface.
 *
 * See mln_metal_surface_set_target() for what replacing a surface preserves,
 * when a host reaches for it, and how failures are reported.
 * descriptor->context must name the same instance, physical device, device, and
 * graphics queue the session attached with, and the new descriptor->surface
 * must be presentable from that queue family.
 *
 * The outgoing VkSurfaceKHR must still be valid when this is called, because
 * Vulkan requires the session's swapchain to be destroyed before its surface. A
 * host that has to release its surface first destroys the session with
 * mln_render_session_destroy() and attaches again afterward. Metal and OpenGL
 * carry no such requirement.
 *
 * The replacement must report the color format and the surface-transform
 * support this session already compiled a render pass and shaders for.
 * MLN_STATUS_UNSUPPORTED reports one that does not, with the session still
 * rendering into the surface it has; destroy the session and attach again to
 * change either.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, descriptor is
 *   null or invalid, descriptor->context names handles other than the
 *   session's, or the surface is not usable by this session.
 * - MLN_STATUS_INVALID_STATE when the session is detached.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when the session does not render through a Vulkan
 *   surface, when the replacement's color format or surface-transform support
 *   differs from the session's, or when Vulkan surface sessions are not
 *   supported by this build.
 * - MLN_STATUS_NATIVE_ERROR when a Vulkan query about the replacement fails, or
 *   when an internal exception is converted to status.
 */
MLN_API mln_status mln_vulkan_surface_set_target(
  mln_render_session session, const mln_vulkan_surface_descriptor* descriptor
) MLN_NOEXCEPT;

/**
 * Presents an attached OpenGL surface session through a new surface.
 *
 * See mln_metal_surface_set_target() for what replacing a surface preserves,
 * when a host reaches for it, and how failures are reported.
 * descriptor->context must name the context provider data the session attached
 * with. The new surface is made current on the next render, so a host may
 * replace a surface it has already destroyed. Under dedicated ownership the
 * session's context stays current on the previous surface until that render.
 *
 * Because nothing is made current here, a surface this call accepts can still
 * turn out to be unusable. An HDC whose pixel format does not match the
 * session's context, or an EGLSurface from another display, is reported by the
 * next mln_render_session_render_update() as MLN_STATUS_NATIVE_ERROR rather
 * than by this function. The session stays destroyable in that state.
 *
 * A lost OpenGL context requires destroying the session and attaching again.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, descriptor is
 *   null or invalid, or descriptor->context names context provider data other
 *   than the session's.
 * - MLN_STATUS_INVALID_STATE when the session is detached.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when the session does not render through an OpenGL
 *   surface, or when OpenGL surface sessions are not supported by this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_opengl_surface_set_target(
  mln_render_session session, const mln_opengl_surface_descriptor* descriptor
) MLN_NOEXCEPT;

/**
 * Presents an attached WebGPU surface session through a new surface.
 *
 * The replacement must name the device and format the session attached with.
 * The session unconfigures the surface it had and configures the replacement
 * for this extent.
 *
 * Every failure status but MLN_STATUS_NATIVE_ERROR is reported before the
 * target is touched and leaves the session rendering into the one it had.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, descriptor is
 *   null or invalid, or descriptor->context names a device other than the
 *   session's.
 * - MLN_STATUS_INVALID_STATE when the session is detached.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when the session does not render through a WebGPU
 *   surface, or when WebGPU surface sessions are not supported by this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_webgpu_surface_set_target(
  mln_render_session session, const mln_webgpu_surface_descriptor* descriptor
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_SURFACE_H
