/**
 * @file maplibre_native_c/base.h
 * Public C API declarations for base ABI types, status values, and handles.
 *
 * Handles are opaque 64-bit generational ids.
 *
 * Each id packs the handle's type, a slot index, and a reuse generation, so a
 * released handle stays distinguishable from every later handle. A handle is
 * released once the call that ends it, such as a release, close, destroy, or
 * dispose, accepts it. Passing a released handle reports
 * MLN_STATUS_INVALID_STATE. Passing an invalid handle, which is the null
 * handle, a handle of another type, or a value this library never issued,
 * reports MLN_STATUS_INVALID_ARGUMENT. Either way the call has no effect, and
 * its diagnostic names the case. Handle values are safe to copy, compare,
 * hash, and move between threads, and carry no ownership on their own.
 *
 * The bit layout is internal. Hosts pass handles back as issued rather than
 * decoding or synthesizing them.
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

/** Status values returned by status-returning functions. */
typedef enum mln_status : int32_t {
  MLN_STATUS_OK = 0,
  /** A pointer, size field, mask, or handle argument was invalid. */
  MLN_STATUS_INVALID_ARGUMENT = -1,
  /** The object is valid but not currently in a state that permits the call. */
  MLN_STATUS_INVALID_STATE = -2,
  /** The handle is thread-affine and the call was made from the wrong thread.
   */
  MLN_STATUS_WRONG_THREAD = -3,
  /** The entry point or requested behavior is unavailable in this build. */
  MLN_STATUS_UNSUPPORTED = -4,
  /** A native MapLibre error or C++ exception was converted to status. */
  MLN_STATUS_NATIVE_ERROR = -5,
  /** The operation reached its terminal cancelled disposition. */
  MLN_STATUS_CANCELLED = -6,
  /** A conflicting driver call or lifecycle transition is in flight. */
  MLN_STATUS_BUSY = -7,
  /** The render target or graphics receiver was irreversibly lost. */
  MLN_STATUS_TARGET_LOST = -8,
  /** A nonblocking acquisition or service call has no result yet. */
  MLN_STATUS_NOT_READY = -9,
  /** A call named an ID with no live object behind it. */
  MLN_STATUS_NOT_FOUND = -10,
} mln_status;

/** Capacity of mln_diagnostic.message, including the terminating null byte. */
#define MLN_DIAGNOSTIC_MESSAGE_CAPACITY 4096

/**
 * The diagnostic message of one status-returning call.
 *
 * Every status-returning function, except the binding-internal callback
 * adapters, takes a nullable mln_diagnostic* as its last parameter,
 * out_diagnostic. The caller sets size to sizeof(mln_diagnostic). The function
 * writes message as null-terminated UTF-8, truncated to fit. The message is
 * empty when the function returns MLN_STATUS_OK, or MLN_STATUS_NOT_READY from a
 * drain with nothing queued, and describes the failure otherwise. It writes no
 * more than size bytes of the struct. A null out_diagnostic discards the
 * message.
 *
 * Asynchronous failures carry their diagnostic in the completion instead.
 */
typedef struct mln_diagnostic {
  uint32_t size;
  char message[MLN_DIAGNOSTIC_MESSAGE_CAPACITY];
} mln_diagnostic;

/** Render backend support flags reported by this native library build. */
typedef enum MLN_BINDING("kind=bitmask") mln_render_backend_flag : uint32_t {
  MLN_RENDER_BACKEND_FLAG_METAL = 1u << 0u,
  MLN_RENDER_BACKEND_FLAG_VULKAN = 1u << 1u,
  MLN_RENDER_BACKEND_FLAG_OPENGL = 1u << 2u,
  MLN_RENDER_BACKEND_FLAG_WEBGPU = 1u << 3u,
} mln_render_backend_flag;

/**
 * The null handle, for every handle type.
 *
 * A live handle always carries a nonzero kind tag, so this value names no
 * object of any type. Status-returning functions reject it. Void release
 * functions accept it as a no-op. Output handle parameters that create or
 * acquire ownership require `*out_handle` to equal it on entry.
 */
#define MLN_HANDLE_NULL ((uint64_t)0)

/** A runtime: the native scheduler thread and event store for its maps. */
typedef uint64_t mln_runtime MLN_BINDING(
  "kind=handle;release=mln_runtime_release;"
  "dispose=mln_runtime_dispose"
);
/** A map, which holds map state independent of any render target. */
typedef uint64_t mln_map MLN_BINDING(
  "kind=handle;release=mln_map_release;parent=mln_runtime;dispose=mln_map_"
  "dispose"
);
/** A standalone projection of a map's transform state at its creation. */
typedef uint64_t mln_map_projection MLN_BINDING(
  "kind=handle;release=mln_map_projection_close;"
  "dispose=mln_map_projection_close"
);
/** A resource request that a resource provider handles. */
typedef uint64_t mln_resource_request_handle MLN_BINDING(
  "kind=handle;release=mln_resource_request_release;"
  "dispose=mln_resource_request_release;prefix=mln_resource_request"
);
/** A render session, which renders one map to one render target. */
typedef uint64_t mln_render_session MLN_BINDING(
  "kind=handle;release=mln_render_session_destroy;parent=mln_map;"
  "abandon=mln_render_session_abandon;dispose=mln_render_session_dispose"
);
/** An owned batch of runtime events from one drain. */
typedef uint64_t mln_event_batch MLN_BINDING(
  "kind=handle;release=mln_event_batch_release;"
  "dispose=mln_event_batch_release"
);
/** A rendered frame that a render session lends until its release. */
typedef uint64_t mln_acquired_frame MLN_BINDING(
  "kind=handle;release=mln_acquired_frame_release;parent=mln_render_session;"
  "dispose=mln_acquired_frame_dispose;"
  "view_begin=mln_acquired_frame_view_begin;"
  "view_end=mln_acquired_frame_view_end"
);
/** An owned batch of frame results from one drain. */
typedef uint64_t mln_render_frame_batch MLN_BINDING(
  "kind=handle;release=mln_render_frame_batch_release;"
  "dispose=mln_render_frame_batch_release"
);

/**
 * Borrowed data. The data pointer may be null only when size is zero.
 *
 * Each parameter documents whether its view contains UTF-8 text, serialized
 * data, or arbitrary bytes. The view carries no ownership and requires no
 * trailing null byte.
 */
typedef struct mln_buffer_view {
  const void* data;
  size_t size MLN_BINDING("kind=count");
} mln_buffer_view;

/**
 * Releases the user_data of a callback registration.
 *
 * Each registration struct's release_user_data member states when native code
 * calls it and on which thread. A null release_user_data leaves user_data with
 * the caller.
 */
typedef void (*mln_user_data_release)(void* user_data);

/**
 * Reports the C ABI contract version. The value is 0 while the ABI is unstable,
 * and will increment on each SemVer major release.
 */
MLN_API uint32_t mln_c_version(void) MLN_NOEXCEPT;

/**
 * Reports the render backends available in this native library build.
 *
 * The return value is a mask of mln_render_backend_flag values.
 */
MLN_BINDING("enum=mln_render_backend_flag")
MLN_API uint32_t mln_supported_render_backend_mask(void) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_BASE_H
