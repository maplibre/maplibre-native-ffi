/**
 * @file maplibre_native_c/texture.h
 * Public C API declarations for texture render targets.
 */

#ifndef MAPLIBRE_NATIVE_C_TEXTURE_H
#define MAPLIBRE_NATIVE_C_TEXTURE_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Metal texture session attachment options for a session-owned target. */
typedef struct mln_metal_owned_texture_descriptor {
  uint32_t size;
  /** Logical texture extent. */
  mln_render_target_extent extent;
  /** Metal backend context. device is required. */
  mln_metal_context_descriptor context;
} mln_metal_owned_texture_descriptor;

/** Metal caller-owned texture session attachment options. */
typedef struct mln_metal_borrowed_texture_descriptor {
  uint32_t size;
  /** Logical texture extent. */
  mln_render_target_extent extent;
  /**
   * Borrowed id<MTLTexture> / MTL::Texture*. Required.
   *
   * The texture's physical pixel dimensions must match extent, and the texture
   * must allow render-target usage. The caller owns the texture and must keep
   * it valid until detach or destroy.
   */
  void* texture;
} mln_metal_borrowed_texture_descriptor;

/** Metal frame acquired from a session-owned texture target. */
typedef struct mln_metal_owned_texture_frame {
  uint32_t size;
  /** Session generation that produced this frame. */
  uint64_t generation;
  /** Physical Metal texture width in device pixels. */
  uint32_t width;
  /** Physical Metal texture height in device pixels. */
  uint32_t height;
  /** UI-to-device pixel scale used for this frame. */
  double scale_factor;
  /** Opaque frame identity used to reject stale releases. */
  uint64_t frame_id;
  /** Borrowed id<MTLTexture> / MTL::Texture*. Valid until frame release. */
  void* texture;
  /** Borrowed id<MTLDevice> / MTL::Device*. Valid until frame release. */
  void* device;
  /** Backend-native pixel format value. Metal uses MTLPixelFormat. */
  uint64_t pixel_format;
} mln_metal_owned_texture_frame;

/** Vulkan texture session attachment options for a session-owned target. */
typedef struct mln_vulkan_owned_texture_descriptor {
  uint32_t size;
  /** Logical texture extent. */
  mln_render_target_extent extent;
  /** Borrowed Vulkan context. All handles are required. */
  mln_vulkan_context_descriptor context;
} mln_vulkan_owned_texture_descriptor;

/** Vulkan caller-owned texture session attachment options. */
typedef struct mln_vulkan_borrowed_texture_descriptor {
  uint32_t size;
  /** Logical texture extent. */
  mln_render_target_extent extent;
  /** Borrowed Vulkan context. All handles are required. */
  mln_vulkan_context_descriptor context;
  /**
   * Borrowed VkImage. Required.
   *
   * The image must be a 2D, single-sample color image with
   * VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT. Its physical dimensions must match
   * extent. Include VK_IMAGE_USAGE_SAMPLED_BIT when the host will sample from
   * the image after rendering.
   */
  void* image;
  /**
   * Borrowed VkImageView for image. Required.
   *
   * The view must be a 2D color view that matches image and format.
   */
  void* image_view;
  /** Backend-native VkFormat value for image. VK_FORMAT_UNDEFINED is invalid.
   */
  uint32_t format;
  /**
   * Backend-native VkImageLayout value expected at render-pass begin.
   *
   * Use VK_IMAGE_LAYOUT_UNDEFINED when the previous image contents may be
   * discarded.
   */
  uint32_t initial_layout;
  /** Backend-native VkImageLayout value left after rendering succeeds. */
  uint32_t final_layout;
} mln_vulkan_borrowed_texture_descriptor;

/** Vulkan frame acquired from a session-owned texture target. */
typedef struct mln_vulkan_owned_texture_frame {
  uint32_t size;
  /** Session generation that produced this frame. */
  uint64_t generation;
  /** Physical Vulkan image width in device pixels. */
  uint32_t width;
  /** Physical Vulkan image height in device pixels. */
  uint32_t height;
  /** UI-to-device pixel scale used for this frame. */
  double scale_factor;
  /** Opaque frame identity used to reject stale releases. */
  uint64_t frame_id;
  /** Borrowed VkImage. Valid until frame release. */
  void* image;
  /** Borrowed VkImageView. Valid until frame release. */
  void* image_view;
  /** Borrowed VkDevice. Valid until frame release. */
  void* device;
  /** Backend-native VkFormat value. */
  uint32_t format;
  /** Backend-native VkImageLayout value; Vulkan frames are host-sampleable. */
  uint32_t layout;
} mln_vulkan_owned_texture_frame;

/** CPU image readback metadata for a texture session frame. */
typedef struct mln_texture_image_info {
  uint32_t size;
  /** Physical image width in device pixels. */
  uint32_t width;
  /** Physical image height in device pixels. */
  uint32_t height;
  /** Bytes per image row. */
  uint32_t stride;
  /** Required output buffer byte length. */
  size_t byte_length;
} mln_texture_image_info;

/**
 * Returns Metal owned-texture descriptor defaults for this C API version.

 */
MLN_API mln_metal_owned_texture_descriptor
mln_metal_owned_texture_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Returns Metal borrowed-texture descriptor defaults for this C API version.
 */
MLN_API mln_metal_borrowed_texture_descriptor
mln_metal_borrowed_texture_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Returns Vulkan owned-texture descriptor defaults for this C API version.
 */
MLN_API mln_vulkan_owned_texture_descriptor
mln_vulkan_owned_texture_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Returns Vulkan borrowed-texture descriptor defaults for this C API version.
 */
MLN_API mln_vulkan_borrowed_texture_descriptor
mln_vulkan_borrowed_texture_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Returns texture image info defaults for this C API version.
 */
MLN_API mln_texture_image_info
mln_texture_image_info_default(void) MLN_NOEXCEPT;

/**
 * Attaches a Metal texture render target owned by the session to a map.
 *
 * The map may have at most one live render session. The session and
 * every texture-session call are owner-thread affine to the map owner thread.
 * The session renders into a session-owned texture created on
 * descriptor->context.device. On success, *out_session receives a handle the
 * caller destroys with mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_UNSUPPORTED when Metal texture sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_metal_owned_texture_attach(
  mln_map* map, const mln_metal_owned_texture_descriptor* descriptor,
  mln_render_session** out_session
) MLN_NOEXCEPT;

/**
 * Attaches a Metal caller-owned texture render target to a map.
 *
 * The map may have at most one live render session. The session and
 * every texture-session call are owner-thread affine to the map owner thread.
 * The session renders into descriptor->texture. The caller owns the texture,
 * keeps it valid until detach or destroy, and synchronizes any use outside this
 * session. On success, *out_session receives a handle the caller destroys with
 * mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_UNSUPPORTED when Metal borrowed texture sessions are not
 *   supported by this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_metal_borrowed_texture_attach(
  mln_map* map, const mln_metal_borrowed_texture_descriptor* descriptor,
  mln_render_session** out_session
) MLN_NOEXCEPT;

/**
 * Attaches a Vulkan texture render target owned by the session to a map.
 *
 * The map may have at most one live render session. The session and
 * every texture-session call are owner-thread affine to the map owner thread.
 * The session renders into a session-owned image created on
 * descriptor->context.device. Vulkan handles are borrowed and must remain valid
 * until detach or destroy. On success, *out_session receives a handle the
 * caller destroys with mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_UNSUPPORTED when Vulkan texture sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_vulkan_owned_texture_attach(
  mln_map* map, const mln_vulkan_owned_texture_descriptor* descriptor,
  mln_render_session** out_session
) MLN_NOEXCEPT;

/**
 * Attaches a Vulkan caller-owned texture render target to a map.
 *
 * The map may have at most one live render session. The session and
 * every texture-session call are owner-thread affine to the map owner thread.
 * The session renders into descriptor->image through descriptor->image_view.
 * The caller owns the image and view, keeps them valid until detach or destroy,
 * and handles queue-family ownership and synchronization outside this session.
 * On success, *out_session receives a handle the caller destroys with
 * mln_render_session_destroy().
 *
 * Before each mln_render_session_render_update(), make the image available on
 * descriptor->context.graphics_queue in descriptor->initial_layout and keep it
 * out of concurrent use. The session submits rendering on that queue, waits for
 * the submitted work to finish, and leaves the image in
 * descriptor->final_layout before mln_render_session_render_update() returns.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_UNSUPPORTED when Vulkan borrowed texture sessions are not
 *   supported by this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_vulkan_borrowed_texture_attach(
  mln_map* map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  mln_render_session** out_session
) MLN_NOEXCEPT;

/**
 * Reads the most recently rendered session-owned texture frame into
 * caller-owned storage.
 *
 * The copied image is premultiplied RGBA8 in physical pixels. The function
 * fills out_info with the required byte length and image layout metadata. When
 * out_data is null or out_data_capacity is too small, out_info is still filled
 * and the function returns MLN_STATUS_INVALID_ARGUMENT.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, out_info is
 *   null, out_info->size is too small, out_data is null, or out_data_capacity
 *   is too small.
 * - MLN_STATUS_INVALID_STATE when no rendered frame is available, the session
 *   is detached, a frame is currently acquired, or readback produces no image.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when session is not a texture session or when the
 *   texture session uses a caller-owned target.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_texture_read_premultiplied_rgba8(
  mln_render_session* session, uint8_t* out_data, size_t out_data_capacity,
  mln_texture_image_info* out_info
) MLN_NOEXCEPT;

/**
 * Acquires the most recently rendered Metal texture frame.
 *
 * Use this function with sessions created by mln_metal_owned_texture_attach().
 *
 * The returned texture and device pointers are borrowed and remain valid only
 * until mln_metal_owned_texture_release_frame() is called for the same frame.
 * While acquired, resize, render update, detach, destroy, and a
 * second acquire return MLN_STATUS_INVALID_STATE.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, out_frame is
 *   null, or out_frame->size is too small.
 * - MLN_STATUS_INVALID_STATE when the session is detached, no rendered frame is
 *   available, or a texture frame is already acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when session cannot expose a Metal texture frame.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_metal_owned_texture_acquire_frame(
  mln_render_session* session, mln_metal_owned_texture_frame* out_frame
) MLN_NOEXCEPT;

/**
 * Releases a Metal texture frame acquired from a session-owned texture target.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, frame is
 *   null, frame->size is too small, or frame identity does not match the
 *   acquired frame.
 * - MLN_STATUS_INVALID_STATE when no texture frame is currently acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when session cannot release a Metal texture frame.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_metal_owned_texture_release_frame(
  mln_render_session* session, const mln_metal_owned_texture_frame* frame
) MLN_NOEXCEPT;

/**
 * Acquires the most recently rendered Vulkan texture frame.
 *
 * Use this function with sessions created by mln_vulkan_owned_texture_attach().
 *
 * The returned image, image view, and device pointers are borrowed and remain
 * valid only until mln_vulkan_owned_texture_release_frame() is called for the
 * same frame. While acquired, resize, render update, detach, destroy, and a
 * second acquire return MLN_STATUS_INVALID_STATE.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, out_frame is
 *   null, or out_frame->size is too small.
 * - MLN_STATUS_INVALID_STATE when the session is detached, no rendered frame is
 *   available, or a texture frame is already acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when session cannot expose a Vulkan texture frame.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_vulkan_owned_texture_acquire_frame(
  mln_render_session* session, mln_vulkan_owned_texture_frame* out_frame
) MLN_NOEXCEPT;

/**
 * Releases a Vulkan texture frame acquired from a session-owned texture target.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, frame is
 *   null, frame->size is too small, or frame identity does not match the
 *   acquired frame.
 * - MLN_STATUS_INVALID_STATE when no texture frame is currently acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when session cannot release a Vulkan texture frame.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_vulkan_owned_texture_release_frame(
  mln_render_session* session, const mln_vulkan_owned_texture_frame* frame
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_TEXTURE_H
