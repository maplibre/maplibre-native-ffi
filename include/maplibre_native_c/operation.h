/**
 * @file maplibre_native_c/operation.h
 * Common asynchronous operation API.
 *
 * This header targets C23.
 */

#ifndef MAPLIBRE_NATIVE_C_OPERATION_H
#define MAPLIBRE_NATIVE_C_OPERATION_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c/base.h"          // IWYU pragma: export
#include "maplibre_native_c/notification.h"  // IWYU pragma: export

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTBEGIN(modernize-use-using,modernize-use-trailing-return-type)

/**
 * Reports whether an operation has reached a terminal disposition.
 *
 * Completion is permanent. This function may be called from any thread.
 */
MLN_API mln_status
mln_operation_poll(mln_operation operation, bool* out_completed) MLN_NOEXCEPT;

/**
 * Waits for an operation to reach a terminal disposition.
 *
 * A negative timeout waits without a deadline, zero performs a nonblocking
 * check, and a positive timeout waits for at most that many milliseconds.
 * out_completed reports whether completion occurred before the deadline. This
 * function may be called from any thread and while another thread releases its
 * own copy of the handle value.
 */
MLN_API mln_status mln_operation_wait(
  mln_operation operation, int64_t timeout_ms, bool* out_completed
) MLN_NOEXCEPT;

/**
 * Requests cancellation.
 *
 * A supported request completes the operation with MLN_STATUS_CANCELLED.
 * MLN_STATUS_UNSUPPORTED leaves an uncancellable operation pending. A request
 * after completion returns MLN_STATUS_INVALID_STATE. This function may be
 * called from any thread.
 */
MLN_API mln_status mln_operation_cancel(mln_operation operation) MLN_NOEXCEPT;

/**
 * Copies the terminal status of a completed operation.
 *
 * A pending operation returns MLN_STATUS_INVALID_STATE. The function's own
 * return value reports inspection failure; out_status receives the operation's
 * terminal status, including MLN_STATUS_CANCELLED or another non-OK status.
 */
MLN_API mln_status mln_operation_get_status(
  mln_operation operation, mln_status* out_status
) MLN_NOEXCEPT;

/**
 * Copies the completed operation's diagnostic bytes.
 *
 * The bytes are UTF-8 and are not null-terminated. A null out_diagnostic with a
 * zero capacity is a size probe. out_diagnostic_size receives the required
 * length before capacity is checked. A pending operation returns
 * MLN_STATUS_INVALID_STATE.
 */
MLN_API mln_status mln_operation_copy_diagnostic(
  mln_operation operation, char* out_diagnostic, size_t diagnostic_capacity,
  size_t* out_diagnostic_size
) MLN_NOEXCEPT;

/**
 * Discards an untaken result from a completed operation.
 *
 * The operation remains live for status and diagnostic inspection. A pending
 * operation or an operation whose result was already taken or discarded
 * returns MLN_STATUS_INVALID_STATE.
 */
MLN_API mln_status
mln_operation_discard_result(mln_operation operation) MLN_NOEXCEPT;

/**
 * Releases the public observer for an operation.
 *
 * A null or already released handle is a no-op. Releasing a pending operation
 * requests cancellation when cancellation is supported, detaches its
 * notification endpoint, and lets internal work finish without publishing a
 * result. Releasing a completed operation destroys an untaken result. This
 * function may be called from any thread.
 */
MLN_API void mln_operation_release(mln_operation operation) MLN_NOEXCEPT;

// NOLINTEND(modernize-use-using,modernize-use-trailing-return-type)

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_OPERATION_H
