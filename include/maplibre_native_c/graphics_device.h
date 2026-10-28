/**
 * @file maplibre_native_c/graphics_device.h
 * Public C API declarations for graphics devices that native code creates.
 */

#ifndef MAPLIBRE_NATIVE_C_GRAPHICS_DEVICE_H
#define MAPLIBRE_NATIVE_C_GRAPHICS_DEVICE_H

#include <stdint.h>

#include "base.h"
#include "completion.h"
#include "render_session.h"
#include "render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Fields present in mln_graphics_device_options. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_graphics_device_option_field : uint32_t {
  /** adapter_index is set. */
  MLN_GRAPHICS_DEVICE_OPTION_ADAPTER_INDEX = 1U << 0U,
  /** single_context is set. */
  MLN_GRAPHICS_DEVICE_OPTION_SINGLE_CONTEXT = 1U << 1U,
} mln_graphics_device_option_field;

/** A set of mln_graphics_device_option_field bits. */
typedef uint32_t mln_graphics_device_option_fields;

/** Options for creating a graphics device. */
typedef struct mln_graphics_device_options {
  uint32_t size;
  /** Bitwise combination of mln_graphics_device_option_field values. */
  mln_graphics_device_option_fields fields;
  /**
   * The adapter to create the device on, as a position in the graphics API's
   * own list of adapters, from zero. Without it, the backend picks its default
   * adapter.
   *
   * Metal lists adapters with MTLCopyAllDevices() on macOS and has only the
   * system default device elsewhere. Vulkan lists physical devices with
   * vkEnumeratePhysicalDevices(), and its default prefers a discrete GPU, then
   * an integrated one, then any other. EGL lists devices with
   * eglQueryDevicesEXT() on Linux and FreeBSD, and its default display takes
   * the platform's own choice. WGL has one adapter, the driver of the primary
   * display.
   */
  uint32_t adapter_index
    MLN_BINDING("mask=fields;bit=MLN_GRAPHICS_DEVICE_OPTION_ADAPTER_INDEX");
  /**
   * Renders every texture session of the device in one graphics context, one
   * session at a time, instead of in a context of each session's own. Absent
   * selects false.
   *
   * This saves the memory of a context per session. The sessions of such a
   * device never render at the same time, so a host that renders on several
   * threads at once creates a device for each thread.
   *
   * EGL creates the context with the device. Metal and Vulkan sessions share
   * the device's one device and queue regardless, so those backends ignore
   * this. WGL does not support it.
   */
  bool single_context
    MLN_BINDING("mask=fields;bit=MLN_GRAPHICS_DEVICE_OPTION_SINGLE_CONTEXT");
} mln_graphics_device_options;

/** Attachment options for a graphics device texture target. */
typedef struct mln_graphics_device_texture_descriptor {
  uint32_t size;
  /**
   * Logical texture extent. Required. A scale_factor that differs from the
   * map's is accepted and logged as a warning.
   */
  mln_logical_extent extent MLN_BINDING("required=true");
} mln_graphics_device_texture_descriptor;

/**
 * Creates a graphics device for this build's render backend.
 *
 * The device lets a host render to a texture and read its pixels without
 * creating graphics objects of its own: a Metal device, a Vulkan instance,
 * device, and graphics queue, an EGL display and config, or a hidden window
 * for WGL. Any number of texture sessions can share one device. A Vulkan
 * device synchronizes its queue among those sessions, and an EGL device
 * created with single_context has them take turns in one context.
 *
 * The device is not tied to a thread, so any thread may use or release it.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when options is undersized or carries unknown
 *   fields, or out_device is null or *out_device is not MLN_HANDLE_NULL.
 * - MLN_STATUS_UNSUPPORTED when this build's backend cannot create a device:
 *   WebGPU, and OpenGL on WebGL; or options set single_context on WGL.
 * - MLN_STATUS_NATIVE_ERROR when the graphics API is unavailable, has no
 *   adapter at adapter_index, or fails to create the device or its single
 *   context.
 */
MLN_API mln_status mln_graphics_device_create(
  const mln_graphics_device_options* options MLN_BINDING("nullable=true"),
  mln_graphics_device* out_device MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases a graphics device.
 *
 * Callable from any thread. A null or already-released handle is a no-op.
 * Sessions attached to the device keep their own reference, so releasing the
 * handle preserves each session, and the device's graphics objects are
 * destroyed after the last such session tears down.
 */
MLN_API void mln_graphics_device_release(
  mln_graphics_device device
) MLN_NOEXCEPT;

/**
 * Starts attachment of a texture target that renders on a graphics device.
 *
 * The session owns one texture on the device, renders on a core worker, and
 * grants readback only. The session keeps its own reference to the device, so
 * the host may release the device handle at any time. The descriptor and the
 * options are copied before return.
 *
 * The options' driver must be MLN_RENDER_DRIVER_CORE_WORKER. Their requested
 * ring depth is ignored, and they must not enable a queue lock, because only
 * the device uses its queue.
 *
 * Returns:
 * - MLN_STATUS_OK when the attachment is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map or device is an invalid handle;
 *   descriptor, options, or completion is null or undersized;
 *   descriptor->extent has a zero width or height, a scale_factor that is not
 *   finite and positive, or a scaled dimension past UINT32_MAX; out_session is
 *   null or does not point to the null handle; or the requested driver kind is
 *   unknown, or options carry a malformed wake or queue lock.
 * - MLN_STATUS_INVALID_STATE when map or device has been released, or the
 *   map's creation has not succeeded.
 * - MLN_STATUS_UNSUPPORTED when the driver is not the core worker, options
 *   enable a queue lock, or options request a producer sync kind other than
 *   MLN_GPU_SYNC_CPU_COMPLETE.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver owns the target.
 * - MLN_STATUS_NATIVE_ERROR when target initialization fails.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_attach_graphics_device_texture(
  mln_map map, mln_graphics_device device,
  const mln_graphics_device_texture_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif
