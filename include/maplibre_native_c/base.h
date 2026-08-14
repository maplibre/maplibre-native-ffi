/**
 * @file maplibre_native_c/base.h
 * Public C API declarations for base ABI types and status values.
 */

#ifndef MAPLIBRE_NATIVE_C_BASE_H
#define MAPLIBRE_NATIVE_C_BASE_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

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
} mln_status;

/** Render backend support flags reported by this native library build. */
typedef enum mln_render_backend_flag : uint32_t {
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

/**
 * Handles are opaque 64-bit generational ids.
 *
 * Each id packs the handle's type, a slot index, and a reuse generation, so a
 * released handle stays distinguishable from every later handle. Passing a
 * released id, an id of the wrong type, or a value this library never issued
 * reports MLN_STATUS_INVALID_ARGUMENT and leaves the call without effect;
 * mln_thread_last_error_message() distinguishes the cases. Handle values are
 * safe to copy, compare, hash, and move between threads, and carry no
 * ownership on their own.
 *
 * The bit layout is internal. Hosts pass handles back as issued rather than
 * decoding or synthesizing them.
 */
typedef uint64_t mln_runtime;
typedef uint64_t mln_map;
typedef uint64_t mln_map_projection;
typedef uint64_t mln_offline_region_snapshot;
typedef uint64_t mln_offline_region_list;
typedef uint64_t mln_buffer;
typedef uint64_t mln_resource_request_handle;
typedef uint64_t mln_render_session;
typedef uint64_t mln_operation;
typedef uint64_t mln_notification_source;
typedef uint64_t mln_event_batch;
typedef uint64_t mln_ready_batch;
typedef uint64_t mln_adapter_resource_request_queue;
typedef uint64_t mln_adapter_log_queue;
typedef uint64_t mln_acquired_frame;
typedef uint64_t mln_render_frame_batch;

/**
 * Borrowed data. The data pointer may be null only when size is zero.
 *
 * Each parameter documents whether its view contains UTF-8 text, serialized
 * data, or arbitrary bytes. The view carries no ownership and requires no
 * trailing null byte.
 */
typedef struct mln_buffer_view {
  const void* data;
  size_t size;
} mln_buffer_view;

/**
 * Borrows the data stored by an owned buffer.
 *
 * The view remains valid until buffer is destroyed. The caller must not access
 * it concurrently with mln_buffer_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when out_view receives the borrowed view.
 * - MLN_STATUS_INVALID_ARGUMENT when buffer is null or not live, buffer has the
 *   wrong handle type, or out_view is null.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status
mln_buffer_get(mln_buffer buffer, mln_buffer_view* out_view) MLN_NOEXCEPT;

/** Destroys an owned buffer. A null handle is a no-op. */
MLN_API void mln_buffer_destroy(mln_buffer buffer) MLN_NOEXCEPT;

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
MLN_API uint32_t mln_supported_render_backend_mask(void) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_BASE_H
