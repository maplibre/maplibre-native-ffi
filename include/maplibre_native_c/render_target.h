/**
 * @file maplibre_native_c/render_target.h
 * Shared public C API declarations for render target descriptors.
 */

#ifndef MAPLIBRE_NATIVE_C_RENDER_TARGET_H
#define MAPLIBRE_NATIVE_C_RENDER_TARGET_H

#include <stdint.h>

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Logical render target extent in UI pixels. */
typedef struct mln_render_target_extent {
  uint32_t size;
  /** Logical map width in UI pixels. */
  uint32_t width;
  /** Logical map height in UI pixels. */
  uint32_t height;
  /** UI-to-device pixel scale. Must be positive and finite. */
  double scale_factor;
} mln_render_target_extent;

/** Metal backend context fields shared by Metal render targets. */
typedef struct mln_metal_context_descriptor {
  uint32_t size;
  /** id<MTLDevice> / MTL::Device*. Retained when the target requires it. */
  void* device;
} mln_metal_context_descriptor;

/** Vulkan backend context fields shared by Vulkan render targets. */
typedef struct mln_vulkan_context_descriptor {
  uint32_t size;
  /** Borrowed VkInstance. Required. */
  void* instance;
  /** Borrowed VkPhysicalDevice. Required. */
  void* physical_device;
  /** Borrowed VkDevice. Required. */
  void* device;
  /** Borrowed graphics VkQueue. Required. */
  void* graphics_queue;
  /** Queue family index for graphics_queue. Must support graphics commands. */
  uint32_t graphics_queue_family_index;
  /** PFN_vkGetInstanceProcAddr for the loader that created the Vulkan handles.
   */
  void* get_instance_proc_addr;
  /** PFN_vkGetDeviceProcAddr for the loader that created the Vulkan device. */
  void* get_device_proc_addr;
} mln_vulkan_context_descriptor;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_RENDER_TARGET_H
