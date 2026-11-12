/**
 * @file maplibre_native_c/texture.h
 * The textures of acquired frames, borrowed and exported, and reading a
 * session's frames back.
 */

#ifndef MAPLIBRE_NATIVE_C_TEXTURE_H
#define MAPLIBRE_NATIVE_C_TEXTURE_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

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

/** The IOSurface of an exported ring's acquired frame. */
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

/** The dmabuf of an exported ring's acquired frame. */
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

/** The Android hardware buffer of an exported ring's acquired frame. */
typedef struct mln_android_hardware_buffer_frame {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  double scale_factor;
  /** The buffer's index in the ring. */
  uint32_t slot;
  /** The `AHardwareBuffer*`. */
  void* buffer MLN_BINDING("lifetime=owner");
} mln_android_hardware_buffer_frame;

/** The DXGI shared texture of an exported ring's acquired frame. */
typedef struct mln_dxgi_shared_texture_frame {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  double scale_factor;
  /** The texture's index in the ring. */
  uint32_t slot;
  /** The texture's `DXGI_FORMAT`. */
  uint32_t format;
  /** The texture's shared NT `HANDLE`. */
  void* handle MLN_BINDING("lifetime=owner");
} mln_dxgi_shared_texture_frame;

/**
 * The memory of an exported ring's acquired frame, as an opaque file
 * descriptor.
 *
 * The memory holds a 2D Vulkan image of `format`, with one mip level, one
 * layer, optimal tiling, and the usage VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
 * VK_IMAGE_USAGE_SAMPLED_BIT, and VK_IMAGE_USAGE_TRANSFER_SRC_BIT, which a
 * Vulkan image that imports it has too. In OpenGL, import it with
 * glImportMemoryFdEXT() and GL_HANDLE_TYPE_OPAQUE_FD_EXT, and make the texture
 * with GL_TEXTURE_TILING_EXT set to GL_OPTIMAL_TILING_EXT and
 * glTexStorageMem2DEXT() at offset 0. Each frame leaves the image in
 * VK_IMAGE_LAYOUT_GENERAL, released to VK_QUEUE_FAMILY_EXTERNAL.
 */
typedef struct mln_opaque_fd_frame {
  uint32_t size;
  uint32_t width;
  uint32_t height;
  double scale_factor;
  /** The texture's index in the ring. */
  uint32_t slot;
  /** The file descriptor of the image's memory. */
  int32_t fd MLN_BINDING("lifetime=owner");
  /** The size of the memory in bytes. */
  uint64_t allocation_size;
  /**
   * Whether the memory is the image's own, which an import says, such as with
   * GL_DEDICATED_MEMORY_OBJECT_EXT.
   */
  bool dedicated;
  /** The image's `VkFormat`: VK_FORMAT_R8G8B8A8_UNORM, OpenGL's GL_RGBA8. */
  uint32_t format;
} mln_opaque_fd_frame;

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

/**
 * Gets the IOSurface of an exported ring's acquired frame.
 *
 * Wait for mln_acquired_frame_get_producer_sync() before you read the surface.
 * Once you release or discard the frame, the session may render into the
 * surface again. For an API that takes a `CVPixelBuffer`, wrap the surface
 * with `CVPixelBufferCreateWithIOSurface`. A reference that you take with
 * `CFRetain`, or a pixel buffer that wraps the surface, keeps the surface's
 * memory, but not its contents.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when the session's ring shares its textures in
 *   another way.
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

/**
 * Gets the dmabuf of an exported ring's acquired frame.
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
 * - MLN_STATUS_UNSUPPORTED when the session's ring shares its textures in
 *   another way.
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

/**
 * Gets the Android hardware buffer of an exported ring's acquired frame.
 *
 * Wait for mln_acquired_frame_get_producer_sync() before you read the buffer.
 * Once you release or discard the frame, the session may render into the
 * buffer again. A reference that you take with `AHardwareBuffer_acquire` keeps
 * the buffer's memory, but not its contents.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when the session's ring shares its textures in
 *   another way.
 * - MLN_STATUS_TARGET_LOST when the session failed, or you detached, abandoned,
 *   or disposed it.
 *
 * Lifetimes:
 * The buffer in `out_frame` stays valid until you release or discard the
 * frame, or the session is abandoned or disposed. A view from
 * mln_acquired_frame_view_begin() keeps it valid until
 * mln_acquired_frame_view_end().
 */
MLN_BINDING("view_owner=frame;platforms=android")
MLN_API mln_status mln_acquired_frame_get_android_hardware_buffer(
  mln_acquired_frame frame,
  mln_android_hardware_buffer_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gets the DXGI shared texture of an exported ring's acquired frame.
 *
 * Wait for mln_acquired_frame_get_producer_sync() before you read the texture.
 * Once you release or discard the frame, the session may render into the
 * texture again. Open the handle with `ID3D11Device1::OpenSharedResource1` or
 * `ID3D12Device::OpenSharedHandle`. A texture that you open keeps its memory,
 * but not its contents.
 *
 * The session owns the handle, so don't close it. To keep the handle after you
 * release the frame, duplicate it with `DuplicateHandle`.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when the session's ring shares its textures in
 *   another way.
 * - MLN_STATUS_TARGET_LOST when the session failed, or you detached, abandoned,
 *   or disposed it.
 *
 * Lifetimes:
 * The handle in `out_frame` stays valid until you release or discard the
 * frame, or the session is abandoned or disposed. A view from
 * mln_acquired_frame_view_begin() keeps it valid until
 * mln_acquired_frame_view_end().
 */
MLN_BINDING("view_owner=frame;platforms=windows")
MLN_API mln_status mln_acquired_frame_get_dxgi_shared_texture(
  mln_acquired_frame frame,
  mln_dxgi_shared_texture_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gets the memory of an exported ring's acquired frame, as an opaque file
 * descriptor.
 *
 * Wait for mln_acquired_frame_get_producer_sync() before you read the image.
 * Once you release or discard the frame, the session may render into the
 * image again.
 *
 * The session owns the file descriptor, so don't close it. An import takes the
 * descriptor, so import a duplicate from `dup`. Memory that you import keeps
 * the image's memory, but not its contents.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when the session's ring shares its textures in
 *   another way.
 * - MLN_STATUS_TARGET_LOST when the session failed, or you detached, abandoned,
 *   or disposed it.
 *
 * Lifetimes:
 * The file descriptor in `out_frame` stays valid until you release or discard
 * the frame, or the session is abandoned or disposed. A view from
 * mln_acquired_frame_view_begin() keeps it valid until
 * mln_acquired_frame_view_end().
 */
MLN_BINDING("view_owner=frame;platforms=android,freebsd,linux,ohos")
MLN_API mln_status mln_acquired_frame_get_opaque_fd(
  mln_acquired_frame frame,
  mln_opaque_fd_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_TEXTURE_H
