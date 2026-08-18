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
 */
MLN_API mln_status mln_metal_owned_texture_attach(
  mln_map map, const mln_metal_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts attachment of a caller-owned Metal texture target. */
MLN_API mln_status mln_metal_borrowed_texture_attach(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts attachment of a session-owned Vulkan texture ring. */
MLN_API mln_status mln_vulkan_owned_texture_attach(
  mln_map map, const mln_vulkan_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts attachment of a caller-owned Vulkan texture target. */
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
 */
MLN_API mln_status mln_opengl_owned_texture_attach(
  mln_map map, const mln_opengl_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts attachment of a caller-owned OpenGL texture target. */
MLN_API mln_status mln_opengl_borrowed_texture_attach(
  mln_map map, const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts attachment of a session-owned WebGPU texture ring.
 *
 * Browser targets require MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD.
 */
MLN_API mln_status mln_webgpu_owned_texture_attach(
  mln_map map, const mln_webgpu_owned_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts attachment of a caller-owned WebGPU texture target. */
MLN_API mln_status mln_webgpu_borrowed_texture_attach(
  mln_map map, const mln_webgpu_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts an ordered caller-owned Metal texture replacement. */
MLN_API mln_status mln_metal_borrowed_texture_set_target(
  mln_render_session session,
  const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts an ordered caller-owned Vulkan texture replacement. */
MLN_API mln_status mln_vulkan_borrowed_texture_set_target(
  mln_render_session session,
  const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts an ordered caller-owned OpenGL texture replacement. */
MLN_API mln_status mln_opengl_borrowed_texture_set_target(
  mln_render_session session,
  const mln_opengl_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts an ordered caller-owned WebGPU texture replacement. */
MLN_API mln_status mln_webgpu_borrowed_texture_set_target(
  mln_render_session session,
  const mln_webgpu_borrowed_texture_descriptor* descriptor,
  const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts readback of the latest rendered texture frame. */
MLN_API mln_status mln_texture_read_premultiplied_rgba8(
  mln_render_session session, const mln_completion* completion
) MLN_NOEXCEPT;

/** Copies Metal-native metadata from an acquired frame. */
MLN_API mln_status mln_acquired_frame_get_metal_texture(
  mln_acquired_frame frame, mln_metal_owned_texture_frame* out_frame
) MLN_NOEXCEPT;

/** Copies Vulkan-native metadata from an acquired frame. */
MLN_API mln_status mln_acquired_frame_get_vulkan_texture(
  mln_acquired_frame frame, mln_vulkan_owned_texture_frame* out_frame
) MLN_NOEXCEPT;

/**
 * Copies OpenGL-native metadata from an acquired frame.
 *
 * The caller driver's context must be current on this thread.
 */
MLN_API mln_status mln_acquired_frame_get_opengl_texture(
  mln_acquired_frame frame, mln_opengl_owned_texture_frame* out_frame
) MLN_NOEXCEPT;

/** Copies WebGPU-native metadata from an acquired frame. */
MLN_API mln_status mln_acquired_frame_get_webgpu_texture(
  mln_acquired_frame frame, mln_webgpu_owned_texture_frame* out_frame
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_TEXTURE_H
