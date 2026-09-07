/**
 * @file maplibre_native_c/texture.h
 * Public C API declarations for texture render targets.
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
   * Borrowed OpenGL context provider data. Shared ownership creates a context
   * whose texture frames the host can acquire. Dedicated EGL or transferred
   * WebGL ownership creates a private core-worker context for CPU readback.
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

/** Texture readback borrowed for a completion callback. */
typedef struct mln_texture_readback_result {
  uint32_t size;
  uint32_t reserved;
  /** Borrowed pixel bytes, valid only during the callback. */
  mln_buffer_view data;
  mln_texture_image_info info;
} mln_texture_readback_result;

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
 * Starts attachment of a session-owned Metal texture ring.
 *
 * The common options select driver placement and requested ring depth. The
 * descriptor and options are copied before return.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; the stated extent is not positive or scales past uint32_t;
 *   out_session is null or does not point to the null handle; or the requested
 *   driver kind is unknown.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Metal backend.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_metal_owned_texture_attach(
  mln_map map, const mln_metal_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts attachment of a caller-owned Metal texture target.
 *
 * The session renders into the descriptor's texture and grants neither frame
 * acquisition nor readback. The descriptor and options are copied before
 * return.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; the stated extent is not positive or scales past uint32_t;
 *   out_session is null or does not point to the null handle; or the requested
 *   driver kind is unknown.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Metal backend.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_metal_borrowed_texture_attach(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts attachment of a session-owned Vulkan texture ring.
 *
 * The common options select driver placement and requested ring depth. The
 * descriptor and options are copied before return.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; the stated extent is not positive or scales past uint32_t;
 *   out_session is null or does not point to the null handle; or the requested
 *   driver kind is unknown.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Vulkan backend.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_vulkan_owned_texture_attach(
  mln_map map, const mln_vulkan_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts attachment of a caller-owned Vulkan texture target.
 *
 * The session renders into the descriptor's image and grants neither frame
 * acquisition nor readback. The descriptor and options are copied before
 * return.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; the stated extent is not positive or scales past uint32_t;
 *   out_session is null or does not point to the null handle; or the requested
 *   driver kind is unknown.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Vulkan backend.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_vulkan_borrowed_texture_attach(
  mln_map map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts attachment of a session-owned OpenGL texture ring.
 *
 * Shared WGL, EGL, and existing WebGL contexts require the caller driver and
 * grant frame acquisition, readback, and consumer synchronization. Dedicated
 * EGL and transferred WebGL contexts require the core-worker driver and grant
 * readback only with a ring depth of one.
 *
 * The host keeps every descriptor-named backend handle valid through completed
 * detach. In particular, it keeps an EGLDisplay initialized while its session
 * is live.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; the stated extent is not positive or scales past uint32_t;
 *   out_session is null or does not point to the null handle; or the requested
 *   driver kind is unknown.
 * - MLN_STATUS_UNSUPPORTED when this build carries no OpenGL backend, its
 *   context provider is unavailable, or the requested driver does not match the
 *   context placement.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_opengl_owned_texture_attach(
  mln_map map, const mln_opengl_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts attachment of a caller-owned OpenGL texture target.
 *
 * The session renders into the descriptor's texture, which must be a
 * GL_TEXTURE_2D, and grants neither frame acquisition nor readback. The
 * descriptor and options are copied before return.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; the stated extent is not positive or scales past uint32_t;
 *   out_session is null or does not point to the null handle; or the requested
 *   driver kind is unknown.
 * - MLN_STATUS_UNSUPPORTED when this build carries no OpenGL backend, its
 *   context provider is unavailable, or the requested driver is not
 *   MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_opengl_borrowed_texture_attach(
  mln_map map, const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts attachment of a session-owned WebGPU texture ring.
 *
 * Browser targets require MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; the stated extent is not positive or scales past uint32_t;
 *   out_session is null or does not point to the null handle; or the requested
 *   driver kind is unknown.
 * - MLN_STATUS_UNSUPPORTED when this build carries no WebGPU backend, or the
 *   requested driver is not MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_webgpu_owned_texture_attach(
  mln_map map, const mln_webgpu_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts attachment of a caller-owned WebGPU texture target.
 *
 * The session renders into the descriptor's texture and grants neither frame
 * acquisition nor readback. Browser targets require
 * MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD. The descriptor and options are
 * copied before return.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; the stated extent is not positive or scales past uint32_t;
 *   out_session is null or does not point to the null handle; or the requested
 *   driver kind is unknown.
 * - MLN_STATUS_UNSUPPORTED when this build carries no WebGPU backend, or the
 *   requested driver is not MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_webgpu_borrowed_texture_attach(
  mln_map map, const mln_webgpu_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered caller-owned Metal texture replacement.
 *
 * The descriptor is copied before return. The replacement must belong to the
 * device this session attached with.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live; descriptor or
 *   completion is null or undersized; a required backend handle is null; or
 *   the stated physical size is not positive.
 * - MLN_STATUS_INVALID_STATE when the session is not attached, or a texture
 *   frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Metal backend, the
 *   session does not render into a caller-owned texture, or the replacement
 * does not have the pixel format this session compiled its pipeline states for.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders into the new texture.
 * - MLN_STATUS_UNSUPPORTED when the backend cannot keep the GPU state it
 *   compiled for the old one.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_metal_borrowed_texture_set_target(
  mln_render_session session,
  const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered caller-owned Vulkan texture replacement.
 *
 * The descriptor is copied before return. The replacement must name the context
 * this session attached with.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live; descriptor or
 *   completion is null or undersized; a required backend handle is null; or
 *   the stated physical size is not positive.
 * - MLN_STATUS_INVALID_STATE when the session is not attached, or a texture
 *   frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Vulkan backend, the
 *   session does not render into a caller-owned texture, or the replacement
 * does not have the format and layouts this session built its render pass for.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders into the new texture.
 * - MLN_STATUS_UNSUPPORTED when the backend cannot keep the GPU state it
 *   compiled for the old one.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_vulkan_borrowed_texture_set_target(
  mln_render_session session,
  const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered caller-owned OpenGL texture replacement.
 *
 * The descriptor is copied before return. The replacement must be a
 * GL_TEXTURE_2D in the context this session attached with.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live; descriptor or
 *   completion is null or undersized; a required backend handle is null; or
 *   the stated physical size is not positive.
 * - MLN_STATUS_INVALID_STATE when the session is not attached, or a texture
 *   frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no OpenGL backend, or the
 *   session does not render into a caller-owned texture.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders into the new texture.
 * - MLN_STATUS_UNSUPPORTED when the backend cannot keep the GPU state it
 *   compiled for the old one.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_opengl_borrowed_texture_set_target(
  mln_render_session session,
  const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered caller-owned WebGPU texture replacement.
 *
 * The descriptor is copied before return. The replacement must name the device
 * and queue this session attached with.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live; descriptor or
 *   completion is null or undersized; a required backend handle is null; or
 *   the stated physical size is not positive.
 * - MLN_STATUS_INVALID_STATE when the session is not attached, or a texture
 *   frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no WebGPU backend, the
 *   session does not render into a caller-owned texture, or the replacement
 * does not have the format this session built its render pipelines for.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders into the new texture.
 * - MLN_STATUS_UNSUPPORTED when the backend cannot keep the GPU state it
 *   compiled for the old one.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_webgpu_borrowed_texture_set_target(
  mln_render_session session,
  const mln_webgpu_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts readback of the latest rendered texture frame.
 *
 * The completion delivers one mln_texture_readback_result as its value, with a
 * value_count of one. Its pixel bytes are borrowed for the duration of the
 * callback; copy anything the host keeps.
 *
 * Returns:
 * - MLN_STATUS_OK when the readback is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live, or completion is null
 *   or undersized.
 * - MLN_STATUS_INVALID_STATE when the session is not attached.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK and one mln_texture_readback_result.
 * - MLN_STATUS_UNSUPPORTED when the target is not a session-owned texture ring
 *   or its backend cannot read back.
 * - MLN_STATUS_INVALID_STATE when no frame has been rendered at the session's
 *   current generation.
 * - MLN_STATUS_NATIVE_ERROR when the read fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_texture_read_premultiplied_rgba8(
  mln_render_session session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies Metal-native metadata from an acquired frame.
 *
 * The texture and device pointers are borrowed and remain valid only until
 * mln_acquired_frame_release().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is not live or already released, or
 *   out_frame is null or undersized.
 * - MLN_STATUS_UNSUPPORTED when the frame was produced by a different render
 *   backend.
 * - MLN_STATUS_TARGET_LOST when the session lost or abandoned its target.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_acquired_frame_get_metal_texture(
  mln_acquired_frame frame, mln_metal_owned_texture_frame* out_frame
) MLN_NOEXCEPT;

/**
 * Copies Vulkan-native metadata from an acquired frame.
 *
 * The image and image view handles and the device pointer are borrowed and
 * remain valid only until mln_acquired_frame_release().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is not live or already released, or
 *   out_frame is null or undersized.
 * - MLN_STATUS_UNSUPPORTED when the frame was produced by a different render
 *   backend.
 * - MLN_STATUS_TARGET_LOST when the session lost or abandoned its target.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_acquired_frame_get_vulkan_texture(
  mln_acquired_frame frame, mln_vulkan_owned_texture_frame* out_frame
) MLN_NOEXCEPT;

/**
 * Copies OpenGL-native metadata from an acquired frame.
 *
 * The caller driver's context must be current on this thread. The texture name
 * is borrowed and remains valid only until mln_acquired_frame_release().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is not live or already released, or
 *   out_frame is null or undersized.
 * - MLN_STATUS_UNSUPPORTED when the frame was produced by a different render
 *   backend.
 * - MLN_STATUS_TARGET_LOST when the session lost or abandoned its target.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_acquired_frame_get_opengl_texture(
  mln_acquired_frame frame, mln_opengl_owned_texture_frame* out_frame
) MLN_NOEXCEPT;

/**
 * Copies WebGPU-native metadata from an acquired frame.
 *
 * The texture, view, and device pointers are borrowed and remain valid only
 * until mln_acquired_frame_release().
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is not live or already released, or
 *   out_frame is null or undersized.
 * - MLN_STATUS_UNSUPPORTED when the frame was produced by a different render
 *   backend.
 * - MLN_STATUS_TARGET_LOST when the session lost or abandoned its target.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_acquired_frame_get_webgpu_texture(
  mln_acquired_frame frame, mln_webgpu_owned_texture_frame* out_frame
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_TEXTURE_H
