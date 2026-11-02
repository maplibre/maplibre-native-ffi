/**
 * @file maplibre_native_c/logging.h
 * Log messages from the library.
 */

#ifndef MAPLIBRE_NATIVE_C_LOGGING_H
#define MAPLIBRE_NATIVE_C_LOGGING_H

#include <stdint.h>

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** The severity of a log message. */
typedef enum mln_log_severity : uint32_t {
  MLN_LOG_SEVERITY_INFO = 1,
  MLN_LOG_SEVERITY_WARNING = 2,
  MLN_LOG_SEVERITY_ERROR = 3,
} mln_log_severity;

/** A log severity, as a bit in a set. */
typedef enum MLN_BINDING("kind=bitmask") mln_log_severity_flag : uint32_t {
  MLN_LOG_SEVERITY_FLAG_INFO = 1U << MLN_LOG_SEVERITY_INFO,
  MLN_LOG_SEVERITY_FLAG_WARNING = 1U << MLN_LOG_SEVERITY_WARNING,
  MLN_LOG_SEVERITY_FLAG_ERROR = 1U << MLN_LOG_SEVERITY_ERROR,
  /**
   * The severities logged on a background thread until
   * mln_log_set_async_severity_mask() changes them.
   */
  MLN_LOG_SEVERITY_FLAG_DEFAULT = MLN_LOG_SEVERITY_FLAG_INFO |
    MLN_LOG_SEVERITY_FLAG_WARNING,
  MLN_LOG_SEVERITY_FLAG_ALL = MLN_LOG_SEVERITY_FLAG_INFO |
    MLN_LOG_SEVERITY_FLAG_WARNING | MLN_LOG_SEVERITY_FLAG_ERROR,
} mln_log_severity_flag;

/** A set of mln_log_severity_flag bits. */
typedef uint32_t mln_log_severity_flags;

/** The part of the library that logged a message. */
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
 * Receives a log message.
 *
 * Without an mln_delivery_queue, return true if you handled the message, or
 * false to also send it to the platform's log. With an mln_delivery_queue,
 * the platform's log never gets the message.
 *
 * Threads:
 * Runs on the thread that logged the message, or on a background thread for the
 * severities that mln_log_set_async_severity_mask() selects. The callback can't
 * call the library.
 *
 * Lifetimes:
 * `message` stays valid until the callback returns.
 */
MLN_BINDING("failure=false;queue_result=true")
typedef bool (*mln_log_callback)(
  void* user_data, mln_log_severity severity, mln_log_event event, int64_t code,
  mln_buffer_view message
);

/** A log callback, for mln_log_set_callback(). */
typedef struct mln_log_handler {
  uint32_t size;
  mln_log_callback callback;
  void* user_data MLN_BINDING("kind=context");
  mln_user_data_release release_user_data;
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_log_handler MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/**
 * Sends the log messages of every runtime in the process to a callback.
 *
 * The callback replaces the one set before.
 */
MLN_API mln_status mln_log_set_callback(
  const mln_log_handler* handler, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Sends log messages to the platform's log again, instead of a callback. */
MLN_API mln_status
mln_log_clear_callback(mln_diagnostic* out_diagnostic) MLN_NOEXCEPT;

/**
 * Selects the severities that the library logs on a background thread.
 *
 * The library logs other severities on the thread that logged them. `mask`
 * applies to every runtime in the process.
 */
MLN_API mln_status mln_log_set_async_severity_mask(
  mln_log_severity_flags mask, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_LOGGING_H
