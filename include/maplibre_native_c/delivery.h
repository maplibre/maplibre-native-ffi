/**
 * @file maplibre_native_c/delivery.h
 * Run callbacks on a thread that the caller chooses.
 */

#ifndef MAPLIBRE_NATIVE_C_DELIVERY_H
#define MAPLIBRE_NATIVE_C_DELIVERY_H

#include <stdint.h>

#include "base.h"  // IWYU pragma: export
#include "wake.h"  // IWYU pragma: export

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTBEGIN(modernize-use-using,modernize-use-trailing-return-type)

/** Options for mln_delivery_queue_create(). */
typedef struct mln_delivery_queue_options {
  uint32_t size;
  /**
   * Tells the caller when the empty queue gains calls. Dispatch the queue after
   * each wake.
   */
  mln_wake wake;
  /**
   * Runs in place of a registration's release function when the queue is
   * released before that function can run.
   */
  mln_user_data_release release_abandoned_user_data;
} mln_delivery_queue_options;

/**
 * Creates a queue that holds callback calls until mln_delivery_queue_dispatch()
 * runs them.
 */
MLN_API mln_status mln_delivery_queue_create(
  const mln_delivery_queue_options* options,
  mln_delivery_queue* out_queue MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases a queue.
 *
 * The calls in the queue, and any that registrations would add later, never
 * run.
 */
MLN_API void mln_delivery_queue_release(mln_delivery_queue queue) MLN_NOEXCEPT;

/**
 * Runs the calls in the queue on the calling thread.
 *
 * When the queue is empty, it succeeds and runs nothing.
 *
 * Returns:
 * - MLN_STATUS_BUSY when another dispatch of the queue is running. That
 *   dispatch runs the calls instead.
 */
MLN_API mln_status mln_delivery_queue_dispatch(
  mln_delivery_queue queue, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

// NOLINTEND(modernize-use-using,modernize-use-trailing-return-type)

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_DELIVERY_H
