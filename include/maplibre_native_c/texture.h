/**
 * @file maplibre_native_c/texture.h
 * The textures of acquired frames, and reading a session's frames back.
 */

#ifndef MAPLIBRE_NATIVE_C_TEXTURE_H
#define MAPLIBRE_NATIVE_C_TEXTURE_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "completion.h"
#include "render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The Metal texture of an acquired frame. */
typedef struct mln_metal_texture_frame {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  double scale_factor;
  /** The texture's index in the descriptor's `textures`. */
  uint32_t slot;
  /** The MTLTexture. */
  void* texture MLN_BINDING("lifetime=owner");
  /** The texture's MTLDevice. */
  void* device MLN_BINDING("lifetime=owner");
  /** The texture's MTLPixelFormat. */
  uint64_t pixel_format;
} mln_metal_texture_frame;

/** The Vulkan image of an acquired frame. */
typedef struct mln_vulkan_texture_frame {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  double scale_factor;
  /** The image's index in the descriptor's `textures`. */
  uint32_t slot;
  /** The VkImage. */
  mln_vulkan_non_dispatchable_handle image MLN_BINDING("lifetime=owner");
  /** The VkImageView. */
  mln_vulkan_non_dispatchable_handle image_view MLN_BINDING("lifetime=owner");
  /** The image's VkDevice. */
  void* device MLN_BINDING("lifetime=owner");
  /** The image's VkFormat. */
  uint32_t format;
  /** The VkImageLayout that the frame left the image in. */
  uint32_t layout;
} mln_vulkan_texture_frame;

/** The WebGPU texture of an acquired frame. */
typedef struct mln_web_gpu_texture_frame {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  double scale_factor;
  /** The texture's index in the descriptor's `textures`. */
  uint32_t slot;
  /** The WGPUTexture. */
  void* texture MLN_BINDING("lifetime=owner");
  /** The WGPUTextureView. */
  void* texture_view MLN_BINDING("lifetime=owner");
  /** The texture's WGPUDevice. */
  void* device MLN_BINDING("lifetime=owner");
  /** The texture's WGPUTextureFormat. */
  uint32_t format;
} mln_web_gpu_texture_frame;

/** The OpenGL texture of an acquired frame. */
typedef struct mln_open_gl_texture_frame {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  double scale_factor;
  /** The texture's index in the descriptor's `textures`. */
  uint32_t slot;
  uint32_t texture MLN_BINDING("lifetime=owner");
  uint32_t target;
} mln_open_gl_texture_frame;

/** The layout of the pixels that mln_render_session_read_texture() reads. */
typedef struct mln_texture_image_info {
  uint32_t width;
  uint32_t height;
  /** The bytes from the start of one row to the start of the next. */
  uint32_t stride;
  size_t byte_length;
} mln_texture_image_info;

/** The pixels that mln_render_session_read_texture() reads. */
typedef struct mln_texture_readback_result {
  /**
   * Premultiplied RGBA pixels with 8 bits per channel, in rows from top to
   * bottom.
   */
  mln_buffer_view data MLN_BINDING("encoding=bytes");
  mln_texture_image_info info;
} mln_texture_readback_result;

/**
 * Reads the pixels of the last frame that the session rendered.
 *
 * Completes with:
 * - MLN_STATUS_UNSUPPORTED when the session's capabilities lack
 *   MLN_RENDER_SESSION_CAPABILITY_READBACK.
 * - MLN_STATUS_INVALID_STATE when the session hasn't rendered a frame since it
 *   attached, resized, or changed its target.
 */
MLN_BINDING("execution=query;result=mln_texture_readback_result")
MLN_API mln_status mln_render_session_read_texture(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gets the Metal texture of an acquired frame.
 *
 * Returns:
 * - MLN_STATUS_TARGET_LOST when the session failed, or you detached, abandoned,
 *   or disposed it.
 *
 * Lifetimes:
 * The texture and device in `out_frame` stay valid until you release or
 * discard the frame, or the session is abandoned or disposed. A view from
 * mln_acquired_frame_view_begin() keeps them valid until
 * mln_acquired_frame_view_end().
 */
MLN_BINDING("view_owner=frame;platforms=ios,macos,tvos,visionos")
MLN_API mln_status mln_acquired_frame_get_metal_texture(
  mln_acquired_frame frame,
  mln_metal_texture_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gets the Vulkan image of an acquired frame.
 *
 * Returns:
 * - MLN_STATUS_TARGET_LOST when the session failed, or you detached, abandoned,
 *   or disposed it.
 *
 * Lifetimes:
 * The image, view, and device in `out_frame` stay valid until you release or
 * discard the frame, or the session is abandoned or disposed. A view from
 * mln_acquired_frame_view_begin() keeps them valid until
 * mln_acquired_frame_view_end().
 */
MLN_BINDING(
  "view_owner=frame;platforms=android,freebsd,linux,macos,ohos,windows"
)
MLN_API mln_status mln_acquired_frame_get_vulkan_texture(
  mln_acquired_frame frame,
  mln_vulkan_texture_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gets the OpenGL texture of an acquired frame.
 *
 * Returns:
 * - MLN_STATUS_TARGET_LOST when the session failed, or you detached, abandoned,
 *   or disposed it.
 *
 * Lifetimes:
 * The texture in `out_frame` stays valid until you release or discard the
 * frame, or the session is abandoned or disposed. A view from
 * mln_acquired_frame_view_begin() keeps it valid until
 * mln_acquired_frame_view_end().
 */
MLN_BINDING("view_owner=frame")
MLN_API mln_status mln_acquired_frame_get_open_gl_texture(
  mln_acquired_frame frame,
  mln_open_gl_texture_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gets the WebGPU texture of an acquired frame.
 *
 * Returns:
 * - MLN_STATUS_TARGET_LOST when the session failed, or you detached, abandoned,
 *   or disposed it.
 *
 * Lifetimes:
 * The texture, view, and device in `out_frame` stay valid until you release or
 * discard the frame, or the session is abandoned or disposed. A view from
 * mln_acquired_frame_view_begin() keeps them valid until
 * mln_acquired_frame_view_end().
 */
MLN_BINDING("view_owner=frame;platforms=web")
MLN_API mln_status mln_acquired_frame_get_web_gpu_texture(
  mln_acquired_frame frame,
  mln_web_gpu_texture_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_TEXTURE_H
