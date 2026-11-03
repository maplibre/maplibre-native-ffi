/**
 * @file maplibre_native_c/base.h
 * Statuses, diagnostics, handles, and other types that every header uses.
 */

#ifndef MAPLIBRE_NATIVE_C_BASE_H
#define MAPLIBRE_NATIVE_C_BASE_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "binding.h"

#ifdef _WIN32
#if defined(MLN_STATIC)
#define MLN_API
#elif defined(MLN_BUILDING_C)
#define MLN_API __declspec(dllexport)
#else
#define MLN_API __declspec(dllimport)
#endif
#else
#define MLN_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
#define MLN_NOEXCEPT noexcept
#else
#define MLN_NOEXCEPT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/** The result of a call or of asynchronous work. */
typedef enum mln_status : int32_t {
  MLN_STATUS_OK = 0,
  /** An argument is invalid, such as malformed JSON or a value out of range. */
  MLN_STATUS_INVALID_ARGUMENT = -1,
  /**
   * The call doesn't fit the current state, such as removing a source that a
   * layer still uses.
   */
  MLN_STATUS_INVALID_STATE = -2,
  /** The call came from a thread that can't make it. */
  MLN_STATUS_WRONG_THREAD = -3,
  /** The platform, the build, or the render target lacks the feature. */
  MLN_STATUS_UNSUPPORTED = -4,
  /**
   * The library or the platform hit an error, such as a failed graphics driver
   * call.
   */
  MLN_STATUS_NATIVE_ERROR = -5,
  /**
   * The work stopped before it finished, such as when its handle was
   * released.
   */
  MLN_STATUS_CANCELLED = -6,
  /** Another call is still using the same object. */
  MLN_STATUS_BUSY = -7,
  /** The render target can't be drawn to anymore. */
  MLN_STATUS_TARGET_LOST = -8,
  /** Nothing has the given ID. */
  MLN_STATUS_NOT_FOUND = -10,
} mln_status;

/** The size of mln_diagnostic's message in bytes, with its terminator. */
#define MLN_DIAGNOSTIC_MESSAGE_CAPACITY 4096

/**
 * A message that explains why a call failed.
 *
 * Lifetimes:
 * A function writes a NUL-terminated message, or an empty one when it
 * succeeds. Pass null as `out_diagnostic` to skip the message.
 */
typedef struct mln_diagnostic {
  uint32_t size;
  char message[MLN_DIAGNOSTIC_MESSAGE_CAPACITY];
} mln_diagnostic;

/** A render backend, as a bit in a set. */
typedef enum MLN_BINDING("kind=bitmask") mln_render_backend_flag : uint32_t {
  MLN_RENDER_BACKEND_FLAG_METAL = 1u << 0u,
  MLN_RENDER_BACKEND_FLAG_VULKAN = 1u << 1u,
  MLN_RENDER_BACKEND_FLAG_OPEN_GL = 1u << 2u,
  MLN_RENDER_BACKEND_FLAG_WEB_GPU = 1u << 3u,
} mln_render_backend_flag;

/** A set of mln_render_backend_flag bits. */
typedef uint32_t mln_render_backend_flags;

/** The handle that names no object. */
#define MLN_HANDLE_NULL ((uint64_t)0)

/** A runtime owns maps and the network and cache settings that they share. */
typedef uint64_t mln_runtime MLN_BINDING(
  "kind=handle;release=mln_runtime_release;"
  "dispose=mln_runtime_dispose"
);
/** A map, with its style and camera. */
typedef uint64_t mln_map MLN_BINDING(
  "kind=handle;release=mln_map_release;parent=mln_runtime;"
  "dispose=mln_map_dispose"
);
/** GeoJSON data, parsed and ready to add to a GeoJSON source. */
typedef uint64_t mln_geo_json_source_data MLN_BINDING(
  "kind=handle;release=mln_geo_json_source_data_release;"
  "dispose=mln_geo_json_source_data_release"
);
/** A parsed style expression. */
typedef uint64_t mln_style_expression MLN_BINDING(
  "kind=handle;release=mln_style_expression_release;"
  "dispose=mln_style_expression_release"
);
/** A GPU device that the library creates to render maps offscreen. */
typedef uint64_t mln_offscreen_device MLN_BINDING(
  "kind=handle;release=mln_offscreen_device_release;"
  "dispose=mln_offscreen_device_release"
);
/** A camera view for converting between screen points and coordinates. */
typedef uint64_t mln_map_projection MLN_BINDING(
  "kind=handle;release=mln_map_projection_release;"
  "dispose=mln_map_projection_release"
);
/** A resource load that a resource provider answers. */
typedef uint64_t mln_resource_request MLN_BINDING(
  "kind=handle;release=mln_resource_request_release;"
  "dispose=mln_resource_request_release"
);
/** A URL rewrite that a resource transform answers with the URL to load. */
typedef uint64_t mln_resource_transform_request MLN_BINDING(
  "kind=handle;release=mln_resource_transform_request_release;"
  "dispose=mln_resource_transform_request_release"
);
/** A render session draws a map into a surface or a texture. */
typedef uint64_t mln_render_session MLN_BINDING(
  "kind=handle;release=mln_render_session_dispose;parent=mln_map;"
  "abandon=mln_render_session_abandon;dispose=mln_render_session_dispose"
);
/** A rendered frame that the caller holds to read or show. */
typedef uint64_t mln_acquired_frame MLN_BINDING(
  "kind=handle;release=mln_acquired_frame_release;parent=mln_render_session;"
  "discard=mln_acquired_frame_discard;"
  "view_begin=mln_acquired_frame_view_begin;"
  "view_end=mln_acquired_frame_view_end"
);
/** The frame results that mln_render_session_drain_frame_results() took. */
typedef uint64_t mln_render_frame_batch MLN_BINDING(
  "kind=handle;release=mln_render_frame_batch_release;"
  "dispose=mln_render_frame_batch_release"
);
/** A queue that holds callback calls until the caller dispatches them. */
typedef uint64_t mln_delivery_queue MLN_BINDING(
  "kind=handle;release=mln_delivery_queue_release;"
  "dispose=mln_delivery_queue_release"
);

/**
 * A range of bytes, such as UTF-8 text.
 *
 * Lifetimes:
 * `data` can be null when `size` is 0. Text ends at `size` and needs no NUL
 * terminator.
 */
typedef struct mln_buffer_view {
  const void* data;
  size_t size MLN_BINDING("kind=count");
} mln_buffer_view;

/**
 * Frees the `user_data` of a callback registration.
 *
 * Lifetimes:
 * Native calls it once, after the last call to the registration's callback.
 * When the function that takes the registration fails, native never calls it,
 * and the caller still owns `user_data`.
 */
typedef void (*mln_user_data_release)(void* user_data);

/** The API epoch of these headers. */
#define MLN_C_VERSION 0u

/**
 * Returns the API epoch of the library.
 *
 * Headers and a library with different epochs aren't compatible. Compare the
 * result with MLN_C_VERSION.
 */
MLN_API uint32_t mln_c_version(void) MLN_NOEXCEPT;

/** Returns the render backends that this build of the library draws with. */
MLN_API mln_render_backend_flags
mln_supported_render_backend_mask(void) MLN_NOEXCEPT;

// Helpers for C callers. Each binding has its own, so the generator skips
// these.
#ifndef MLN_BINDGEN

/**
 * Makes a buffer view of a NUL-terminated string, without the terminator.
 *
 * A null `text` gives an empty view.
 */
static inline mln_buffer_view mln_buffer_view_from_cstr(const char* text) {
  // Counts instead of calling strlen() so the headers stay freestanding.
  size_t size = 0;
  if (text != NULL) {
    while (text[size] != '\0') {
      size++;
    }
  }
  const mln_buffer_view view = {text, size};
  return view;
}

/**
 * Points at item `index` of an array whose items are `stride` bytes apart.
 *
 * Use it to read arrays whose item size native reports, such as a completion's
 * `value`, because a newer library can return larger items.
 */
#define MLN_STRIDED_AT(type, items, stride, index) \
  ((const type*)((const unsigned char*)(items) +   \
                 ((size_t)(index) * (size_t)(stride))))

#endif  // MLN_BINDGEN

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_BASE_H
