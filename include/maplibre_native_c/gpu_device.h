/**
 * @file maplibre_native_c/gpu_device.h
 * The GPU devices that render sessions render on.
 */

#ifndef MAPLIBRE_NATIVE_C_GPU_DEVICE_H
#define MAPLIBRE_NATIVE_C_GPU_DEVICE_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stdint.h>

#include "base.h"
#include "render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The fields that mln_gpu_device_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_gpu_device_option_field : uint32_t {
  MLN_GPU_DEVICE_OPTION_BACKEND = 1U << 0U,
  MLN_GPU_DEVICE_OPTION_ADAPTER_INDEX = 1U << 1U,
  MLN_GPU_DEVICE_OPTION_ADAPTER_UUID = 1U << 2U,
  MLN_GPU_DEVICE_OPTION_ADAPTER_LUID = 1U << 3U,
  MLN_GPU_DEVICE_OPTION_OPEN_GL_CLIENT_API = 1U << 4U,
  MLN_GPU_DEVICE_OPTION_OPEN_GL_SINGLE_CONTEXT = 1U << 5U,
} mln_gpu_device_option_field;

/** A set of mln_gpu_device_option_field bits. */
typedef uint32_t mln_gpu_device_option_fields;

/**
 * Options for mln_gpu_device_create().
 *
 * Name the GPU by at most one of `adapter_index`, `adapter_uuid`, and
 * `adapter_luid`. Without one, the device prefers a discrete GPU.
 */
typedef struct mln_gpu_device_options {
  uint32_t size;
  mln_gpu_device_option_fields fields;
  /**
   * The backend that the device renders with. By default, the platform's own:
   * Metal on Apple platforms, Vulkan elsewhere, or OpenGL where the system has
   * no Vulkan driver, and WebGPU in a browser, or WebGL where the browser has
   * no WebGPU.
   */
  mln_render_backend backend
    MLN_BINDING("mask=fields;bit=MLN_GPU_DEVICE_OPTION_BACKEND");
  /** The GPU to use, as an index into the backend's list of GPUs. */
  uint32_t adapter_index
    MLN_BINDING("mask=fields;bit=MLN_GPU_DEVICE_OPTION_ADAPTER_INDEX");
  /**
   * The 16-byte UUID of the GPU to use: Vulkan's
   * `VkPhysicalDeviceIDProperties.deviceUUID`, or OpenGL's
   * `GL_DEVICE_UUID_EXT`.
   */
  mln_buffer_view adapter_uuid MLN_BINDING(
    "encoding=bytes;mask=fields;bit=MLN_GPU_DEVICE_OPTION_ADAPTER_UUID"
  );
  /**
   * The 8-byte LUID of the GPU to use: on Windows, its DXGI adapter LUID,
   * which Vulkan reports as `VkPhysicalDeviceIDProperties.deviceLUID`, and on
   * Apple platforms, its `MTLDevice.registryID`, in the platform's byte order.
   */
  mln_buffer_view adapter_luid MLN_BINDING(
    "encoding=bytes;mask=fields;bit=MLN_GPU_DEVICE_OPTION_ADAPTER_LUID"
  );
  /**
   * The OpenGL API that an OpenGL device renders with. An EGL device
   * defaults to MLN_OPEN_GL_CLIENT_API_GLES, and takes
   * MLN_OPEN_GL_CLIENT_API_GL too on Linux and FreeBSD. A WGL device takes
   * only MLN_OPEN_GL_CLIENT_API_GL, its default, and a WebGL device only
   * MLN_OPEN_GL_CLIENT_API_GLES, its default.
   */
  mln_open_gl_client_api open_gl_client_api
    MLN_BINDING("mask=fields;bit=MLN_GPU_DEVICE_OPTION_OPEN_GL_CLIENT_API");
  /**
   * Whether the sessions of an EGL device take turns in one OpenGL context
   * instead of having one each.
   */
  bool open_gl_single_context
    MLN_BINDING("mask=fields;bit=MLN_GPU_DEVICE_OPTION_OPEN_GL_SINGLE_CONTEXT");
  /**
   * Your lock on a Vulkan device's queue, when you submit to the queue that
   * mln_gpu_device_get_vulkan_context() reports. Only a Vulkan device takes
   * one.
   */
  mln_queue_lock queue_lock;
} mln_gpu_device_options;

/** What a GPU device can do. */
typedef struct mln_gpu_device_capabilities {
  uint32_t size;
  mln_render_backend backend;
  /** The kinds of mln_render_target that sessions on the device render to. */
  mln_render_target_kind_flags target_kinds;
  /**
   * The ways that the device can share the textures of an exported ring. A
   * kind is here only once the device has exported a texture of it, so a
   * driver that offers an extension but can't allocate the memory reports no
   * kind of it.
   */
  mln_gpu_export_kind_flags export_kinds;
  /** The kinds of mln_gpu_sync that the device can signal and wait on. */
  mln_gpu_sync_kind_flags sync_kinds;
  /** The formats of the depth textures that a borrowed ring takes. */
  mln_depth_stencil_format_flags depth_stencil_formats;
  /**
   * The kinds of `export_kinds` whose rings also share each frame's depth
   * texture, with mln_render_session_attach_options.keep_depth. An IOSurface
   * or a dmabuf holds no depth format, so only MLN_GPU_EXPORT_KIND_OPAQUE_FD
   * can be here.
   */
  mln_gpu_export_kind_flags depth_export_kinds;
} mln_gpu_device_capabilities;

/**
 * Your Vulkan device and queue, for mln_gpu_device_wrap_vulkan().
 *
 * The device must have the extensions that its sessions use enabled:
 * `VK_KHR_swapchain` for windows and Vulkan surfaces,
 * `VK_KHR_external_semaphore_fd` for MLN_GPU_SYNC_SYNC_FD,
 * `VK_KHR_external_memory_fd` for MLN_GPU_EXPORT_KIND_OPAQUE_FD, and that with
 * `VK_EXT_external_memory_dma_buf` and `VK_EXT_image_drm_format_modifier` for
 * MLN_GPU_EXPORT_KIND_DMA_BUF. List the ones you enabled in
 * `enabled_extensions`. When the GPU supports timeline semaphores, also enable
 * the `timelineSemaphore` feature, of Vulkan 1.2 or
 * `VK_KHR_timeline_semaphore`: the device offers
 * MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE wherever the GPU supports it.
 */
typedef struct mln_vulkan_device_descriptor {
  uint32_t size;
  mln_vulkan_context_descriptor context;
  /**
   * Your lock on `context.graphics_queue`, when you submit to it yourself.
   * Give every device that wraps the queue the same lock.
   */
  mln_queue_lock queue_lock;
  /**
   * The names of the device extensions that you enabled on `context.device`,
   * such as "VK_KHR_external_semaphore_fd". Vulkan offers no way to ask a
   * device which extensions it has, so the device offers the export kinds and
   * MLN_GPU_SYNC_SYNC_FD only when the extensions that they need are listed
   * here, and none of them when the list is empty.
   */
  const mln_buffer_view* enabled_extensions
    MLN_BINDING("length=enabled_extension_count");
  size_t enabled_extension_count;
} mln_vulkan_device_descriptor;

/** Your Metal device and command queue, for mln_gpu_device_wrap_metal(). */
typedef struct mln_metal_device_descriptor {
  uint32_t size;
  /** Your `MTLDevice`. */
  void* device MLN_BINDING("required=true");
  /**
   * Your `MTLCommandQueue` on `device`, or null for a queue of the library's.
   * With your queue, borrowed rings can use MLN_GPU_SYNC_QUEUE_ORDER.
   */
  void* command_queue;
} mln_metal_device_descriptor;

/**
 * The Vulkan objects of a device, as mln_gpu_device_get_vulkan_context()
 * reports them.
 */
typedef struct mln_gpu_device_vulkan_context_result {
  uint32_t size;
  mln_vulkan_context_descriptor context;
} mln_gpu_device_vulkan_context_result;

/**
 * Creates a GPU device that the library owns, for render sessions to share.
 *
 * The sessions on a device share its GPU and queue: one `VkDevice` and queue,
 * one `MTLDevice` and command queue, or one OpenGL share group.
 * mln_map_attach() creates a device of its own when it gets none. Create one
 * to share it among the sessions of many maps, or to pick the backend or the
 * GPU. A Vulkan device enables the presentation, timeline semaphore, external
 * memory, and external semaphore extensions that its GPU offers.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when this build or platform lacks `backend`, when
 *   the device can't render with `open_gl_client_api`, when a device that
 *   doesn't render with Vulkan gets a `queue_lock`, or in a browser WebGPU
 *   build, where you attach without a device or wrap the page's.
 * - MLN_STATUS_NOT_FOUND when no GPU matches `adapter_index`, `adapter_uuid`,
 *   or `adapter_luid`.
 * - MLN_STATUS_NATIVE_ERROR when the platform can't create the device.
 */
MLN_API mln_status mln_gpu_device_create(
  const mln_gpu_device_options* options MLN_BINDING("nullable=true"),
  mln_gpu_device* out_device MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Wraps your Vulkan device and queue, so that sessions render on them.
 *
 * Sessions on the device render into your images, present to your surfaces,
 * and submit to your queue, under your queue lock when you give one. Sessions
 * of several devices that wrap one queue never submit to it at once.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when mln_supported_render_backend_mask() lacks
 *   MLN_RENDER_BACKEND_FLAG_VULKAN.
 * - MLN_STATUS_NATIVE_ERROR when the library can't load the device's
 *   functions.
 *
 * Lifetimes:
 * Keep the objects that `descriptor` names alive until every session on the
 * device detaches or is released, or is abandoned, and you release the device.
 */
MLN_BINDING("platforms=android,freebsd,linux,macos,ohos,windows")
MLN_API mln_status mln_gpu_device_wrap_vulkan(
  const mln_vulkan_device_descriptor* descriptor,
  mln_gpu_device* out_device MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Wraps your Metal device and command queue, so that sessions render on them.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when mln_supported_render_backend_mask() lacks
 *   MLN_RENDER_BACKEND_FLAG_METAL.
 *
 * Lifetimes:
 * The device retains the objects that `descriptor` names until every session
 * on it ends and you release it.
 */
MLN_BINDING("platforms=ios,macos,tvos,visionos")
MLN_API mln_status mln_gpu_device_wrap_metal(
  const mln_metal_device_descriptor* descriptor,
  mln_gpu_device* out_device MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Makes an OpenGL device whose share group is your context's.
 *
 * Each session on the device makes a context of its own that shares objects
 * with yours, so that it can render into your textures and present to your
 * surfaces. A device that the library creates is a share group of its own.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when mln_open_gl_supported_context_provider_mask()
 *   lacks the context's platform.
 * - MLN_STATUS_NATIVE_ERROR when the platform rejects the context.
 *
 * Lifetimes:
 * Keep the objects that `context` names alive until every session on the
 * device detaches or is released, or is abandoned, and you release the device.
 */
MLN_API mln_status mln_gpu_device_wrap_open_gl(
  const mln_open_gl_context_descriptor* context,
  mln_gpu_device* out_device MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Wraps your WebGPU device, so that sessions render on it and present to the
 * surfaces you make with its instance.
 *
 * Sessions on the device run on MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED outside a browser WebGPU build.
 *
 * Lifetimes:
 * Keep the objects that `context` names alive until every session on the
 * device detaches or is released, or is abandoned.
 */
MLN_BINDING("platforms=web")
MLN_API mln_status mln_gpu_device_wrap_web_gpu(
  const mln_web_gpu_context_descriptor* context,
  mln_gpu_device* out_device MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads what the device can do.
 *
 * Check `export_kinds` before you attach an exported ring, to choose between
 * the ring and a way of your own to show the map.
 */
MLN_API mln_status mln_gpu_device_get_capabilities(
  mln_gpu_device device,
  mln_gpu_device_capabilities* out_capabilities MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gets the Vulkan instance, device, and graphics queue of a Vulkan device.
 *
 * Use them to make the images of a borrowed ring, or a surface, that sessions
 * on the device render to. The sessions on the device share its graphics
 * queue. To submit to the queue yourself, create the device with
 * mln_gpu_device_options.queue_lock.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when the device doesn't render with Vulkan.
 *
 * Lifetimes:
 * The objects that `out_result` names stay valid until you release the
 * device.
 */
MLN_BINDING("platforms=android,freebsd,linux,macos,ohos,windows")
MLN_API mln_status mln_gpu_device_get_vulkan_context(
  mln_gpu_device device,
  mln_gpu_device_vulkan_context_result* out_result MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases a GPU device.
 *
 * Render sessions on the device keep using it until they detach, or their
 * release finishes.
 *
 * Lifetimes:
 * The library destroys a device of its own once you release it and no
 * session uses it. Destroy the Vulkan objects that you created with its
 * context, such as images, before then. A device that wraps yours releases
 * its queue lock then.
 */
MLN_API void mln_gpu_device_release(mln_gpu_device device) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_GPU_DEVICE_H
