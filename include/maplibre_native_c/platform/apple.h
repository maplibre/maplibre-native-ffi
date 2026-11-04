/**
 * @file maplibre_native_c/platform/apple.h
 * Render into IOSurfaces, and pace the library's frames to the display, on
 * Apple platforms.
 */

#ifndef MAPLIBRE_NATIVE_C_PLATFORM_APPLE_H
#define MAPLIBRE_NATIVE_C_PLATFORM_APPLE_H

#include <stdint.h>

#include "../base.h"
#include "../completion.h"
#include "../map.h"
#include "../render_session.h"
#include "../render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Creates a frame pacer that follows the display through a `CADisplayLink`.
 *
 * You can call it on any thread. The pacer needs no run loop of yours. On
 * macOS, it follows the main display, the one with the menu bar.
 *
 * With mln_frame_pacer_options.maximum_frames_per_second, the pacer also asks
 * the system for that refresh rate. On iPhone, a rate above 60 needs
 * `CADisableMinimumFrameDurationOnPhone` in the app's `Info.plist`.
 *
 * Returns:
 * - MLN_STATUS_NATIVE_ERROR when the system has no display to follow.
 */
MLN_BINDING("platforms=ios,macos,tvos")
MLN_API mln_status mln_apple_frame_pacer_create(
  const mln_frame_pacer_options* options MLN_BINDING("nullable=true"),
  mln_frame_pacer* out_pacer MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * A ring of IOSurfaces that the library allocates, for
 * mln_map_attach_io_surface_ring().
 *
 * Each surface has the map's size in pixels, `logical_size` times the map's
 * scale factor, and the pixel format kCVPixelFormatType_32BGRA, which Metal
 * names MTLPixelFormatBGRA8Unorm.
 */
typedef struct mln_io_surface_ring_descriptor {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** How many surfaces the ring has, from 1 to 8. */
  uint32_t buffer_count MLN_BINDING("required=true");
  /** The MTLDevice to render with. A null device picks the system's default. */
  mln_metal_context_descriptor context;
} mln_io_surface_ring_descriptor;

/** The IOSurface of an acquired frame. */
typedef struct mln_io_surface_frame {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  double scale_factor;
  /** The surface's index in the ring. */
  uint32_t slot;
  /** The `IOSurfaceRef`. */
  void* surface MLN_BINDING("lifetime=owner");
} mln_io_surface_frame;

/**
 * Creates a session that renders the map into a ring of IOSurfaces that the
 * library allocates.
 *
 * Acquire each frame with mln_render_session_acquire_frame(), and get its
 * surface with mln_acquired_frame_get_io_surface().
 * mln_render_session_resize() gives the ring new surfaces of the new size.
 * Attach with MLN_RENDER_DRIVER_CORE_WORKER.
 *
 * To hand frames to a reader that can't wait on an event, such as Flutter's
 * `copyPixelBuffer`, attach without MLN_GPU_SYNC_KIND_FLAG_METAL_SHARED_EVENT
 * in mln_render_session_attach_options.producer_sync_kinds, so that each frame
 * finishes before you acquire it. Release a frame with
 * MLN_GPU_SYNC_CPU_COMPLETE only once the reader has finished with its surface.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when mln_supported_render_backend_mask() lacks
 *   MLN_RENDER_BACKEND_FLAG_METAL.
 * - MLN_STATUS_NATIVE_ERROR when `context` names no device and the system has
 *   no Metal device.
 */
MLN_BINDING("execution=lifecycle;platforms=ios,macos,tvos,visionos")
MLN_API mln_status mln_map_attach_io_surface_ring(
  mln_map map, const mln_io_surface_ring_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gets the IOSurface of an acquired frame.
 *
 * Wait for mln_acquired_frame_get_producer_sync() before you read the surface.
 * Once you release or discard the frame, the session may render into the
 * surface again. For an API that takes a `CVPixelBuffer`, wrap the surface
 * with `CVPixelBufferCreateWithIOSurface`. A reference that you take with
 * `CFRetain`, or a pixel buffer that wraps the surface, keeps the surface's
 * memory, but not its contents.
 *
 * Returns:
 * - MLN_STATUS_TARGET_LOST when the session failed, or you detached, abandoned,
 *   or disposed it.
 *
 * Lifetimes:
 * The surface in `out_frame` stays valid until you release or discard the
 * frame, or the session is abandoned or disposed. A view from
 * mln_acquired_frame_view_begin() keeps it valid until
 * mln_acquired_frame_view_end().
 */
MLN_BINDING("view_owner=frame;platforms=ios,macos,tvos,visionos")
MLN_API mln_status mln_acquired_frame_get_io_surface(
  mln_acquired_frame frame,
  mln_io_surface_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PLATFORM_APPLE_H
