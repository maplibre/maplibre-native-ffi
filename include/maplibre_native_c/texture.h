/**
 * @file maplibre_native_c/texture.h
 * Render a map into a ring of textures that your renderer draws.
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

/** A Metal texture of a borrowed ring. */
typedef struct mln_metal_borrowed_texture {
  /** Your MTLTexture. */
  void* texture MLN_BINDING("required=true");
} mln_metal_borrowed_texture;

/**
 * A ring of your Metal textures, for mln_map_attach_metal_borrowed_texture().
 *
 * `logical_size` sets the map's size. `physical_width` and `physical_height`
 * give the textures' size in pixels, which is usually `logical_size` times the
 * map's scale factor. The textures must all have that size, one device, one
 * pixel format, one sample, and MTLTextureUsageRenderTarget. The ring holds up
 * to MLN_TEXTURE_RING_MAX_DEPTH distinct textures.
 *
 * The session uses the textures until a replacement or release of the target
 * completes, a detach or disposal of the session completes, or an abandon of it
 * returns.
 */
typedef struct mln_metal_borrowed_texture_descriptor {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  uint32_t physical_width MLN_BINDING("required=true");
  uint32_t physical_height MLN_BINDING("required=true");
  const mln_metal_borrowed_texture* textures
    MLN_BINDING("length=texture_count;required=true");
  size_t texture_count;
  /**
   * Your MTLCommandQueue on the textures' device, or null for a queue of the
   * session's own. With your queue, frames can use MLN_GPU_SYNC_QUEUE_ORDER.
   */
  void* command_queue;
} mln_metal_borrowed_texture_descriptor;

/**
 * New Metal textures for a borrowed ring, for
 * mln_render_session_set_metal_borrowed_texture_target().
 *
 * The members mean what they mean in mln_metal_borrowed_texture_descriptor,
 * and the textures follow its rules. The ring keeps its depth, device, and
 * command queue, so give it as many textures as it attached with, on the same
 * device. The session uses the textures until a replacement or release of the
 * target completes, a detach or disposal of the session completes, or an
 * abandon of it returns.
 */
typedef struct mln_metal_borrowed_texture_replacement {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  uint32_t physical_width MLN_BINDING("required=true");
  uint32_t physical_height MLN_BINDING("required=true");
  const mln_metal_borrowed_texture* textures
    MLN_BINDING("length=texture_count;required=true");
  size_t texture_count;
} mln_metal_borrowed_texture_replacement;

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

/** A Vulkan image of a borrowed ring. */
typedef struct mln_vulkan_borrowed_texture {
  /** Your VkImage. */
  mln_vulkan_non_dispatchable_handle image MLN_BINDING("required=true");
  /** A 2D VkImageView of `image` with one mip level and one layer. */
  mln_vulkan_non_dispatchable_handle image_view MLN_BINDING("required=true");
} mln_vulkan_borrowed_texture;

/**
 * The queue family that a borrowed image goes to after each frame.
 *
 * With MLN_VULKAN_IMAGE_OWNER_EXTERNAL or MLN_VULKAN_IMAGE_OWNER_FOREIGN,
 * acquire the image on your side before you read it. You don't need to release
 * it back.
 */
typedef enum mln_vulkan_image_owner : uint32_t {
  /** The image stays with the queue family of `graphics_queue_family_index`. */
  MLN_VULKAN_IMAGE_OWNER_GRAPHICS_QUEUE = 0U,
  /** Each frame releases the image to VK_QUEUE_FAMILY_EXTERNAL. */
  MLN_VULKAN_IMAGE_OWNER_EXTERNAL = 1U,
  /** Each frame releases the image to VK_QUEUE_FAMILY_FOREIGN_EXT. */
  MLN_VULKAN_IMAGE_OWNER_FOREIGN = 2U,
} mln_vulkan_image_owner;

/**
 * A ring of your Vulkan images, for mln_map_attach_vulkan_borrowed_texture().
 *
 * `logical_size` sets the map's size. `physical_width` and `physical_height`
 * give the images' size in pixels, which is usually `logical_size` times the
 * map's scale factor. Each image must have that size, `format`, and
 * VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, and belong to the context's device. The
 * ring holds up to MLN_TEXTURE_RING_MAX_DEPTH distinct images.
 *
 * The session uses the images until a replacement or release of the target
 * completes, a detach or disposal of the session completes, or an abandon of it
 * returns.
 */
typedef struct mln_vulkan_borrowed_texture_descriptor {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  uint32_t physical_width MLN_BINDING("required=true");
  uint32_t physical_height MLN_BINDING("required=true");
  mln_vulkan_context_descriptor context;
  const mln_vulkan_borrowed_texture* textures
    MLN_BINDING("length=texture_count;required=true");
  size_t texture_count;
  /** The images' VkFormat. */
  uint32_t format MLN_BINDING("required=true");
  /** The VkImageLayout that each frame leaves its image in. */
  uint32_t final_layout MLN_BINDING("required=true");
  mln_vulkan_image_owner owner;
} mln_vulkan_borrowed_texture_descriptor;

/**
 * New Vulkan images for a borrowed ring, for
 * mln_render_session_set_vulkan_borrowed_texture_target().
 *
 * The members mean what they mean in mln_vulkan_borrowed_texture_descriptor,
 * and the images follow its rules. The ring keeps its depth, context,
 * `format`, `final_layout`, and `owner`, so give it as many images as it
 * attached with, in that format, on the same device. The session uses the
 * images until a replacement or release of the target completes, a detach or
 * disposal of the session completes, or an abandon of it returns.
 */
typedef struct mln_vulkan_borrowed_texture_replacement {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  uint32_t physical_width MLN_BINDING("required=true");
  uint32_t physical_height MLN_BINDING("required=true");
  const mln_vulkan_borrowed_texture* textures
    MLN_BINDING("length=texture_count;required=true");
  size_t texture_count;
} mln_vulkan_borrowed_texture_replacement;

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

/** An OpenGL texture of a borrowed ring. */
typedef struct mln_open_gl_borrowed_texture {
  /** Your texture name. */
  uint32_t texture MLN_BINDING("required=true;native_object=true");
} mln_open_gl_borrowed_texture;

/**
 * A ring of your OpenGL textures, for
 * mln_map_attach_open_gl_borrowed_texture().
 *
 * `logical_size` sets the map's size. `physical_width` and `physical_height`
 * give the textures' size in pixels, which is usually `logical_size` times the
 * map's scale factor. The context must use
 * MLN_OPEN_GL_CONTEXT_OWNERSHIP_SHARED. Each texture must be in your context's
 * share group, with storage of that size. The ring holds up to
 * MLN_TEXTURE_RING_MAX_DEPTH distinct textures.
 *
 * The session uses the textures until a replacement or release of the target
 * completes, a detach or disposal of the session completes, or an abandon of it
 * returns.
 */
typedef struct mln_open_gl_borrowed_texture_descriptor {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  uint32_t physical_width MLN_BINDING("required=true");
  uint32_t physical_height MLN_BINDING("required=true");
  mln_open_gl_context_descriptor context;
  const mln_open_gl_borrowed_texture* textures
    MLN_BINDING("length=texture_count;required=true");
  size_t texture_count;
  /** The textures' target. It must be GL_TEXTURE_2D. */
  uint32_t target MLN_BINDING("required=true");
} mln_open_gl_borrowed_texture_descriptor;

/**
 * New OpenGL textures for a borrowed ring, for
 * mln_render_session_set_open_gl_borrowed_texture_target().
 *
 * The members mean what they mean in mln_open_gl_borrowed_texture_descriptor,
 * and the textures follow its rules. The ring keeps its depth, context, and
 * texture target, so give it as many textures as it attached with, in the same
 * share group. The session uses the textures until a replacement or release of
 * the target completes, a detach or disposal of the session completes, or an
 * abandon of it returns.
 */
typedef struct mln_open_gl_borrowed_texture_replacement {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  uint32_t physical_width MLN_BINDING("required=true");
  uint32_t physical_height MLN_BINDING("required=true");
  const mln_open_gl_borrowed_texture* textures
    MLN_BINDING("length=texture_count;required=true");
  size_t texture_count;
} mln_open_gl_borrowed_texture_replacement;

/** A WebGPU texture of a borrowed ring. */
typedef struct mln_web_gpu_borrowed_texture {
  /** Your WGPUTexture. */
  void* texture MLN_BINDING("required=true");
  /** A WGPUTextureView of `texture`. */
  void* texture_view MLN_BINDING("required=true");
} mln_web_gpu_borrowed_texture;

/**
 * A ring of your WebGPU textures, for
 * mln_map_attach_web_gpu_borrowed_texture().
 *
 * `logical_size` sets the map's size. `physical_width` and `physical_height`
 * give the textures' size in pixels, which is usually `logical_size` times the
 * map's scale factor. Each texture must be 2D with that size, `format`, one
 * layer, one mip level, one sample, and WGPUTextureUsage_RenderAttachment. The
 * ring holds up to MLN_TEXTURE_RING_MAX_DEPTH distinct textures.
 *
 * The session uses the textures until a replacement or release of the target
 * completes, a detach or disposal of the session completes, or an abandon of it
 * returns.
 */
typedef struct mln_web_gpu_borrowed_texture_descriptor {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  uint32_t physical_width MLN_BINDING("required=true");
  uint32_t physical_height MLN_BINDING("required=true");
  mln_web_gpu_context_descriptor context;
  const mln_web_gpu_borrowed_texture* textures
    MLN_BINDING("length=texture_count;required=true");
  size_t texture_count;
  /** The textures' WGPUTextureFormat. */
  uint32_t format MLN_BINDING("required=true");
} mln_web_gpu_borrowed_texture_descriptor;

/**
 * New WebGPU textures for a borrowed ring, for
 * mln_render_session_set_web_gpu_borrowed_texture_target().
 *
 * The members mean what they mean in mln_web_gpu_borrowed_texture_descriptor,
 * and the textures follow its rules. The ring keeps its depth, context, and
 * `format`, so give it as many textures as it attached with, in that format,
 * on the same device. The session uses the textures until a replacement or
 * release of the target completes, a detach or disposal of the session
 * completes, or an abandon of it returns.
 */
typedef struct mln_web_gpu_borrowed_texture_replacement {
  uint32_t size;
  mln_logical_size logical_size MLN_BINDING("required=true");
  uint32_t physical_width MLN_BINDING("required=true");
  uint32_t physical_height MLN_BINDING("required=true");
  const mln_web_gpu_borrowed_texture* textures
    MLN_BINDING("length=texture_count;required=true");
  size_t texture_count;
} mln_web_gpu_borrowed_texture_replacement;

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
 * Creates a session that renders the map into a ring of your Metal textures.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when mln_supported_render_backend_mask() lacks
 *   MLN_RENDER_BACKEND_FLAG_METAL, or when the target supports none of the
 *   `producer_sync_kinds` in `options`.
 */
MLN_BINDING("execution=lifecycle;platforms=ios,macos,tvos,visionos")
MLN_API mln_status mln_map_attach_metal_borrowed_texture(
  mln_map map, const mln_metal_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Creates a session that renders the map into a ring of your Vulkan images.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when mln_supported_render_backend_mask() lacks
 *   MLN_RENDER_BACKEND_FLAG_VULKAN, or when the target supports none of the
 *   `producer_sync_kinds` in `options`.
 */
MLN_BINDING(
  "execution=lifecycle;platforms=android,freebsd,linux,macos,ohos,windows"
)
MLN_API mln_status mln_map_attach_vulkan_borrowed_texture(
  mln_map map, const mln_vulkan_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Creates a session that renders the map into a ring of your OpenGL textures.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when mln_open_gl_supported_context_provider_mask()
 *   lacks the context's platform, or when the target supports none of the
 *   `producer_sync_kinds` in `options`.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_attach_open_gl_borrowed_texture(
  mln_map map, const mln_open_gl_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Creates a session that renders the map into a ring of your WebGPU textures.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when mln_supported_render_backend_mask() lacks
 *   MLN_RENDER_BACKEND_FLAG_WEB_GPU, or when the target supports none of the
 *   `producer_sync_kinds` in `options`.
 */
MLN_BINDING("execution=lifecycle;platforms=web")
MLN_API mln_status mln_map_attach_web_gpu_borrowed_texture(
  mln_map map, const mln_web_gpu_borrowed_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the Metal textures that a borrowed ring renders into.
 *
 * Use it to resize the ring, or to give the session textures again after
 * mln_render_session_release_target(). Release every acquired frame first.
 * Once the replacement completes, the session no longer uses the old textures.
 * A replacement with the textures and sizes that the ring renders into already
 * changes nothing.
 *
 * Completes with:
 * - MLN_STATUS_UNSUPPORTED when the new textures have a different pixel format
 *   than the old ones. Attach a new session to change it.
 */
MLN_BINDING("execution=operation;platforms=ios,macos,tvos,visionos")
MLN_API mln_status mln_render_session_set_metal_borrowed_texture_target(
  mln_render_session session,
  const mln_metal_borrowed_texture_replacement* replacement,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the Vulkan images that a borrowed ring renders into.
 *
 * Use it to resize the ring, or to give the session images again after
 * mln_render_session_release_target(). Release every acquired frame first.
 * Once the replacement completes, the session no longer uses the old images.
 * A replacement with the images and sizes that the ring renders into already
 * changes nothing.
 */
MLN_BINDING(
  "execution=operation;platforms=android,freebsd,linux,macos,ohos,windows"
)
MLN_API mln_status mln_render_session_set_vulkan_borrowed_texture_target(
  mln_render_session session,
  const mln_vulkan_borrowed_texture_replacement* replacement,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the OpenGL textures that a borrowed ring renders into.
 *
 * Use it to resize the ring, or to give the session textures again after
 * mln_render_session_release_target(). Release every acquired frame first.
 * Once the replacement completes, the session no longer uses the old textures.
 * A replacement with the textures and sizes that the ring renders into already
 * changes nothing.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_set_open_gl_borrowed_texture_target(
  mln_render_session session,
  const mln_open_gl_borrowed_texture_replacement* replacement,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the WebGPU textures that a borrowed ring renders into.
 *
 * Use it to resize the ring, or to give the session textures again after
 * mln_render_session_release_target(). Release every acquired frame first.
 * Once the replacement completes, the session no longer uses the old textures.
 * A replacement with the textures and sizes that the ring renders into already
 * changes nothing.
 */
MLN_BINDING("execution=operation;platforms=web")
MLN_API mln_status mln_render_session_set_web_gpu_borrowed_texture_target(
  mln_render_session session,
  const mln_web_gpu_borrowed_texture_replacement* replacement,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

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
