/**
 * @file maplibre_native_c/completion.h
 * Receive the result of an asynchronous function.
 */

#ifndef MAPLIBRE_NATIVE_C_COMPLETION_H
#define MAPLIBRE_NATIVE_C_COMPLETION_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Whether a command's change was applied. */
typedef enum mln_command_disposition : uint32_t {
  MLN_COMMAND_DISPOSITION_COMMITTED = 0,
  MLN_COMMAND_DISPOSITION_FAILED = 1,
  MLN_COMMAND_DISPOSITION_CANCELLED = 2,
} mln_command_disposition;

/**
 * The result of an asynchronous function.
 *
 * Lifetimes:
 * `diagnostic` and `value` stay valid until the callback returns. `value`
 * points at `value_count` results of the type that the function's annotation
 * names, each `value_size` bytes apart, so read them with MLN_STRIDED_AT().
 * `value` is null when there is no result.
 */
typedef struct mln_completion_result {
  uint32_t size;
  mln_status status;
  mln_command_disposition disposition;
  uint32_t value_size;
  /**
   * For a map command, the first mln_map_snapshot generation that includes the
   * change.
   */
  uint64_t generation;
  /**
   * For a map command, the first mln_render_frame_result map update generation
   * that includes the change.
   */
  uint64_t render_update_generation;
  /** A message that explains a failure. Empty on success. */
  mln_buffer_view diagnostic;
  const void* value;
  size_t value_count;
} mln_completion_result;

/**
 * Receives the result of an asynchronous function.
 *
 * Lifetimes:
 * `result` stays valid until the callback returns.
 */
typedef void (*mln_completion_callback)(
  void* user_data, const mln_completion_result* result
);

/**
 * The callback that receives an asynchronous function's result.
 *
 * Lifetimes:
 * When the function returns MLN_STATUS_OK, the callback runs once, and it can
 * run before the function returns. When the function fails, the callback
 * never runs. A command also accepts a null `completion` when the caller
 * doesn't need its result.
 */
typedef struct mln_completion {
  uint32_t size;
  mln_completion_callback callback;
  void* user_data MLN_BINDING("kind=context");
  mln_user_data_release release_user_data;
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_completion MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_COMPLETION_H
