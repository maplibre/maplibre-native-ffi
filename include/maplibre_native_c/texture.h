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
  /**
   * Logical texture extent. The map viewport uses width and height and the
   * renderer uses scale_factor; the physical size is stated separately below.
   */
  mln_render_target_extent extent;
  /** Physical texture width in device pixels. Must be positive. */
  uint32_t physical_width;
  /** Physical texture height in device pixels. Must be positive. */
  uint32_t physical_height;
  /**
   * Borrowed id<MTLTexture> / MTL::Texture*. Required.
   *
   * The texture's pixel dimensions must equal physical_width and
   * physical_height, the texture must allow render-target usage, and it must be
   * single-sample, because the session builds single-sample depth and stencil
   * attachments to match. The session reads all three from the texture and
   * rejects a mismatch. The caller owns the texture and must keep it valid
   * until detach or destroy.
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
  /**
   * Logical texture extent. The map viewport uses width and height and the
   * renderer uses scale_factor; the physical size is stated separately below.
   */
  mln_render_target_extent extent;
  /** Physical image width in device pixels. Must be positive. */
  uint32_t physical_width;
  /** Physical image height in device pixels. Must be positive. */
  uint32_t physical_height;
  /** Borrowed Vulkan context. All handles are required. */
  mln_vulkan_context_descriptor context;
  /**
   * Borrowed VkImage. Required.
   *
   * The image must be a 2D, single-sample color image with
   * VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT. Its dimensions must equal
   * physical_width and physical_height. Include VK_IMAGE_USAGE_SAMPLED_BIT when
   * the host will sample from the image after rendering.
   *
   * A VkImage exposes no queryable extent, so the caller guarantees the stated
   * physical size: the session builds a framebuffer at that size, and Vulkan
   * leaves a framebuffer larger than its attachment undefined.
   */
  mln_vulkan_non_dispatchable_handle image;
  /**
   * Borrowed VkImageView for image. Required.
   *
   * The view must be a 2D color view that matches image and format.
   */
  mln_vulkan_non_dispatchable_handle image_view;
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
  /** Borrowed VkImage bit pattern. Valid until frame release. */
  mln_vulkan_non_dispatchable_handle image;
  /** Borrowed VkImageView bit pattern. Valid until frame release. */
  mln_vulkan_non_dispatchable_handle image_view;
  /** Borrowed VkDevice. Valid until frame release. */
  void* device;
  /** Backend-native VkFormat value. */
  uint32_t format;
  /** Backend-native VkImageLayout value; Vulkan frames are host-sampleable. */
  uint32_t layout;
} mln_vulkan_owned_texture_frame;

/** OpenGL texture session attachment options for a session-owned target. */
typedef struct mln_opengl_owned_texture_descriptor {
  uint32_t size;
  /** Logical texture extent. */
  mln_render_target_extent extent;
  /**
   * Borrowed OpenGL context provider data. The session creates and owns a
   * context that shares texture objects with the host context.
   */
  mln_opengl_context_descriptor context;
} mln_opengl_owned_texture_descriptor;

/** OpenGL caller-owned texture session attachment options. */
typedef struct mln_opengl_borrowed_texture_descriptor {
  uint32_t size;
  /**
   * Logical texture extent. The map viewport uses width and height and the
   * renderer uses scale_factor; the physical size is stated separately below.
   */
  mln_render_target_extent extent;
  /** Physical texture width in device pixels. Must be positive. */
  uint32_t physical_width;
  /** Physical texture height in device pixels. Must be positive. */
  uint32_t physical_height;
  /**
   * Borrowed OpenGL context provider data. The texture must belong to this
   * context or a context in the same share group.
   */
  mln_opengl_context_descriptor context;
  /**
   * Borrowed OpenGL texture object name. Required.
   *
   * The texture's level-0 dimensions must equal physical_width and
   * physical_height.
   *
   * Querying texture dimensions needs glGetTexLevelParameteriv, absent before
   * OpenGL ES 3.1, so the caller guarantees the stated physical size: the
   * session renders through a framebuffer at that size, and a smaller texture
   * clips or garbles output.
   */
  uint32_t texture;
  /** OpenGL texture target. GL_TEXTURE_2D is the expected target. */
  uint32_t target;
} mln_opengl_borrowed_texture_descriptor;

/** WebGPU texture session attachment options for a session-owned target. */
typedef struct mln_webgpu_owned_texture_descriptor {
  uint32_t size;
  /** Logical texture extent. */
  mln_render_target_extent extent;
  /** Borrowed WebGPU context. device is required. */
  mln_webgpu_context_descriptor context;
} mln_webgpu_owned_texture_descriptor;

/** WebGPU caller-owned texture session attachment options. */
typedef struct mln_webgpu_borrowed_texture_descriptor {
  uint32_t size;
  /** Logical texture extent. */
  mln_render_target_extent extent;
  /** Physical texture width in device pixels. */
  uint32_t physical_width;
  /** Physical texture height in device pixels. */
  uint32_t physical_height;
  /** Borrowed WebGPU context. device is required. */
  mln_webgpu_context_descriptor context;
  /**
   * Borrowed WGPUTexture. Required.
   *
   * The texture and view must be created by context.device, and rendering is
   * submitted through context.queue or that device's default queue. The texture
   * must be 2D, single-sample, and render-attachment capable. Its physical
   * dimensions and format must match this descriptor. Include TextureBinding
   * usage when the host will sample from the texture after rendering.
   */
  void* texture;
  /**
   * Borrowed WGPUTextureView for texture. Required.
   *
   * The view must be a 2D color view compatible with texture and format.
   */
  void* texture_view;
  /** Backend-native WGPUTextureFormat value. Undefined is invalid. */
  uint32_t format;
} mln_webgpu_borrowed_texture_descriptor;

/** WebGPU frame acquired from a session-owned texture target. */
typedef struct mln_webgpu_owned_texture_frame {
  uint32_t size;
  /** Session generation that produced this frame. */
  uint64_t generation;
  /** Physical WebGPU texture width in device pixels. */
  uint32_t width;
  /** Physical WebGPU texture height in device pixels. */
  uint32_t height;
  /** UI-to-device pixel scale used for this frame. */
  double scale_factor;
  /** Opaque frame identity used to reject stale releases. */
  uint64_t frame_id;
  /** Borrowed WGPUTexture. Valid until frame release. */
  void* texture;
  /** Borrowed WGPUTextureView. Valid until frame release. */
  void* texture_view;
  /** Borrowed WGPUDevice. Valid until frame release. */
  void* device;
  /** Backend-native WGPUTextureFormat value. */
  uint32_t format;
} mln_webgpu_owned_texture_frame;

/** OpenGL frame acquired from a session-owned texture target. */
typedef struct mln_opengl_owned_texture_frame {
  uint32_t size;
  /** Session generation that produced this frame. */
  uint64_t generation;
  /** Physical OpenGL texture width in device pixels. */
  uint32_t width;
  /** Physical OpenGL texture height in device pixels. */
  uint32_t height;
  /** UI-to-device pixel scale used for this frame. */
  double scale_factor;
  /** Opaque frame identity used to reject stale releases. */
  uint64_t frame_id;
  /** Borrowed OpenGL texture object name. Valid until frame release. */
  uint32_t texture;
  /** OpenGL texture target. GL_TEXTURE_2D is the expected target. */
  uint32_t target;
  /** OpenGL internal format, such as GL_RGBA8. */
  uint32_t internal_format;
  /** OpenGL pixel format, such as GL_RGBA. */
  uint32_t format;
  /** OpenGL pixel type, such as GL_UNSIGNED_BYTE. */
  uint32_t type;
} mln_opengl_owned_texture_frame;

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
 * Returns OpenGL owned-texture descriptor defaults for this C API version.
 */
MLN_API mln_opengl_owned_texture_descriptor
mln_opengl_owned_texture_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Returns OpenGL borrowed-texture descriptor defaults for this C API version.
 */
MLN_API mln_opengl_borrowed_texture_descriptor
mln_opengl_borrowed_texture_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Returns WebGPU owned-texture descriptor defaults for this C API version.
 */
MLN_API mln_webgpu_owned_texture_descriptor
mln_webgpu_owned_texture_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Returns WebGPU borrowed-texture descriptor defaults for this C API version.
 */
MLN_API mln_webgpu_borrowed_texture_descriptor
mln_webgpu_borrowed_texture_descriptor_default(void) MLN_NOEXCEPT;

/**
 * Returns texture image info defaults for this C API version.
 */
MLN_API mln_texture_image_info
mln_texture_image_info_default(void) MLN_NOEXCEPT;

/**
 * Attaches a Metal texture render target owned by the session to a map.
 *
 * The map may have at most one live render session. The calling thread becomes
 * the session's owner thread, and every texture-session call is affine to it.
 * The map need only be live, so a host may attach on the thread that drives its
 * render loop while the map stays on the runtime loop thread. Attach creates
 * the session's graphics resources on the calling thread, so the host resources
 * named by descriptor must be usable there. The session renders into a
 * session-owned texture created on descriptor->context.device. On success,
 * *out_session receives a handle the caller destroys with
 * mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_UNSUPPORTED when Metal texture sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_metal_owned_texture_attach(
  mln_map map, const mln_metal_owned_texture_descriptor* descriptor,
  mln_render_session* out_session
) MLN_NOEXCEPT;

/**
 * Attaches a Metal caller-owned texture render target to a map.
 *
 * The map may have at most one live render session. The calling thread becomes
 * the session's owner thread, and every texture-session call is affine to it.
 * The map need only be live, so a host may attach on the thread that drives its
 * render loop while the map stays on the runtime loop thread. Attach creates
 * the session's graphics resources on the calling thread, so the host resources
 * named by descriptor must be usable there. The session renders into
 * descriptor->texture. The caller owns the texture, keeps it valid until detach
 * or destroy, and synchronizes any use outside this session. On success,
 * *out_session receives a handle the caller destroys with
 * mln_render_session_destroy().
 *
 * mln_render_session_resize() returns MLN_STATUS_UNSUPPORTED for this target,
 * which the host owns and sizes. Follow a resized host by allocating a texture
 * at the new size and handing it over with
 * mln_metal_borrowed_texture_set_target(), which keeps the session.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_UNSUPPORTED when Metal borrowed texture sessions are not
 *   supported by this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_metal_borrowed_texture_attach(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  mln_render_session* out_session
) MLN_NOEXCEPT;

/**
 * Attaches a Vulkan texture render target owned by the session to a map.
 *
 * The map may have at most one live render session. The calling thread becomes
 * the session's owner thread, and every texture-session call is affine to it.
 * The map need only be live, so a host may attach on the thread that drives its
 * render loop while the map stays on the runtime loop thread. Attach creates
 * the session's graphics resources on the calling thread, so the host resources
 * named by descriptor must be usable there. The session renders into a
 * session-owned image created on descriptor->context.device. Vulkan handles are
 * borrowed and must remain valid until detach or destroy. On success,
 * *out_session receives a handle the caller destroys with
 * mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_UNSUPPORTED when Vulkan texture sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_vulkan_owned_texture_attach(
  mln_map map, const mln_vulkan_owned_texture_descriptor* descriptor,
  mln_render_session* out_session
) MLN_NOEXCEPT;

/**
 * Attaches a Vulkan caller-owned texture render target to a map.
 *
 * The map may have at most one live render session. The calling thread becomes
 * the session's owner thread, and every texture-session call is affine to it.
 * The map need only be live, so a host may attach on the thread that drives its
 * render loop while the map stays on the runtime loop thread. Attach creates
 * the session's graphics resources on the calling thread, so the host resources
 * named by descriptor must be usable there. The session renders into
 * descriptor->image through descriptor->image_view. The caller owns the image
 * and view, keeps them valid until detach or destroy, and handles queue-family
 * ownership and synchronization outside this session. On success, *out_session
 * receives a handle the caller destroys with mln_render_session_destroy().
 *
 * Before each mln_render_session_render_update(), make the image available on
 * descriptor->context.graphics_queue in descriptor->initial_layout and keep it
 * out of concurrent use. The session submits rendering on that queue, waits for
 * the submitted work to finish, and leaves the image in
 * descriptor->final_layout before mln_render_session_render_update() returns.
 *
 * mln_render_session_resize() returns MLN_STATUS_UNSUPPORTED for this target,
 * which the host owns and sizes. Follow a resized host by allocating an image
 * and view at the new size and handing them over with
 * mln_vulkan_borrowed_texture_set_target(), which keeps the session.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_UNSUPPORTED when Vulkan borrowed texture sessions are not
 *   supported by this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_vulkan_borrowed_texture_attach(
  mln_map map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  mln_render_session* out_session
) MLN_NOEXCEPT;

/**
 * Attaches an OpenGL texture render target owned by the session to a map.
 *
 * The map may have at most one live render session. The calling thread becomes
 * the session's owner thread, and every texture-session call is affine to it.
 * The map need only be live, so a host may attach on the thread that drives its
 * render loop while the map stays on the runtime loop thread. Attach creates
 * the session's graphics resources on the calling thread, so the host resources
 * named by descriptor must be usable there, so the host context must be current
 * on this thread. The session creates an OpenGL texture in a context that
 * shares objects with descriptor->context. Host sampling may
 * use the acquired texture from a context in the same share group after acquire
 * succeeds and before release. On success, *out_session receives a handle the
 * caller destroys with mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_UNSUPPORTED when OpenGL texture sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_opengl_owned_texture_attach(
  mln_map map, const mln_opengl_owned_texture_descriptor* descriptor,
  mln_render_session* out_session
) MLN_NOEXCEPT;

/**
 * Attaches an OpenGL caller-owned texture render target to a map.
 *
 * The map may have at most one live render session. The calling thread becomes
 * the session's owner thread, and every texture-session call is affine to it.
 * The map need only be live, so a host may attach on the thread that drives its
 * render loop while the map stays on the runtime loop thread. Attach creates
 * the session's graphics resources on the calling thread, so the host resources
 * named by descriptor must be usable there, so the host context must be current
 * on this thread. The session renders into descriptor->texture. The caller owns
 * the texture, keeps it valid until detach or destroy, and synchronizes any use
 * outside this session. Each render completes before
 * mln_render_session_render_update() returns, so the caller reads or samples
 * the texture from any context in the share group of descriptor->context
 * without adding synchronization of its own. On success,
 * *out_session receives a handle the caller destroys with
 * mln_render_session_destroy().
 *
 * mln_render_session_resize() returns MLN_STATUS_UNSUPPORTED for this target,
 * which the host owns and sizes. Follow a resized host by allocating a texture
 * at the new size and handing it over with
 * mln_opengl_borrowed_texture_set_target(), which keeps the session.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_UNSUPPORTED when OpenGL borrowed texture sessions are not
 *   supported by this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_opengl_borrowed_texture_attach(
  mln_map map, const mln_opengl_borrowed_texture_descriptor* descriptor,
  mln_render_session* out_session
) MLN_NOEXCEPT;

/**
 * Renders an attached Metal texture session into a new caller-owned texture.
 *
 * A caller-owned texture is sized by its owner, so mln_render_session_resize()
 * reports MLN_STATUS_UNSUPPORTED for these targets and a host that follows a
 * resize hands over a replacement here instead. The texture is replaced in
 * place, so the session keeps its renderer along with the tile pyramid, glyph
 * and image atlases, and symbol placement. Map-owned feature state is
 * unchanged.
 *
 * descriptor->texture must belong to the device the session attached with. A
 * texture on a different device requires destroying this session with
 * mln_render_session_destroy() and attaching again.
 *
 * The caller owns the replacement, keeps it valid until the next replacement,
 * detach, or destroy, and synchronizes any use outside this session, exactly as
 * for mln_metal_borrowed_texture_attach(). The outgoing texture is neither read
 * nor released here and need not still be valid.
 *
 * The new extent applies exactly as mln_render_session_resize() applies one,
 * including how the next mln_render_session_render_update() waits for the map
 * to catch up to it. A scale_factor that differs from the session's current
 * value rebuilds the renderer, whose shaders are compiled for a fixed pixel
 * ratio. A pixel format that differs from the one this session attached with is
 * reported as MLN_STATUS_UNSUPPORTED, with the session still rendering into the
 * texture it has; destroy the session and attach again to change the format.
 *
 * Every failure status but MLN_STATUS_NATIVE_ERROR is reported before the
 * target is touched and leaves the session rendering into the one it had.
 * MLN_STATUS_NATIVE_ERROR may mean a replacement was already under way, which
 * cannot be unwound; destroy the session with mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, descriptor is
 *   null or invalid, or descriptor->texture belongs to another device.
 * - MLN_STATUS_INVALID_STATE when the session is detached or a texture frame is
 *   currently acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when the session does not render into a caller-owned
 *   Metal texture, when descriptor->texture has a different pixel format from
 *   the session's, or when Metal borrowed texture sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_metal_borrowed_texture_set_target(
  mln_render_session session,
  const mln_metal_borrowed_texture_descriptor* descriptor
) MLN_NOEXCEPT;

/**
 * Renders an attached Vulkan texture session into a new caller-owned image.
 *
 * See mln_metal_borrowed_texture_set_target() for what replacing a target
 * preserves, when a host reaches for it, and how failures are reported.
 * descriptor->context must name the same instance, physical device, device, and
 * graphics queue the session attached with.
 *
 * The replacement must carry the format and both layouts this session built its
 * render pass around. MLN_STATUS_UNSUPPORTED reports one that does not, with
 * the session still rendering into the image it has; destroy the session and
 * attach again to change them.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, descriptor is
 *   null or invalid, or descriptor->context names handles other than the
 *   session's.
 * - MLN_STATUS_INVALID_STATE when the session is detached or a texture frame is
 *   currently acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when the session does not render into a caller-owned
 *   Vulkan image, when descriptor->format, descriptor->initial_layout, or
 *   descriptor->final_layout differs from the session's, or when Vulkan
 *   borrowed texture sessions are not supported by this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_vulkan_borrowed_texture_set_target(
  mln_render_session session,
  const mln_vulkan_borrowed_texture_descriptor* descriptor
) MLN_NOEXCEPT;

/**
 * Renders an attached OpenGL texture session into a new caller-owned texture.
 *
 * See mln_metal_borrowed_texture_set_target() for what replacing a target
 * preserves, when a host reaches for it, and how failures are reported.
 * descriptor->context must name the context provider data the session attached
 * with. The replacement belongs to that context or one in the same share group,
 * and the host context must be current on the calling thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, descriptor is
 *   null or invalid, or descriptor->context names context provider data other
 *   than the session's.
 * - MLN_STATUS_INVALID_STATE when the session is detached or a texture frame is
 *   currently acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when the session does not render into a caller-owned
 *   OpenGL texture, or when OpenGL borrowed texture sessions are not supported
 *   by this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_opengl_borrowed_texture_set_target(
  mln_render_session session,
  const mln_opengl_borrowed_texture_descriptor* descriptor
) MLN_NOEXCEPT;

/**
 * Reads the most recently rendered session-owned texture frame into
 * caller-owned storage.
 *
 * The copied image is premultiplied RGBA8 in physical pixels. The function
 * fills out_info with the required byte length and image layout metadata.
 *
 * Passing null for out_data with a capacity of 0 is a size probe: out_info is
 * filled and the call succeeds, so a caller can size a buffer without treating
 * the result as a failure. Otherwise out_info is still filled when out_data is
 * null or out_data_capacity is too small, and the function returns
 * MLN_STATUS_INVALID_ARGUMENT.
 *
 * A backend that cannot read pixels back answers MLN_STATUS_UNSUPPORTED without
 * filling out_info, so a size probe is not a way to ask whether readback works.
 *
 * Returns:
 * - MLN_STATUS_OK on success, including a size probe.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, out_info is
 *   null, out_info->size is too small, out_data is null with non-zero capacity,
 *   or out_data_capacity is too small for a non-null buffer.
 * - MLN_STATUS_INVALID_STATE when no rendered frame is available, the session
 *   is detached, or a frame is currently acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when session is not a texture session, when the
 *   texture session uses a caller-owned target, or when the session's render
 *   backend cannot read pixels back.
 * - MLN_STATUS_NATIVE_ERROR when readback produces no image or an image whose
 *   layout does not match the session, when the render backend reports no
 *   renderer backend, or when an internal exception is converted to status.
 */
MLN_API mln_status mln_texture_read_premultiplied_rgba8(
  mln_render_session session, uint8_t* out_data, size_t out_data_capacity,
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
 * - MLN_STATUS_NATIVE_ERROR when the render backend reports no rendered Metal
 *   texture, or when an internal exception is converted to status.
 */
MLN_API mln_status mln_metal_owned_texture_acquire_frame(
  mln_render_session session, mln_metal_owned_texture_frame* out_frame
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
  mln_render_session session, const mln_metal_owned_texture_frame* frame
) MLN_NOEXCEPT;

/**
 * Acquires the most recently rendered Vulkan texture frame.
 *
 * Use this function with sessions created by mln_vulkan_owned_texture_attach().
 *
 * The returned image and image view handles and device pointer are borrowed and
 * remain valid only until mln_vulkan_owned_texture_release_frame() is called
 * for the same frame. While acquired, resize, render update, detach, destroy,
 * and a second acquire return MLN_STATUS_INVALID_STATE.
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
  mln_render_session session, mln_vulkan_owned_texture_frame* out_frame
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
  mln_render_session session, const mln_vulkan_owned_texture_frame* frame
) MLN_NOEXCEPT;

/**
 * Acquires the most recently rendered OpenGL texture frame.
 *
 * Use this function with sessions created by mln_opengl_owned_texture_attach().
 *
 * The returned texture object is borrowed and remains valid only until
 * mln_opengl_owned_texture_release_frame() is called for the same frame.
 * While acquired, resize, render update, detach, destroy, and a second acquire
 * return MLN_STATUS_INVALID_STATE.
 *
 * Acquiring completes the session rendering for the frame, so the caller reads
 * or samples the texture from any context in the share group of the context
 * passed to mln_opengl_owned_texture_attach() without adding synchronization
 * of its own.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, out_frame is
 *   null, or out_frame->size is too small.
 * - MLN_STATUS_INVALID_STATE when the session is detached, no rendered frame is
 *   available, or a texture frame is already acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when session cannot expose an OpenGL texture frame.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_opengl_owned_texture_acquire_frame(
  mln_render_session session, mln_opengl_owned_texture_frame* out_frame
) MLN_NOEXCEPT;

/**
 * Releases an OpenGL texture frame acquired from a session-owned texture
 * target.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, frame is
 *   null, frame->size is too small, or frame identity does not match the
 *   acquired frame.
 * - MLN_STATUS_INVALID_STATE when no texture frame is currently acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when session cannot release an OpenGL texture frame.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_opengl_owned_texture_release_frame(
  mln_render_session session, const mln_opengl_owned_texture_frame* frame
) MLN_NOEXCEPT;

/**
 * Attaches a WebGPU texture render target owned by the session to a map.
 *
 * The map may have at most one live render session. The calling thread becomes
 * the session's owner thread, and every texture-session call is affine to it.
 * The map need only be live, so a host may attach on the thread that drives its
 * render loop while the map stays on the runtime loop thread. Attach creates
 * the session's graphics resources on the calling thread, so the host resources
 * named by descriptor must be usable there. The session creates a WebGPU
 * texture on descriptor->context.device. The caller owns that device and queue
 * and keeps them valid until detach or destroy. Host sampling or copying may
 * use the acquired texture after acquire succeeds and before release. On
 * success, *out_session receives a handle the caller destroys with
 * mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_UNSUPPORTED when WebGPU texture sessions are not supported by
 *   this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_webgpu_owned_texture_attach(
  mln_map map, const mln_webgpu_owned_texture_descriptor* descriptor,
  mln_render_session* out_session
) MLN_NOEXCEPT;

/**
 * Attaches a WebGPU caller-owned texture render target to a map.
 *
 * The map may have at most one live render session. The calling thread becomes
 * the session's owner thread, and every texture-session call is affine to it.
 * The map need only be live, so a host may attach on the thread that drives its
 * render loop while the map stays on the runtime loop thread.
 *
 * The session renders into descriptor->texture_view. The caller owns the
 * texture, view, device, and queue, keeps them valid until detach or destroy,
 * and synchronizes any use outside this session.
 *
 * Before each mln_render_session_render_update(), make the texture available on
 * descriptor->context.queue and keep it out of concurrent use. The session
 * submits rendering on that queue before mln_render_session_render_update()
 * returns.
 *
 * mln_render_session_resize() returns MLN_STATUS_UNSUPPORTED for this target,
 * which the host owns and sizes. Follow a resized host by allocating a texture
 * and view at the new size and handing them over with
 * mln_webgpu_borrowed_texture_set_target(), which keeps the session.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, descriptor is
 *   null or invalid, out_session is null, or *out_session is not null.
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_UNSUPPORTED when WebGPU borrowed texture sessions are not
 *   supported by this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_webgpu_borrowed_texture_attach(
  mln_map map, const mln_webgpu_borrowed_texture_descriptor* descriptor,
  mln_render_session* out_session
) MLN_NOEXCEPT;

/**
 * Renders an attached WebGPU texture session into a new caller-owned texture.
 *
 * See mln_metal_borrowed_texture_set_target() for what replacing a target
 * preserves, when a host reaches for it, and how failures are reported.
 * descriptor->context must name the device and queue the session attached with.
 * A null queue names that device's default queue here exactly as it does at
 * attach. descriptor->context.instance is not compared, because a texture
 * session never uses it.
 *
 * The replacement must carry the format this session built its render pipelines
 * around. MLN_STATUS_UNSUPPORTED reports one that does not, with the session
 * still rendering into the texture it has; destroy the session and attach again
 * to change it.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, descriptor is
 *   null or invalid, or descriptor->context names a device or queue other than
 *   the session's.
 * - MLN_STATUS_INVALID_STATE when the session is detached or a texture frame is
 *   currently acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when the session does not render into a caller-owned
 *   WebGPU texture, when descriptor->format differs from the session's, or when
 *   WebGPU borrowed texture sessions are not supported by this build.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_webgpu_borrowed_texture_set_target(
  mln_render_session session,
  const mln_webgpu_borrowed_texture_descriptor* descriptor
) MLN_NOEXCEPT;

/**
 * Acquires the most recently rendered WebGPU texture frame.
 *
 * Use this function with sessions created by mln_webgpu_owned_texture_attach().
 *
 * The returned texture, texture view, and device pointers are borrowed and
 * remain valid only until mln_webgpu_owned_texture_release_frame() is called
 * for the same frame. While acquired, resize, render update, detach, destroy,
 * and a second acquire return MLN_STATUS_INVALID_STATE.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, out_frame is
 *   null, or out_frame->size is too small.
 * - MLN_STATUS_INVALID_STATE when the session is detached, no rendered frame is
 *   available, or a texture frame is already acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when session cannot expose a WebGPU texture frame.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_webgpu_owned_texture_acquire_frame(
  mln_render_session session, mln_webgpu_owned_texture_frame* out_frame
) MLN_NOEXCEPT;

/**
 * Releases a WebGPU texture frame acquired from a session-owned texture target.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, frame is
 *   null, frame->size is too small, or frame identity does not match the
 *   acquired frame.
 * - MLN_STATUS_INVALID_STATE when no texture frame is currently acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_UNSUPPORTED when session cannot release a WebGPU texture frame.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_webgpu_owned_texture_release_frame(
  mln_render_session session, const mln_webgpu_owned_texture_frame* frame
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_TEXTURE_H
