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
  /** Metal backend context. */
  mln_metal_context_descriptor context;
  /** CAMetalLayer* / CA::MetalLayer* retained by the session. Required. */
  void* layer;
} mln_metal_surface_descriptor;

/** Vulkan native surface session attachment options. */
typedef struct mln_vulkan_surface_descriptor {
  uint32_t size;
  /** Logical surface extent. */
  mln_render_target_extent extent;
  /** Borrowed Vulkan context. All handles are required. */
  mln_vulkan_context_descriptor context;
  /** Borrowed VkSurfaceKHR. Required. */
  void* surface;
} mln_vulkan_surface_descriptor;

/** EGL native surface session attachment options (Android / Linux). */
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

/** WGL native surface session attachment options (Windows). */
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
 * The map may have at most one live render session. The session retains
 * descriptor->layer and optional descriptor->context.device.
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
 * The map may have at most one live render session. Vulkan handles are
 * borrowed and must remain valid until the session is detached or destroyed.
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
 * The map may have at most one live render session. All EGL handles are
 * borrowed and must remain valid until the session is detached or destroyed.
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
 * The map may have at most one live render session. The HDC and HGLRC are
 * borrowed and must remain valid until the session is detached or destroyed.
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
