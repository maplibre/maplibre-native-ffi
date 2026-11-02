/**
 * @file maplibre_native_c/offscreen.h
 * Render maps offscreen on a GPU device that the library creates.
 */

#ifndef MAPLIBRE_NATIVE_C_OFFSCREEN_H
#define MAPLIBRE_NATIVE_C_OFFSCREEN_H

#include <stdint.h>

#include "base.h"
#include "completion.h"
#include "render_session.h"
#include "render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The fields that mln_offscreen_device_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_offscreen_device_option_field : uint32_t {
  MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX = 1U << 0U,
  MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT = 1U << 1U,
} mln_offscreen_device_option_field;

/** A set of mln_offscreen_device_option_field bits. */
typedef uint32_t mln_offscreen_device_option_fields;

/** Options for mln_offscreen_device_create(). */
typedef struct mln_offscreen_device_options {
  uint32_t size;
  mln_offscreen_device_option_fields fields;
  /** The GPU to use, as an index into the platform's list of GPUs. */
  uint32_t adapter_index
    MLN_BINDING("mask=fields;bit=MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX");
  /**
   * Whether the device's sessions take turns in one OpenGL context instead of
   * having one each. Only an EGL device reads it.
   */
  bool single_context
    MLN_BINDING("mask=fields;bit=MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT");
} mln_offscreen_device_options;

/** The image that mln_map_attach_offscreen() renders. */
typedef struct mln_offscreen_descriptor {
  uint32_t size;
  mln_logical_extent extent MLN_BINDING("required=true");
} mln_offscreen_descriptor;

/**
 * Creates a GPU device for rendering maps offscreen.
 *
 * mln_map_attach_offscreen() creates a device of its own when it gets none.
 * Create one to share it among the sessions on many maps, or to pick a GPU.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when this build's render backend can't create a
 *   device.
 * - MLN_STATUS_NATIVE_ERROR when `adapter_index` names no GPU, or the platform
 *   can't create the device.
 */
MLN_API mln_status mln_offscreen_device_create(
  const mln_offscreen_device_options* options MLN_BINDING("nullable=true"),
  mln_offscreen_device* out_device MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases an offscreen device.
 *
 * Render sessions on the device keep working until they detach.
 */
MLN_API void mln_offscreen_device_release(
  mln_offscreen_device device
) MLN_NOEXCEPT;

/**
 * Attaches a render session that renders the map offscreen.
 *
 * Read each image's pixels with mln_render_session_read_texture(). `options`
 * must select MLN_RENDER_DRIVER_CORE_WORKER.
 *
 * The session renders on `device`. When `device` is null, the session creates
 * a device with the default options, and releases it when the session ends.
 *
 * Returns:
 * - MLN_STATUS_INVALID_STATE when the map already has a render session.
 * - MLN_STATUS_UNSUPPORTED when `device` is null and this build's render
 *   backend can't create a device.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when `device` is null and the platform can't
 *   create a device.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_attach_offscreen(
  mln_map map, const mln_offscreen_descriptor* descriptor,
  const mln_render_session_attach_options* options,
  mln_offscreen_device device MLN_BINDING("nullable=true"),
  mln_render_session* out_session MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif
