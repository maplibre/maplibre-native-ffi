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

/** Metal attachment options for an owned texture target. */
typedef struct mln_metal_owned_texture_descriptor {
  uint32_t size;
  /**
   * Logical texture extent. A scale_factor that differs from the map's is
   * accepted and logged as a warning.
   */
  mln_logical_extent extent;
  /** Metal backend context. device is required. */
  mln_metal_context_descriptor context;
} mln_metal_owned_texture_descriptor;

/** One caller-owned Metal texture of a borrowed texture ring. */
typedef struct mln_metal_borrowed_texture {
  /** Borrowed `id<MTLTexture>` / `MTL::Texture*`. Required. */
  void* texture;
} mln_metal_borrowed_texture;

/** Metal attachment options for a borrowed texture target. */
typedef struct mln_metal_borrowed_texture_descriptor {
  uint32_t size;
  /**
   * Logical texture extent. The map viewport uses width and height and the
   * renderer uses scale_factor; the physical size is stated separately below.
   * A scale_factor that differs from the map's is accepted and logged as a
   * warning.
   */
  mln_logical_extent extent;
  /** Physical texture width in device pixels. Must be positive. Defaults to
   * 256. */
  uint32_t physical_width MLN_BINDING("default=256");
  /** Physical texture height in device pixels. Must be positive. Defaults to
   * 256. */
  uint32_t physical_height MLN_BINDING("default=256");
  /**
   * The ring's textures, one per slot, in slot order. Required.
   *
   * The call copies the array before it returns. Only the textures it names
   * must stay valid: until set_target replaces them, until detach completes,
   * or until abandon returns. The textures are distinct and share one device
   * and one pixel format. Each texture's pixel dimensions equal physical_width
   * and physical_height, and each allows render-target usage and is
   * single-sample, because the session builds single-sample depth and stencil
   * attachments to match. The session reads all of this from the textures and
   * rejects a mismatch.
   */
  const mln_metal_borrowed_texture* textures
    MLN_BINDING("length=texture_count");
  /**
   * Number of textures, which is the ring depth, from one to three. A
   * replacement keeps the count the session attached with.
   */
  size_t texture_count;
} mln_metal_borrowed_texture_descriptor;

/** Metal frame acquired from a texture ring. */
typedef struct mln_metal_texture_frame {
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
  /**
   * Ring slot that holds this frame. For a borrowed target, the index of its
   * texture in the descriptor's textures array.
   */
  uint32_t slot;
  /**
   * Borrowed `id<MTLTexture>` / `MTL::Texture*`. Valid until frame release.
   */
  void* texture;
  /** Borrowed `id<MTLDevice>` / `MTL::Device*`. Valid until frame release. */
  void* device;
  /** Backend-native pixel format value. Metal uses MTLPixelFormat. */
  uint64_t pixel_format;
} mln_metal_texture_frame;

/** Vulkan attachment options for an owned texture target. */
typedef struct mln_vulkan_owned_texture_descriptor {
  uint32_t size;
  /**
   * Logical texture extent. A scale_factor that differs from the map's is
   * accepted and logged as a warning.
   */
  mln_logical_extent extent;
  /** Borrowed Vulkan context. All handles are required. */
  mln_vulkan_context_descriptor context;
} mln_vulkan_owned_texture_descriptor;

/** One caller-owned Vulkan image of a borrowed texture ring. */
typedef struct mln_vulkan_borrowed_texture {
  /**
   * Borrowed VkImage. Required.
   *
   * The image must be a 2D, single-sample color image with
   * VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT. Include VK_IMAGE_USAGE_SAMPLED_BIT
   * when the host will sample from the image after rendering.
   */
  mln_vulkan_non_dispatchable_handle image;
  /**
   * Borrowed VkImageView for image. Required. The view must be a 2D color view
   * that matches image and the descriptor's format.
   */
  mln_vulkan_non_dispatchable_handle image_view;
} mln_vulkan_borrowed_texture;

/** Vulkan attachment options for a borrowed texture target. */
typedef struct mln_vulkan_borrowed_texture_descriptor {
  uint32_t size;
  /**
   * Logical texture extent. The map viewport uses width and height and the
   * renderer uses scale_factor; the physical size is stated separately below.
   * A scale_factor that differs from the map's is accepted and logged as a
   * warning.
   */
  mln_logical_extent extent;
  /** Physical image width in device pixels. Must be positive. Defaults to 256.
   */
  uint32_t physical_width MLN_BINDING("default=256");
  /** Physical image height in device pixels. Must be positive. Defaults to 256.
   */
  uint32_t physical_height MLN_BINDING("default=256");
  /** Borrowed Vulkan context. All handles are required. */
  mln_vulkan_context_descriptor context;
  /**
   * The ring's images, one per slot, in slot order. Required.
   *
   * The call copies the array before it returns. Only the images it names
   * must stay valid: until set_target replaces them, until detach completes,
   * or until abandon returns. The images are distinct, and each image must be
   * usable on context.graphics_queue's family without an ownership transfer
   * (VK_SHARING_MODE_EXCLUSIVE on that family, or CONCURRENT).
   *
   * A VkImage exposes no queryable extent, so the caller guarantees that each
   * image's dimensions equal physical_width and physical_height: the session
   * builds a framebuffer at that size, and Vulkan leaves a framebuffer larger
   * than its attachment undefined.
   */
  const mln_vulkan_borrowed_texture* textures
    MLN_BINDING("length=texture_count");
  /**
   * Number of images, which is the ring depth, from one to three. A
   * replacement keeps the count the session attached with.
   */
  size_t texture_count;
  /**
   * Backend-native VkFormat value of every image. VK_FORMAT_UNDEFINED is
   * invalid.
   */
  uint32_t format;
  /**
   * Backend-native VkImageLayout value expected at render-pass begin.
   *
   * The session expects an image in this layout each time it renders into
   * it: the first time, and again after the host releases a frame of its
   * slot. A frame's image is in final_layout when the host acquires it, so
   * the host returns it to this layout before it releases the frame.
   * VK_IMAGE_LAYOUT_UNDEFINED accepts an image in any layout and discards its
   * previous contents. Setting it equal to final_layout, or to
   * VK_IMAGE_LAYOUT_UNDEFINED, lets the host release a frame without a layout
   * transition.
   */
  uint32_t initial_layout;
  /**
   * Backend-native VkImageLayout value left after rendering succeeds. Defaults
   * to 5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
   */
  uint32_t final_layout MLN_BINDING("default=5");
} mln_vulkan_borrowed_texture_descriptor;

/** Vulkan frame acquired from a texture ring. */
typedef struct mln_vulkan_texture_frame {
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
  /**
   * Ring slot that holds this frame. For a borrowed target, the index of its
   * image in the descriptor's textures array.
   */
  uint32_t slot;
  /** Borrowed VkImage bit pattern. Valid until frame release. */
  mln_vulkan_non_dispatchable_handle image;
  /** Borrowed VkImageView bit pattern. Valid until frame release. */
  mln_vulkan_non_dispatchable_handle image_view;
  /** Borrowed VkDevice. Valid until frame release. */
  void* device;
  /** Backend-native VkFormat value. */
  uint32_t format;
  /**
   * Backend-native VkImageLayout value that the image is in:
   * VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for a session-owned ring, and the
   * descriptor's final_layout for a borrowed one.
   */
  uint32_t layout;
} mln_vulkan_texture_frame;

/** OpenGL attachment options for an owned texture target. */
typedef struct mln_opengl_owned_texture_descriptor {
  uint32_t size;
  /**
   * Logical texture extent. A scale_factor that differs from the map's is
   * accepted and logged as a warning.
   */
  mln_logical_extent extent;
  /**
   * Borrowed OpenGL context provider data. Shared ownership creates a context
   * whose texture frames the host can acquire. Dedicated EGL or transferred
   * WebGL ownership creates a private core-worker context for CPU readback.
   */
  mln_opengl_context_descriptor context;
} mln_opengl_owned_texture_descriptor;

/** One caller-owned OpenGL texture of a borrowed texture ring. */
typedef struct mln_opengl_borrowed_texture {
  /** Borrowed OpenGL texture object name. Required. */
  uint32_t texture;
} mln_opengl_borrowed_texture;

/** OpenGL attachment options for a borrowed texture target. */
typedef struct mln_opengl_borrowed_texture_descriptor {
  uint32_t size;
  /**
   * Logical texture extent. The map viewport uses width and height and the
   * renderer uses scale_factor; the physical size is stated separately below.
   * A scale_factor that differs from the map's is accepted and logged as a
   * warning.
   */
  mln_logical_extent extent;
  /** Physical texture width in device pixels. Must be positive. Defaults to
   * 256. */
  uint32_t physical_width MLN_BINDING("default=256");
  /** Physical texture height in device pixels. Must be positive. Defaults to
   * 256. */
  uint32_t physical_height MLN_BINDING("default=256");
  /**
   * Borrowed OpenGL context provider data. The textures must belong to this
   * context or a context in the same share group.
   */
  mln_opengl_context_descriptor context;
  /**
   * The ring's textures, one per slot, in slot order. Required.
   *
   * The call copies the array before it returns. Only the textures it names
   * must stay valid: until set_target replaces them, until detach completes,
   * or until abandon returns. The textures are distinct.
   *
   * Querying texture dimensions needs glGetTexLevelParameteriv, absent before
   * OpenGL ES 3.1, so the caller guarantees that each texture's level-0
   * dimensions equal physical_width and physical_height: the session renders
   * through a framebuffer at that size, and a smaller texture clips or garbles
   * output.
   */
  const mln_opengl_borrowed_texture* textures
    MLN_BINDING("length=texture_count");
  /**
   * Number of textures, which is the ring depth, from one to three. A
   * replacement keeps the count the session attached with.
   */
  size_t texture_count;
  /** OpenGL texture target of every texture. Must be GL_TEXTURE_2D. */
  uint32_t target;
} mln_opengl_borrowed_texture_descriptor;

/** WebGPU attachment options for an owned texture target. */
typedef struct mln_webgpu_owned_texture_descriptor {
  uint32_t size;
  /**
   * Logical texture extent. A scale_factor that differs from the map's is
   * accepted and logged as a warning.
   */
  mln_logical_extent extent;
  /** Borrowed WebGPU context. device is required. */
  mln_webgpu_context_descriptor context;
} mln_webgpu_owned_texture_descriptor;

/** One caller-owned WebGPU texture of a borrowed texture ring. */
typedef struct mln_webgpu_borrowed_texture {
  /**
   * Borrowed WGPUTexture. Required.
   *
   * The texture must be created by the descriptor's context.device. It must be
   * 2D, single-sample, and render-attachment capable, and its physical
   * dimensions and format must match the descriptor. Include TextureBinding
   * usage when the host will sample from the texture after rendering.
   */
  void* texture;
  /**
   * Borrowed WGPUTextureView for texture. Required. The view must be a 2D
   * color view compatible with texture and the descriptor's format.
   */
  void* texture_view;
} mln_webgpu_borrowed_texture;

/** WebGPU attachment options for a borrowed texture target. */
typedef struct mln_webgpu_borrowed_texture_descriptor {
  uint32_t size;
  /**
   * Logical texture extent. The map viewport uses width and height and the
   * renderer uses scale_factor; the physical size is stated separately below.
   * A scale_factor that differs from the map's is accepted and logged as a
   * warning.
   */
  mln_logical_extent extent;
  /** Physical texture width in device pixels. Defaults to 256. */
  uint32_t physical_width MLN_BINDING("default=256");
  /** Physical texture height in device pixels. Defaults to 256. */
  uint32_t physical_height MLN_BINDING("default=256");
  /**
   * Borrowed WebGPU context. device is required. Rendering is submitted
   * through context.queue or that device's default queue.
   */
  mln_webgpu_context_descriptor context;
  /**
   * The ring's textures, one per slot, in slot order. Required.
   *
   * The call copies the array before it returns. Only the textures it names
   * must stay valid: until set_target replaces them, until detach completes,
   * or until abandon returns. The textures are distinct.
   */
  const mln_webgpu_borrowed_texture* textures
    MLN_BINDING("length=texture_count");
  /**
   * Number of textures, which is the ring depth, from one to three. A
   * replacement keeps the count the session attached with.
   */
  size_t texture_count;
  /**
   * Backend-native WGPUTextureFormat value of every texture. Undefined is
   * invalid.
   */
  uint32_t format;
} mln_webgpu_borrowed_texture_descriptor;

/** WebGPU frame acquired from a texture ring. */
typedef struct mln_webgpu_texture_frame {
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
  /**
   * Ring slot that holds this frame. For a borrowed target, the index of its
   * texture in the descriptor's textures array.
   */
  uint32_t slot;
  /** Borrowed WGPUTexture. Valid until frame release. */
  void* texture;
  /** Borrowed WGPUTextureView. Valid until frame release. */
  void* texture_view;
  /** Borrowed WGPUDevice. Valid until frame release. */
  void* device;
  /** Backend-native WGPUTextureFormat value. */
  uint32_t format;
} mln_webgpu_texture_frame;

/** OpenGL frame acquired from a texture ring. */
typedef struct mln_opengl_texture_frame {
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
  /**
   * Ring slot that holds this frame. For a borrowed target, the index of its
   * texture in the descriptor's textures array.
   */
  uint32_t slot;
  /**
   * Borrowed OpenGL texture object name. Valid until frame release.
   *
   * The session's context wrote the texture. A host that samples it from
   * another context of the share group binds it again after acquisition,
   * because OpenGL makes another context's completed writes visible only
   * through a new bind.
   */
  uint32_t texture;
  /** OpenGL texture target. GL_TEXTURE_2D is the expected target. */
  uint32_t target;
  /**
   * OpenGL internal format, such as GL_RGBA8. Zero for a borrowed texture,
   * whose format the host chose.
   */
  uint32_t internal_format;
  /**
   * OpenGL pixel format, such as GL_RGBA. Zero for a borrowed texture.
   */
  uint32_t format;
  /**
   * OpenGL pixel type, such as GL_UNSIGNED_BYTE. Zero for a borrowed texture.
   */
  uint32_t type;
} mln_opengl_texture_frame;

/** CPU image readback metadata for a texture target frame. */
typedef struct mln_texture_image_info {
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
  /** Borrowed pixel bytes, valid only during the callback. */
  mln_buffer_view data MLN_BINDING("encoding=bytes");
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
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; descriptor->extent has a zero width or height, a scale_factor that
 *   is not finite and positive, or a scaled dimension past UINT32_MAX;
 *   out_session is null or does not point to the null handle; or the
 *   requested driver kind is unknown, or options carry a malformed wake or
 *   queue lock.
 * - MLN_STATUS_INVALID_STATE when map has been released.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Metal backend, or
 *   options enable a queue lock.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_attach_metal_owned_texture(
  mln_map map, const mln_metal_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts attachment of a ring of caller-owned Metal textures.
 *
 * The session renders into the descriptor's textures as a ring whose depth is
 * their count, and grants frame acquisition and consumer synchronization, but
 * not readback. It never renders into a texture whose frame is acquired.
 * Without acquisition, the session may render into any slot that no acquired
 * frame holds whenever it runs a demand. A host that samples a texture across
 * demands on a core worker acquires its frame first. The common options select
 * driver placement; their requested ring depth is ignored. The descriptor, its
 * textures array, and the options are copied before return.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; textures is null, texture_count is zero or above three, or two
 *   entries name the same texture; the textures differ in device or pixel
 *   format, or one does not match the physical size, render-target usage, or
 *   sample count; descriptor->extent has a zero width or height, or a
 *   scale_factor that is not finite and positive; the stated physical size is
 *   zero; out_session is null or does not point to the null handle; or the
 *   requested driver kind is unknown, or options carry a malformed wake or
 *   queue lock.
 * - MLN_STATUS_INVALID_STATE when map has been released.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Metal backend, or
 *   options enable a queue lock.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_attach_metal_borrowed_texture(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
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
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; descriptor->extent has a zero width or height, a scale_factor that
 *   is not finite and positive, or a scaled dimension past UINT32_MAX;
 *   out_session is null or does not point to the null handle; or the
 *   requested driver kind is unknown, or options carry a malformed wake or
 *   queue lock.
 * - MLN_STATUS_INVALID_STATE when map has been released.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Vulkan backend.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_INVALID_ARGUMENT when the driver finds the context
 *   inconsistent, such as a physical device of another instance or a
 *   graphics_queue_family_index that names no graphics queue family.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_attach_vulkan_owned_texture(
  mln_map map, const mln_vulkan_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts attachment of a ring of caller-owned Vulkan images.
 *
 * The session renders into the descriptor's images as a ring whose depth is
 * their count, and grants frame acquisition and consumer synchronization, but
 * not readback. It never renders into an image whose frame is acquired.
 * Without acquisition, the session may render into any slot that no acquired
 * frame holds whenever it runs a demand. A host that samples an image across
 * demands on a core worker acquires its frame first. The common options select
 * driver placement; their requested ring depth is ignored. The descriptor, its
 * textures array, and the options are copied before return.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; textures is null, texture_count is zero or above three, or two
 *   entries name the same texture; descriptor->extent has a zero width or
 *   height, or a scale_factor that is not finite and positive; the stated
 *   physical size is zero; out_session is null or does not point to the null
 *   handle; or the requested driver kind is unknown, or options carry a
 *   malformed wake or queue lock.
 * - MLN_STATUS_INVALID_STATE when map has been released.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Vulkan backend.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_INVALID_ARGUMENT when the driver finds the context
 *   inconsistent, such as a physical device of another instance or a
 *   graphics_queue_family_index that names no graphics queue family.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_attach_vulkan_borrowed_texture(
  mln_map map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
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
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; descriptor->extent has a zero width or height, a scale_factor that
 *   is not finite and positive, or a scaled dimension past UINT32_MAX;
 *   out_session is null or does not point to the null handle; or the
 *   requested driver kind is unknown, or options carry a malformed wake or
 *   queue lock.
 * - MLN_STATUS_INVALID_STATE when map has been released.
 * - MLN_STATUS_UNSUPPORTED when this build carries no OpenGL backend, its
 *   context provider is unavailable; the requested driver does not match the
 *   context placement; or options enable a queue lock.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_attach_opengl_owned_texture(
  mln_map map, const mln_opengl_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts attachment of a ring of caller-owned OpenGL textures.
 *
 * The session renders into the descriptor's GL_TEXTURE_2D textures as a ring
 * whose depth is their count, and grants frame acquisition and consumer
 * synchronization, but not readback. It never renders into a texture whose
 * frame is acquired. Without acquisition, the session may render into any
 * slot that no acquired frame holds whenever it runs a demand. A host that
 * samples a texture across demands on a core worker acquires its frame first.
 * The common options select driver placement; their requested ring depth is
 * ignored. The descriptor, its textures array, and the options are copied
 * before return.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; textures is null, texture_count is zero or above three, or two
 *   entries name the same texture; target is not GL_TEXTURE_2D;
 *   descriptor->extent has a zero width or height, or a scale_factor that is
 *   not finite and positive; the stated physical size is zero; out_session is
 *   null or does not point to the null handle; or the requested driver kind is
 *   unknown, or options carry a malformed wake or queue lock.
 * - MLN_STATUS_INVALID_STATE when map has been released.
 * - MLN_STATUS_UNSUPPORTED when this build carries no OpenGL backend, its
 *   context provider is unavailable; the requested driver is not
 *   MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD; or options enable a queue lock.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_attach_opengl_borrowed_texture(
  mln_map map, const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
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
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; descriptor->extent has a zero width or height, a scale_factor that
 *   is not finite and positive, or a scaled dimension past UINT32_MAX;
 *   out_session is null or does not point to the null handle; or the
 *   requested driver kind is unknown, or options carry a malformed wake or
 *   queue lock.
 * - MLN_STATUS_INVALID_STATE when map has been released.
 * - MLN_STATUS_UNSUPPORTED when this build carries no WebGPU backend, or the
 *   requested driver is not MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, or
 *   options enable a queue lock.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_attach_webgpu_owned_texture(
  mln_map map, const mln_webgpu_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts attachment of a ring of caller-owned WebGPU textures.
 *
 * The session renders into the descriptor's textures as a ring whose depth is
 * their count, and grants frame acquisition and consumer synchronization, but
 * not readback. It never renders into a texture whose frame is acquired.
 * Without acquisition, the session may render into any slot that no acquired
 * frame holds whenever it runs a demand. A host that samples a texture across
 * demands on a core worker acquires its frame first. Browser targets require
 * MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, and the requested ring depth of the
 * common options is ignored. The descriptor, its textures array, and the
 * options are copied before return.
 *
 * *out_session must be MLN_HANDLE_NULL on entry. MLN_STATUS_OK publishes an
 * ATTACHING session there and transfers it to the caller. A non-OK return
 * leaves *out_session unchanged and never invokes the completion. A failed
 * completion still requires mln_render_session_detach() or
 * mln_render_session_abandon() before mln_render_session_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle; descriptor,
 *   options, or completion is null or undersized; a required backend handle is
 *   null; textures is null, texture_count is zero or above three, or two
 *   entries name the same texture; descriptor->extent has a zero width or
 *   height, or a scale_factor that is not finite and positive; the stated
 *   physical size is zero; out_session is null or does not point to the null
 *   handle; or the requested driver kind is unknown, or options carry a
 *   malformed wake or queue lock.
 * - MLN_STATUS_INVALID_STATE when map has been released.
 * - MLN_STATUS_UNSUPPORTED when this build carries no WebGPU backend, or the
 *   requested driver is not MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, or
 *   options enable a queue lock.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_attach_webgpu_borrowed_texture(
  mln_map map, const mln_webgpu_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered replacement of every texture of a caller-owned Metal ring.
 *
 * The descriptor and its textures array are copied before return. The
 * replacement names as many textures as the session attached with, each
 * belonging to the device this session attached with.
 *
 * The replacement runs in order with the session's other driver work. Frames
 * published before the replacement can no longer be acquired: acquisition
 * reports MLN_STATUS_NOT_READY from this call's return until the replacement
 * has run. The replacement also returns to service every slot whose frame was
 * disposed. The completion means that the session no longer renders into or
 * reads the replaced textures.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle; descriptor
 *   or completion is null or undersized; a required backend handle is null;
 *   textures is null, texture_count is zero, above three, or different from the
 *   session's ring depth, or two entries name the same texture;
 *   descriptor->extent has a zero width or height, or a scale_factor that is
 *   not finite and positive; or the stated physical size is zero.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached,
 *   or while a frame of the ring is acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Metal backend, or the
 *   session does not render into a caller-owned texture.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders into the new textures.
 * - MLN_STATUS_INVALID_ARGUMENT when the replacement belongs to another device.
 * - MLN_STATUS_UNSUPPORTED when the replacement does not have the pixel format
 *   this session compiled its pipeline states for.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_set_metal_borrowed_texture_target(
  mln_render_session session,
  const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered replacement of every image of a caller-owned Vulkan ring.
 *
 * The descriptor and its textures array are copied before return. The
 * replacement names as many images as the session attached with, and the
 * context this session attached with.
 *
 * The replacement runs in order with the session's other driver work. Frames
 * published before the replacement can no longer be acquired: acquisition
 * reports MLN_STATUS_NOT_READY from this call's return until the replacement
 * has run. The replacement also returns to service every slot whose frame was
 * disposed. The completion means that the session no longer renders into or
 * reads the replaced textures.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle; descriptor
 *   or completion is null or undersized; a required backend handle is null;
 *   textures is null, texture_count is zero, above three, or different from the
 *   session's ring depth, or two entries name the same texture;
 *   descriptor->extent has a zero width or height, or a scale_factor that is
 *   not finite and positive; or the stated physical size is zero.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached,
 *   or while a frame of the ring is acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no Vulkan backend, or the
 *   session does not render into a caller-owned texture.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders into the new textures.
 * - MLN_STATUS_INVALID_ARGUMENT when the replacement names another context.
 * - MLN_STATUS_UNSUPPORTED when the replacement does not have the format and
 *   layouts this session built its render pass for.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_set_vulkan_borrowed_texture_target(
  mln_render_session session,
  const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered replacement of every texture of a caller-owned OpenGL
 * ring.
 *
 * The descriptor and its textures array are copied before return. The
 * replacement names as many GL_TEXTURE_2D textures as the session attached
 * with, in the context this session attached with.
 *
 * The replacement runs in order with the session's other driver work. Frames
 * published before the replacement can no longer be acquired: acquisition
 * reports MLN_STATUS_NOT_READY from this call's return until the replacement
 * has run. The replacement also returns to service every slot whose frame was
 * disposed. The completion means that the session no longer renders into or
 * reads the replaced textures.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle; descriptor
 *   or completion is null or undersized; a required backend handle is null;
 *   textures is null, texture_count is zero, above three, or different from the
 *   session's ring depth, or two entries name the same texture;
 *   descriptor->extent has a zero width or height, or a scale_factor that is
 *   not finite and positive; or the stated physical size is zero.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached,
 *   or while a frame of the ring is acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no OpenGL backend, or the
 *   session does not render into a caller-owned texture.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders into the new textures.
 * - MLN_STATUS_INVALID_ARGUMENT when the replacement names another context.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_set_opengl_borrowed_texture_target(
  mln_render_session session,
  const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered replacement of every texture of a caller-owned WebGPU
 * ring.
 *
 * The descriptor and its textures array are copied before return. The
 * replacement names as many textures as the session attached with, and the
 * device and queue this session attached with.
 *
 * The replacement runs in order with the session's other driver work. Frames
 * published before the replacement can no longer be acquired: acquisition
 * reports MLN_STATUS_NOT_READY from this call's return until the replacement
 * has run. The replacement also returns to service every slot whose frame was
 * disposed. The completion means that the session no longer renders into or
 * reads the replaced textures.
 *
 * Returns:
 * - MLN_STATUS_OK when the replacement is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle; descriptor
 *   or completion is null or undersized; a required backend handle is null;
 *   textures is null, texture_count is zero, above three, or different from the
 *   session's ring depth, or two entries name the same texture;
 *   descriptor->extent has a zero width or height, or a scale_factor that is
 *   not finite and positive; or the stated physical size is zero.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached,
 *   or while a frame of the ring is acquired.
 * - MLN_STATUS_UNSUPPORTED when this build carries no WebGPU backend, or the
 *   session does not render into a caller-owned texture.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver renders into the new textures.
 * - MLN_STATUS_INVALID_ARGUMENT when the replacement names another device or
 *   queue.
 * - MLN_STATUS_UNSUPPORTED when the replacement does not have the format this
 *   session built its render pipelines for.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_set_webgpu_borrowed_texture_target(
  mln_render_session session,
  const mln_webgpu_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads back the latest frame of the session's owned texture as premultiplied
 * RGBA8.
 *
 * The completion delivers one mln_texture_readback_result as its value, with a
 * value_count of one. Its pixel bytes are borrowed for the duration of the
 * callback; copy anything the host keeps.
 *
 * Returns:
 * - MLN_STATUS_OK when the readback is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, or
 *   completion is null or undersized.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached.
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
MLN_BINDING("execution=query;result=mln_texture_readback_result")
MLN_API mln_status mln_render_session_read_texture(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies Metal-native metadata from an acquired frame.
 *
 * The texture and device pointers are borrowed and remain valid only until
 * mln_acquired_frame_release(). A host that may dispose the frame or its
 * session from another thread uses them inside an
 * mln_acquired_frame_view_begin() scope.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is an invalid handle, or out_frame
 *   is null or undersized.
 * - MLN_STATUS_INVALID_STATE when frame has been released.
 * - MLN_STATUS_UNSUPPORTED when the frame was produced by a different render
 *   backend.
 * - MLN_STATUS_TARGET_LOST when the session lost or abandoned its target.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_BINDING("view_owner=frame")
MLN_API mln_status mln_acquired_frame_get_metal_texture(
  mln_acquired_frame frame,
  mln_metal_texture_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies Vulkan-native metadata from an acquired frame.
 *
 * The image and image view handles and the device pointer are borrowed and
 * remain valid only until mln_acquired_frame_release(). A host that may
 * dispose the frame or its session from another thread uses them inside an
 * mln_acquired_frame_view_begin() scope.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is an invalid handle, or out_frame
 *   is null or undersized.
 * - MLN_STATUS_INVALID_STATE when frame has been released.
 * - MLN_STATUS_UNSUPPORTED when the frame was produced by a different render
 *   backend.
 * - MLN_STATUS_TARGET_LOST when the session lost or abandoned its target.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_BINDING("view_owner=frame")
MLN_API mln_status mln_acquired_frame_get_vulkan_texture(
  mln_acquired_frame frame,
  mln_vulkan_texture_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies OpenGL-native metadata from an acquired frame.
 *
 * The caller driver's context must be current on this thread. The texture name
 * is borrowed and remains valid only until mln_acquired_frame_release(). A
 * host that may dispose the frame or its session from another thread uses it
 * inside an mln_acquired_frame_view_begin() scope.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is an invalid handle, or out_frame
 *   is null or undersized.
 * - MLN_STATUS_INVALID_STATE when frame has been released.
 * - MLN_STATUS_UNSUPPORTED when the frame was produced by a different render
 *   backend.
 * - MLN_STATUS_TARGET_LOST when the session lost or abandoned its target.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_BINDING("view_owner=frame")
MLN_API mln_status mln_acquired_frame_get_opengl_texture(
  mln_acquired_frame frame,
  mln_opengl_texture_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies WebGPU-native metadata from an acquired frame.
 *
 * The texture, view, and device pointers are borrowed and remain valid only
 * until mln_acquired_frame_release(). A host that may dispose the frame or its
 * session from another thread uses them inside an
 * mln_acquired_frame_view_begin() scope.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is an invalid handle, or out_frame
 *   is null or undersized.
 * - MLN_STATUS_INVALID_STATE when frame has been released.
 * - MLN_STATUS_UNSUPPORTED when the frame was produced by a different render
 *   backend.
 * - MLN_STATUS_TARGET_LOST when the session lost or abandoned its target.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_BINDING("view_owner=frame")
MLN_API mln_status mln_acquired_frame_get_webgpu_texture(
  mln_acquired_frame frame,
  mln_webgpu_texture_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_TEXTURE_H
