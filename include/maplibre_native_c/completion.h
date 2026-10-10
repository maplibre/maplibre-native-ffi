/**
 * @file maplibre_native_c/completion.h
 * Public C API declarations for one-shot asynchronous completion.
 */

#ifndef MAPLIBRE_NATIVE_C_COMPLETION_H
#define MAPLIBRE_NATIVE_C_COMPLETION_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Terminal dispositions reported by command completions. */
typedef enum mln_command_disposition : uint32_t {
  MLN_COMMAND_DISPOSITION_COMMITTED = 0,
  MLN_COMMAND_DISPOSITION_SUPERSEDED = 1,
  MLN_COMMAND_DISPOSITION_FAILED = 2,
  MLN_COMMAND_DISPOSITION_CANCELLED = 3,
} mln_command_disposition;

/**
 * Borrowed terminal result for one accepted asynchronous submission.
 *
 * The submitting function defines the type behind value and the meaning of
 * value_count. A function with no result completes with value null and
 * value_count zero, as does any failed completion. Every pointer remains valid
 * only for the completion callback. A binding copies any result or diagnostic
 * that it keeps.
 *
 * disposition and generation are meaningful for commands. Other one-shot
 * functions set disposition to MLN_COMMAND_DISPOSITION_COMMITTED and
 * generation to zero.
 *
 * A map command queues the runtime events it raises before its completion
 * runs, and those events carry the generation this result reports.
 */
typedef struct mln_completion_result {
  uint32_t size;
  /** Terminal status, one of mln_status. */
  int32_t status MLN_BINDING("enum=mln_status");
  /** One of mln_command_disposition. */
  uint32_t disposition MLN_BINDING("enum=mln_command_disposition");
  /**
   * Byte stride of one element of value: sizeof the result record in the
   * native build, at least sizeof in the caller's header. Zero when value is
   * null. Index array results with this value.
   */
  uint32_t value_size;
  /**
   * Map snapshot generation published after a map command ran, whether it
   * committed or failed; zero when the command never ran or the function is
   * not a command.
   */
  uint64_t generation;
  /** Borrowed diagnostic bytes, empty on success. */
  mln_buffer_view diagnostic;
  /** Borrowed function-specific result, or null when the function has none. */
  const void* value
    MLN_BINDING("kind=erased;length=value_count;stride=value_size");
  /** Function-specific element count for value. */
  size_t value_count;
} mln_completion_result;

/** Receives one terminal result for an accepted asynchronous submission. */
typedef void (*mln_completion_callback)(
  void* user_data, const mln_completion_result* result
);

/**
 * Callback state for one asynchronous submission.
 *
 * The struct itself is borrowed for the submission call. MLN_STATUS_OK
 * transfers callback, user_data, and release_user_data to the C API. The
 * completion runs exactly once and may run before the submission returns. The
 * release callback runs after the completion returns. A non-OK submission
 * invokes neither callback and leaves user_data owned by the caller.
 *
 * The completion runs on whichever thread finishes the work, including the
 * submitting thread for inline completion. It may call any-thread C APIs, but
 * must return promptly and must not wait for runtime or graphics-driver work
 * that can only progress on its current thread.
 */
typedef struct mln_completion {
  uint32_t size;
  mln_completion_callback callback;
  void* user_data MLN_BINDING("kind=context");
  /**
   * Optional. Releases user_data on the completing thread after the completion
   * returns.
   */
  mln_user_data_release release_user_data;
} mln_completion MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_COMPLETION_H
