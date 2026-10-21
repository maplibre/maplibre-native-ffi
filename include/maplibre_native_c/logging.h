/**
 * @file maplibre_native_c/logging.h
 * Public C API declarations for logging.
 */

#ifndef MAPLIBRE_NATIVE_C_LOGGING_H
#define MAPLIBRE_NATIVE_C_LOGGING_H

#include <stdint.h>

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Log severity values emitted by MapLibre Native. */
typedef enum mln_log_severity : uint32_t {
  MLN_LOG_SEVERITY_INFO = 1,
  MLN_LOG_SEVERITY_WARNING = 2,
  MLN_LOG_SEVERITY_ERROR = 3,
} mln_log_severity;

/** Bitmask values for log severities dispatched asynchronously. */
typedef enum MLN_BINDING("kind=bitmask") mln_log_severity_mask : uint32_t {
  MLN_LOG_SEVERITY_MASK_INFO = 1U << MLN_LOG_SEVERITY_INFO,
  MLN_LOG_SEVERITY_MASK_WARNING = 1U << MLN_LOG_SEVERITY_WARNING,
  MLN_LOG_SEVERITY_MASK_ERROR = 1U << MLN_LOG_SEVERITY_ERROR,
  MLN_LOG_SEVERITY_MASK_DEFAULT = MLN_LOG_SEVERITY_MASK_INFO |
    MLN_LOG_SEVERITY_MASK_WARNING,
  MLN_LOG_SEVERITY_MASK_ALL = MLN_LOG_SEVERITY_MASK_INFO |
    MLN_LOG_SEVERITY_MASK_WARNING | MLN_LOG_SEVERITY_MASK_ERROR,
} mln_log_severity_mask;

/** Log event categories emitted by MapLibre Native. */
typedef enum mln_log_event : uint32_t {
  MLN_LOG_EVENT_GENERAL = 0,
  MLN_LOG_EVENT_SETUP = 1,
  MLN_LOG_EVENT_SHADER = 2,
  MLN_LOG_EVENT_PARSE_STYLE = 3,
  MLN_LOG_EVENT_PARSE_TILE = 4,
  MLN_LOG_EVENT_RENDER = 5,
  MLN_LOG_EVENT_STYLE = 6,
  MLN_LOG_EVENT_DATABASE = 7,
  MLN_LOG_EVENT_HTTP_REQUEST = 8,
  MLN_LOG_EVENT_SPRITE = 9,
  MLN_LOG_EVENT_IMAGE = 10,
  MLN_LOG_EVENT_GRAPHICS_BACKEND = 11,
  MLN_LOG_EVENT_JNI = 12,
  MLN_LOG_EVENT_ANDROID = 13,
  MLN_LOG_EVENT_CRASH = 14,
  MLN_LOG_EVENT_GLYPH = 15,
  MLN_LOG_EVENT_TIMING = 16,
} mln_log_event;

/**
 * Receives a MapLibre Native log record.
 *
 * The message pointer is borrowed for the callback duration. Returning non-zero
 * consumes the record. Returning zero lets MapLibre Native's platform logger
 * handle it.
 *
 * A handler in queue mode consumes each record without a callback and queues
 * a copy as an mln_log_delivery.
 */
MLN_BINDING("failure=0;delivery=MLN_DELIVERY_TYPE_LOG_RECORD")
typedef uint32_t (*mln_log_callback)(
  void* user_data, uint32_t severity MLN_BINDING("enum=mln_log_severity"),
  uint32_t event MLN_BINDING("enum=mln_log_event"), int64_t code,
  const char* message
);

/**
 * Process-global log callback or queue state.
 *
 * The struct itself is borrowed for mln_log_set_callback(). Exactly one of
 * callback and queue is set.
 */
typedef struct mln_log_handler {
  uint32_t size;
  /**
   * Receives each record.
   *
   * Required in callback mode; null in queue mode.
   */
  mln_log_callback callback;
  /** Opaque host token in queue mode; native code never dereferences it. */
  void* user_data MLN_BINDING("kind=context");
  /**
   * Optional. Releases user_data after the final callback returns, when the
   * handler is replaced or cleared.
   *
   * It runs on the thread that replaces or clears the handler, under the
   * same restrictions as the callback. Null in queue mode.
   */
  mln_user_data_release release_user_data;
  /**
   * Optional. Queue delivery: native code consumes each record and queues a
   * copy as an MLN_DELIVERY_TYPE_LOG_RECORD delivery carrying user_data. A
   * record the queue refuses, because the queue was released or the copy
   * could not be allocated, goes to the platform logger instead. Replacing or
   * clearing the handler queues one MLN_DELIVERY_TYPE_RETIRED delivery carrying
   * user_data after the handler's final record. The queue wakes inline under
   * the log mutex. Its wake and wake release callback follow the same
   * restrictions as the log callback.
   */
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_log_handler MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/**
 * Installs a process-global MapLibre Native log callback.
 *
 * MLN_STATUS_OK replaces the current handler and transfers the handler's
 * callback, user_data, and release_user_data to the C API. The handler struct
 * itself is borrowed for the call. With a null release_user_data, the caller
 * keeps responsibility for user_data and must keep it valid until the handler
 * is replaced or cleared. A rejected call leaves the current handler in place
 * and invokes no release. mln_log_clear_callback() clears the handler.
 *
 * The callback may run on any thread, according to the async severity mask. It
 * must be thread-safe and return promptly. MapLibre Native holds its log mutex,
 * which is not recursive, while the callback runs, and nearly every native call
 * can log. So the callback must not call this C API or MapLibre Native APIs.
 * Each status-returning function of this API returns MLN_STATUS_INVALID_STATE
 * when the callback calls it.
 *
 * In queue mode the handler's records reach the queue and its wake runs inline
 * on the logging thread. The wake follows the same restrictions.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when handler is null, handler->size is too
 *   small, neither or both of callback and queue are set, release_user_data
 *   is set in queue mode, or queue is an invalid handle.
 * - MLN_STATUS_INVALID_STATE when queue has been released.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_log_set_callback(
  const mln_log_handler* handler, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Clears the process-global log callback.
 *
 * The C API invokes the cleared handler's release_user_data, if any, or queues
 * its MLN_DELIVERY_TYPE_RETIRED delivery, before returning.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status
mln_log_clear_callback(mln_diagnostic* out_diagnostic) MLN_NOEXCEPT;

/**
 * Controls which log severities MapLibre Native may dispatch asynchronously.
 *
 * MLN_LOG_SEVERITY_MASK_DEFAULT restores MapLibre Native's default behavior:
 * info and warning records may be asynchronous, while error records remain
 * synchronous.
 *
 * A severity outside the mask invokes the log callback synchronously on the
 * thread that emits the record, before the logging call returns.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when mask contains unknown bits.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_log_set_async_severity_mask(
  uint32_t mask MLN_BINDING("enum=mln_log_severity_mask"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_LOGGING_H
