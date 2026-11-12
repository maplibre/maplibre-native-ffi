/**
 * @file maplibre_native_c/platform/linux.h
 * Render into dmabufs on Linux and FreeBSD.
 */

#ifndef MAPLIBRE_NATIVE_C_PLATFORM_LINUX_H
#define MAPLIBRE_NATIVE_C_PLATFORM_LINUX_H

#include <stddef.h>
#include <stdint.h>

#include "../base.h"
#include "../completion.h"
#include "../map.h"
#include "../render_session.h"
#include "../render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/** A DRM format and modifier that you can import. */
typedef struct mln_dma_buf_format {
  /** A DRM fourcc code, such as DRM_FORMAT_ABGR8888. */
  uint32_t fourcc;
  /** A DRM format modifier, such as DRM_FORMAT_MOD_LINEAR. */
  uint64_t modifier;
} mln_dma_buf_format;

/**
 * A ring of dmabufs that the library allocates, for
 * mln_map_attach_dma_buf_ring().
 *
 * Each buffer has the map's size in pixels and a format and modifier from
 * `formats`. The library renders only DRM_FORMAT_ABGR8888 and
 * DRM_FORMAT_ARGB8888. A modifier can give a buffer more planes than its
 * format has, such as a plane of compression metadata, and the library takes
 * any such modifier that its GPU can render into and share.
 */
typedef struct mln_dma_buf_ring_descriptor {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  /** How many buffers the ring has, from 1 to 8. */
  uint32_t buffer_count MLN_BINDING("required=true");
  /**
   * The formats and modifiers that your reader imports, most preferred first,
   * such as those that eglQueryDmaBufModifiersEXT() reports.
   */
  const mln_dma_buf_format* formats
    MLN_BINDING("length=format_count;required=true");
  size_t format_count;
} mln_dma_buf_ring_descriptor;

/** Where one plane of a dmabuf keeps its data. */
typedef struct mln_dma_buf_plane {
  /**
   * The file descriptor of the dmabuf that holds the plane. The planes of a
   * buffer often share one.
   */
  int32_t fd;
  /** The bytes from the start of the dmabuf to the plane's first byte. */
  uint32_t offset;
  /** The bytes from the start of one row to the start of the next. */
  uint32_t stride;
} mln_dma_buf_plane;

/** The dmabuf of an acquired frame. */
typedef struct mln_dma_buf_frame {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  double scale_factor;
  /** The buffer's index in the ring. */
  uint32_t slot;
  uint32_t fourcc;
  uint64_t modifier;
  /**
   * The buffer's planes, in the order that its fourcc and modifier define.
   * Import all of them, as EGL_EXT_image_dma_buf_import_modifiers planes 0 and
   * up, or with one `zwp_linux_buffer_params_v1.add` call each.
   */
  const mln_dma_buf_plane* planes
    MLN_BINDING("length=plane_count;lifetime=owner");
  size_t plane_count;
} mln_dma_buf_frame;

/**
 * Creates a session that renders the map into a ring of dmabufs that the
 * library allocates.
 *
 * Acquire each frame with mln_render_session_acquire_frame(), and get its
 * buffer with mln_acquired_frame_get_dma_buf(). mln_render_session_resize()
 * gives the ring new buffers of the new size.
 *
 * The session renders with `device`, a Vulkan or OpenGL device, or, when
 * `device` is null, with one that the library creates. An OpenGL device
 * renders with an EGL context of its own. Attach with
 * MLN_RENDER_DRIVER_CORE_WORKER.
 *
 * For a reader that waits on no sync file, such as a Wayland compositor
 * without explicit synchronization, attach without
 * MLN_GPU_SYNC_KIND_FLAG_SYNC_FD in
 * mln_render_session_attach_options.producer_sync_kinds, so that each frame
 * finishes before you acquire it. Keep each frame acquired until its reader is
 * done with the buffer. Give the ring 3 buffers, so the session can render
 * while you hold two frames.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when the device can't render into a dmabuf of any
 *   of `formats`.
 * - MLN_STATUS_NATIVE_ERROR when the library can't create its Vulkan device or
 *   initialize an EGL display.
 */
MLN_BINDING("execution=lifecycle;platforms=freebsd,linux")
MLN_API mln_status mln_map_attach_dma_buf_ring(
  mln_map map, const mln_dma_buf_ring_descriptor* descriptor,
  mln_gpu_device device MLN_BINDING("nullable=true"),
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gets the dmabuf of an acquired frame.
 *
 * Wait for mln_acquired_frame_get_producer_sync() before you read the buffer.
 * Once you release or discard the frame, the session may render into the
 * buffer again. You can map a buffer with DRM_FORMAT_MOD_LINEAR, which has one
 * plane, with `mmap`, and bracket your reads with DMA_BUF_IOCTL_SYNC.
 *
 * The session owns the planes' file descriptors, so don't close them. To keep
 * the buffer after you release the frame, duplicate each file descriptor with
 * `dup`. A duplicate keeps the buffer's memory, but not its contents.
 *
 * Returns:
 * - MLN_STATUS_TARGET_LOST when the session failed, or you detached, abandoned,
 *   or disposed it.
 *
 * Lifetimes:
 * The planes in `out_frame`, and their file descriptors, stay valid until you
 * release or discard the frame, or the session is abandoned or disposed. A
 * view from mln_acquired_frame_view_begin() keeps them valid until
 * mln_acquired_frame_view_end().
 */
MLN_BINDING("view_owner=frame;platforms=freebsd,linux")
MLN_API mln_status mln_acquired_frame_get_dma_buf(
  mln_acquired_frame frame,
  mln_dma_buf_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_PLATFORM_LINUX_H
