/**
 * @file maplibre_native_c/vulkan.h
 * Render with a Vulkan device that the library creates and owns.
 */

#ifndef MAPLIBRE_NATIVE_C_VULKAN_H
#define MAPLIBRE_NATIVE_C_VULKAN_H

#include <stdint.h>

#include "base.h"
#include "render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The fields that mln_vulkan_device_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_vulkan_device_option_field : uint32_t {
  MLN_VULKAN_DEVICE_OPTION_PRESENTATION = 1U << 0U,
  MLN_VULKAN_DEVICE_OPTION_DEVICE_UUID = 1U << 1U,
  MLN_VULKAN_DEVICE_OPTION_DEVICE_LUID = 1U << 2U,
} mln_vulkan_device_option_field;

/** A set of mln_vulkan_device_option_field bits. */
typedef uint32_t mln_vulkan_device_option_fields;

/** Options for mln_vulkan_device_create(). */
typedef struct mln_vulkan_device_options {
  uint32_t size;
  mln_vulkan_device_option_fields fields;
  /**
   * Whether the device can present to a window. The instance then enables
   * VK_KHR_surface and the platform's surface extensions, and the device
   * enables VK_KHR_swapchain. Defaults to false.
   */
  bool presentation
    MLN_BINDING("mask=fields;bit=MLN_VULKAN_DEVICE_OPTION_PRESENTATION");
  /**
   * The 16-byte `deviceUUID` of the GPU to use, as
   * `VkPhysicalDeviceIDProperties` reports it.
   */
  mln_buffer_view device_uuid MLN_BINDING(
    "encoding=bytes;mask=fields;bit=MLN_VULKAN_DEVICE_OPTION_DEVICE_UUID"
  );
  /**
   * The 8-byte `deviceLUID` of the GPU to use, as
   * `VkPhysicalDeviceIDProperties` reports it. On Windows, this is the GPU's
   * DXGI adapter LUID.
   */
  mln_buffer_view device_luid MLN_BINDING(
    "encoding=bytes;mask=fields;bit=MLN_VULKAN_DEVICE_OPTION_DEVICE_LUID"
  );
} mln_vulkan_device_options;

/** A Vulkan device's context, as mln_vulkan_device_get_context() reports it. */
typedef struct mln_vulkan_device_context_result {
  uint32_t size;
  mln_vulkan_context_descriptor context;
} mln_vulkan_device_context_result;

/**
 * Creates a Vulkan instance, device, and graphics queue for render sessions.
 *
 * Attach sessions with the context from mln_vulkan_device_get_context(), in
 * place of a device of your own. Without `device_uuid` or `device_luid`, it
 * prefers a discrete GPU. The device enables the external memory and semaphore
 * extensions that the GPU offers.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when mln_supported_render_backend_mask() lacks
 *   MLN_RENDER_BACKEND_FLAG_VULKAN, or when `presentation` is true and the
 *   Vulkan loader or the GPU can't present.
 * - MLN_STATUS_NOT_FOUND when no GPU matches `device_uuid` and `device_luid`.
 * - MLN_STATUS_NATIVE_ERROR when there is no Vulkan loader, no GPU has a
 *   graphics queue, or Vulkan can't create the instance or the device.
 */
MLN_BINDING("platforms=android,freebsd,linux,macos,ohos,windows")
MLN_API mln_status mln_vulkan_device_create(
  const mln_vulkan_device_options* options MLN_BINDING("nullable=true"),
  mln_vulkan_device* out_device MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gets the context to attach render sessions to the device with.
 *
 * Pass it as the context of mln_map_attach_vulkan_surface() or
 * mln_map_attach_vulkan_borrowed_texture(). The sessions on the device share
 * its graphics queue. To submit to the queue yourself, set
 * mln_render_session_attach_options.queue_lock on each session.
 *
 * Lifetimes:
 * The objects that `out_result` names stay valid until you release the device.
 */
MLN_BINDING("platforms=android,freebsd,linux,macos,ohos,windows")
MLN_API mln_status mln_vulkan_device_get_context(
  mln_vulkan_device device,
  mln_vulkan_device_context_result* out_result MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases a Vulkan device.
 *
 * Render sessions that attached with the device's context keep using it until
 * they detach or their release finishes.
 *
 * Lifetimes:
 * The library destroys the device once you release it and no session uses it.
 * Destroy the Vulkan objects that you created with its context, such as images
 * and surfaces, before then.
 */
MLN_API void mln_vulkan_device_release(mln_vulkan_device device) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_VULKAN_H
