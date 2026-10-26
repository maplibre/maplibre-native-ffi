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
  MLN_COMMAND_DISPOSITION_FAILED = 1,
  MLN_COMMAND_DISPOSITION_CANCELLED = 2,
} mln_command_disposition;

/**
 * Borrowed terminal result for one accepted asynchronous submission.
 *
 * The submitting function defines the type behind value and the meaning of
 * value_count. A function with no result completes with value null and
 * value_count zero, as does any failed completion. Every pointer remains valid
 * only for the completion callback.
 *
 * disposition and generation are meaningful for commands. Other one-shot
 * functions set disposition to MLN_COMMAND_DISPOSITION_COMMITTED and
 * generation to zero.
 *
 * A map command delivers the runtime events it raises before its completion
 * runs: a callback handler has returned from them, or a queue handler's queue
 * holds them. Those events carry the generation this result reports.
 */
typedef struct mln_completion_result {
  uint32_t size;
  /** Terminal status, one of mln_status. */
  mln_status status;
  mln_command_disposition disposition;
  /**
   * Byte stride of one element of value: sizeof the result record in the
   * native build, at least sizeof in the caller's header. Zero when value is
   * null. Index array results with this value.
   */
  uint32_t value_size;
  /**
   * Map snapshot generation published after a map command ran, whether it
   * committed or failed; zero when the command never ran or the function is
   * not a map command.
   */
  uint64_t generation;
  /**
   * Generation of the first map render update that includes the command. A
   * frame result with disposition MLN_RENDER_RESULT_RENDERED and a
   * map_update_generation at or past this value was rendered from map state
   * that includes the command. Inside a command group, this is the update that
   * the outermost group end publishes. Zero when generation is zero.
   */
  uint64_t render_update_generation;
  /** Borrowed diagnostic bytes, empty on success. */
  mln_buffer_view diagnostic;
  /** Borrowed function-specific result, or null when the function has none. */
  const void* value;
  /** Function-specific element count for value. */
  size_t value_count;
} mln_completion_result;

/**
 * Receives one terminal result for an accepted asynchronous submission.
 *
 * Threads:
 * The completion runs on an unspecified thread, which may be the submitting
 * thread.
 */
typedef void (*mln_completion_callback)(
  void* user_data, const mln_completion_result* result
);

/**
 * Callback or queue state for one asynchronous submission. The completion may
 * run, or its delivery be queued, before the submitting call returns.
 *
 * The struct itself is borrowed for the submission call. A non-null completion
 * sets exactly one of callback and queue. MLN_STATUS_OK transfers callback,
 * user_data, and release_user_data to the C API. The completion runs exactly
 * once. The release callback runs
 * after the completion returns. A non-OK submission invokes neither callback
 * and leaves user_data owned by the caller.
 *
 * A command, which the header marks MLN_BINDING("execution=command"), also
 * accepts a null completion, which discards its outcome and runs no callback.
 * The command otherwise behaves as it would with a completion. It returns the
 * same status and diagnostic, runs in acceptance order, and queues its events;
 * a map command also publishes its snapshot generation.
 * A discarded command that fails logs its status and diagnostic as a warning
 * through the log handler. Queries, operations, and lifecycle functions require
 * a completion, except mln_render_session_dispose(), which accepts null to
 * dispose without reporting retirement.
 *
 * A later completion fences discarded commands. The completion of a later
 * command or query on the same map follows every earlier discarded command,
 * and a map snapshot read after it observes them. The completion of a later
 * command on the same runtime, such as a registration change, follows every
 * earlier discarded command of that runtime and its maps. A render session
 * barrier fences only the session's applied extent after a discarded resize;
 * the resize completion fences both the session and the map. A snapshot
 * generation alone does not, because other work also publishes generations.
 *
 * The completion thread is unspecified and may be the submitting thread. It
 * can differ from the thread that ran the command. The completion may call
 * any-thread C APIs, but must return promptly and must not wait for runtime or
 * graphics-driver work that can only progress on its current thread.
 *
 * In queue mode, no callback runs. The terminal result reaches queue exactly
 * once as an MLN_DELIVERY_TYPE_COMPLETION delivery, unless the host releases
 * the queue before the result arrives, which discards it. See delivery.h.
 * Where another function waits for a completion and its release to return, a
 * queue-mode completion has returned once native has pushed its result and
 * woken the queue, or has discarded the result because the queue was released
 * or the process is exiting. Draining the queue is the host's step.
 */
typedef struct mln_completion {
  uint32_t size;
  /**
   * Receives the terminal result.
   *
   * Required in callback mode; null in queue mode.
   */
  mln_completion_callback callback;
  /**
   * Opaque host token in queue mode; native code never dereferences or releases
   * it. Keep token state until delivery is handled or the queue is abandoned.
   */
  void* user_data MLN_BINDING("kind=context");
  /**
   * Optional. Releases user_data on the completing thread after the completion
   * returns.
   *
   * Null in queue mode.
   */
  mln_user_data_release release_user_data;
  /**
   * Optional. Queue delivery: the terminal result is copied into one
   * MLN_DELIVERY_TYPE_COMPLETION delivery carrying user_data, and no callback
   * runs. Requires callback and release_user_data null. The delivery may be
   * queued before the submission returns. A rejected submission queues nothing
   * and needs no cleanup. A submission rejects an invalid queue handle with
   * MLN_STATUS_INVALID_ARGUMENT and a released queue with
   * MLN_STATUS_INVALID_STATE.
   */
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_completion MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_COMPLETION_H
