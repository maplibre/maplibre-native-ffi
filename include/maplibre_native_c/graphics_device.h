/**
 * @file maplibre_native_c/graphics_device.h
 * Render maps into textures on a GPU device that the library creates.
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

/** The fields that mln_graphics_device_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_graphics_device_option_field : uint32_t {
  MLN_GRAPHICS_DEVICE_OPTION_ADAPTER_INDEX = 1U << 0U,
  MLN_GRAPHICS_DEVICE_OPTION_SINGLE_CONTEXT = 1U << 1U,
} mln_graphics_device_option_field;

/** A set of mln_graphics_device_option_field bits. */
typedef uint32_t mln_graphics_device_option_fields;

/** Options for mln_graphics_device_create(). */
typedef struct mln_graphics_device_options {
  uint32_t size;
  mln_graphics_device_option_fields fields;
  /** The GPU to use, as an index into the platform's list of GPUs. */
  uint32_t adapter_index
    MLN_BINDING("mask=fields;bit=MLN_GRAPHICS_DEVICE_OPTION_ADAPTER_INDEX");
  /**
   * Whether the device's texture sessions take turns in one OpenGL context
   * instead of having one each. Only an EGL device reads it.
   */
  bool single_context
    MLN_BINDING("mask=fields;bit=MLN_GRAPHICS_DEVICE_OPTION_SINGLE_CONTEXT");
} mln_graphics_device_options;

/** The texture that mln_map_attach_graphics_device_texture() draws into. */
typedef struct mln_graphics_device_texture_descriptor {
  uint32_t size;
  mln_logical_extent extent MLN_BINDING("required=true");
} mln_graphics_device_texture_descriptor;

/**
 * Creates a GPU device for rendering maps into textures.
 *
 * Render sessions on several maps can share one device.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when this build's render backend can't create a
 *   device.
 * - MLN_STATUS_NATIVE_ERROR when `adapter_index` names no GPU, or the platform
 *   can't create the device.
 */
MLN_API mln_status mln_graphics_device_create(
  const mln_graphics_device_options* options MLN_BINDING("nullable=true"),
  mln_graphics_device* out_device MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases a graphics device.
 *
 * Render sessions on the device keep working until they detach.
 */
MLN_API void mln_graphics_device_release(
  mln_graphics_device device
) MLN_NOEXCEPT;

/**
 * Attaches a render session that draws the map into a texture on `device`.
 *
 * `options` must select MLN_RENDER_DRIVER_CORE_WORKER. Read the frames back
 * with mln_render_session_read_texture().
 *
 * Returns:
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
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
