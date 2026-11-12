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
} mln_gpu_device_options;

/** A way to share a texture's memory with another API or process. */
typedef enum MLN_BINDING("kind=bitmask") mln_gpu_export_kind_flag : uint32_t {
  /** An `IOSurfaceRef`. */
  MLN_GPU_EXPORT_KIND_FLAG_IO_SURFACE = 1U << 0U,
  /** A Linux dmabuf, as file descriptors with a DRM format and modifier. */
  MLN_GPU_EXPORT_KIND_FLAG_DMA_BUF = 1U << 1U,
  /** An `AHardwareBuffer*`. */
  MLN_GPU_EXPORT_KIND_FLAG_ANDROID_HARDWARE_BUFFER = 1U << 2U,
  /** A DXGI shared texture's NT handle. */
  MLN_GPU_EXPORT_KIND_FLAG_DXGI_SHARED_HANDLE = 1U << 3U,
  /** A Vulkan or OpenGL opaque memory file descriptor. */
  MLN_GPU_EXPORT_KIND_FLAG_OPAQUE_FD = 1U << 4U,
} mln_gpu_export_kind_flag;

/** A set of mln_gpu_export_kind_flag bits. */
typedef uint32_t mln_gpu_export_kind_flags;

/** What a GPU device can do. */
typedef struct mln_gpu_device_capabilities {
  uint32_t size;
  mln_render_backend backend;
  /** The kinds of mln_render_target that sessions on the device render to. */
  mln_render_target_kind_flags target_kinds;
  /** The ways that the device can share the textures it renders. */
  mln_gpu_export_kind_flags export_kinds;
  /** The kinds of mln_gpu_sync that the device can signal and wait on. */
  mln_gpu_sync_kind_flags sync_kinds;
} mln_gpu_device_capabilities;

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
 * mln_map_attach() creates a device of its own when it gets none. Create one
 * to share it among the sessions of many maps, or to pick the backend or the
 * GPU. A Vulkan device enables the presentation, external memory, and
 * external semaphore extensions that its GPU offers.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when this build or platform lacks `backend`, when
 *   the device can't render with `open_gl_client_api`, or in a browser
 *   WebGPU build, where you attach without a device or wrap the page's.
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

/** Reads what the device can do. */
MLN_API mln_status mln_gpu_device_get_capabilities(
  mln_gpu_device device,
  mln_gpu_device_capabilities* out_capabilities MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gets the Vulkan instance, device, and graphics queue of a Vulkan device.
 *
 * Pass the context to mln_map_attach_vulkan_borrowed_texture() to render
 * into your images on the device. The sessions on the device share its
 * graphics queue. To submit to the queue yourself, set
 * mln_render_session_attach_options.queue_lock on each session.
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
 * context, such as images, before then.
 */
MLN_API void mln_gpu_device_release(mln_gpu_device device) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_GPU_DEVICE_H
