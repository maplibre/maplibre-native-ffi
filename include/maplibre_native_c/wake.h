/**
 * @file maplibre_native_c/wake.h
 * Public C API declarations for receiver wake callbacks.
 */

#ifndef MAPLIBRE_NATIVE_C_WAKE_H
#define MAPLIBRE_NATIVE_C_WAKE_H

#include <stdint.h>

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Schedules service by the receiver that owns a queue or driver. */
typedef void (*mln_wake_callback)(void* user_data);

/**
 * Receiver wake callback copied by a successful owning call.
 *
 * Native code may invoke callback from any thread, and calls may coalesce or
 * overlap. Native code holds none of the owner's internal locks while it
 * invokes callback, so a callback may call the owner's functions, such as one
 * that drains its queue. The callback should still only schedule receiver work
 * and return, because it can run on a thread that is doing native work. It must
 * not destroy the object that owns the wake. Native code calls
 * release_user_data after all callback invocations have returned.
 *
 * A descriptor whose callback is null disables waking; size must still be
 * sizeof(mln_wake), and a disabled wake must not carry release_user_data.
 * When the owning API permits an omitted wake, the receiver services its queue
 * or driver on its own schedule instead.
 */
typedef struct mln_wake {
  uint32_t size;
  mln_wake_callback callback MLN_BINDING("nullable=true");
  void* user_data MLN_BINDING("kind=context");
  /**
   * Optional. Releases user_data after all callback invocations have returned.
   * A disabled wake leaves it null.
   */
  mln_user_data_release release_user_data;
} mln_wake MLN_BINDING("kind=callback_registration;release=release_user_data");

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_WAKE_H
