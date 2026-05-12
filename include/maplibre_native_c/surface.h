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

/**
 * EGL native surface session attachment options (Android / Linux).
 *
 * The caller creates and owns the EGL display, config, context, and window
 * surface. The session borrows all handles; they must remain valid until the
 * session is detached or destroyed.
 *
 * Typical Android usage:
 *   - display:  eglGetDisplay(EGL_DEFAULT_DISPLAY)  (initialised by caller)
 *   - config:   chosen with EGL_RENDERABLE_TYPE=EGL_OPENGL_ES2_BIT
 *   - context:  eglCreateContext(display, config, EGL_NO_CONTEXT, …)
 *   - surface:  eglCreateWindowSurface(display, config, ANativeWindow*, …)
 */
typedef struct mln_egl_surface_descriptor {
  uint32_t size;
  /** Logical map width in UI pixels. */
  uint32_t width;
  /** Logical map height in UI pixels. */
  uint32_t height;
  /** UI-to-device pixel scale. Must be positive and finite. */
  double scale_factor;
  /** Borrowed EGLDisplay. Required. */
  void* display;
  /** Borrowed EGLContext. Required. */
  void* context;
  /** Borrowed EGLSurface (window surface). Required. */
  void* surface;
} mln_egl_surface_descriptor;

/**
 * WGL native surface session attachment options (Windows).
 *
 * The caller creates and owns the device context and WGL rendering context.
 * The session borrows both handles and calls wglMakeCurrent to activate the
 * context before each render. SwapBuffers is called by the session after each
 * rendered frame.
 *
 * Typical MAUI / Win32 usage:
 *   - hdc:   GetDC(hwnd) on a CS_OWNDC child window
 *   - hglrc: wglCreateContext(hdc) after choosing a pixel format
 */
typedef struct mln_wgl_surface_descriptor {
  uint32_t size;
  /** Logical map width in UI pixels. */
  uint32_t width;
  /** Logical map height in UI pixels. */
  uint32_t height;
  /** UI-to-device pixel scale. Must be positive and finite. */
  double scale_factor;
  /** Borrowed HDC (device context). Required. */
  void* hdc;
  /** Borrowed HGLRC (WGL rendering context). Required. */
  void* hglrc;
} mln_wgl_surface_descriptor;

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
 * Attaches a Metal native surface render target to a map.
 *
 * The map may have at most one live render session. The session and
 * every surface-session call are owner-thread affine to the map owner thread.
 * The session retains descriptor->layer and optional
 * descriptor->context.device. It renders into the layer and presents through
 * it. On success, *out_session receives a handle the caller destroys with
 * mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_UNSUPPORTED when Metal surface sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_metal_surface_attach(
  mln_map* map, const mln_metal_surface_descriptor* descriptor,
  mln_render_session** out_session
) MLN_NOEXCEPT;

/**
 * Attaches a Vulkan native surface render target to a map.
 *
 * The map may have at most one live render session. The session and
 * every surface-session call are owner-thread affine to the map owner thread.
 * The session renders to descriptor->surface and presents through it. The
 * Vulkan device must support VK_KHR_swapchain, and the queue family must
 * support graphics and presentation to descriptor->surface. Vulkan handles are
 * borrowed and must remain valid until the session is detached or destroyed.
 * On success, *out_session receives a handle the caller destroys with
 * mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_UNSUPPORTED when Vulkan surface sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_vulkan_surface_attach(
  mln_map* map, const mln_vulkan_surface_descriptor* descriptor,
  mln_render_session** out_session
) MLN_NOEXCEPT;

/**
 * Returns EGL surface descriptor defaults for this C API version.
 */
MLN_API mln_egl_surface_descriptor
mln_egl_surface_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Attaches an EGL native surface render target to a map (Android / Linux).
 *
 * The map may have at most one live render session. The session and every
 * surface-session call are owner-thread affine to the map owner thread. All
 * EGL handles in the descriptor are borrowed and must remain valid until the
 * session is detached or destroyed. On success, *out_session receives a handle
 * the caller destroys with mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_UNSUPPORTED when EGL surface sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_egl_surface_attach(
  mln_map* map, const mln_egl_surface_descriptor* descriptor,
  mln_render_session** out_session
) MLN_NOEXCEPT;

/**
 * Returns WGL surface descriptor defaults for this C API version.
 */
MLN_API mln_wgl_surface_descriptor
mln_wgl_surface_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Attaches a WGL native surface render target to a map (Windows).
 *
 * The map may have at most one live render session. The session and every
 * surface-session call are owner-thread affine to the map owner thread. The
 * HDC and HGLRC in the descriptor are borrowed and must remain valid until the
 * session is detached or destroyed. The session calls wglMakeCurrent before
 * each render and SwapBuffers after presenting each frame. On success,
 * *out_session receives a handle the caller destroys with
 * mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_UNSUPPORTED when WGL surface sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_wgl_surface_attach(
  mln_map* map, const mln_wgl_surface_descriptor* descriptor,
  mln_render_session** out_session
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_SURFACE_H
