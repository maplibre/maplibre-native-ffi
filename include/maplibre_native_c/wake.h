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

/** Releases wake callback state after native code can no longer invoke it. */
typedef void (*mln_wake_release)(void* user_data);

/**
 * Receiver wake callback copied by a successful owning call.
 *
 * Native code may invoke callback from any thread, and calls may coalesce or
 * overlap. The callback should only schedule receiver work and return. It must
 * not destroy the object that owns the wake. Native code calls
 * release_user_data after all callback invocations have returned.
 *
 * A descriptor whose callback is null disables waking; size must still be
 * sizeof(mln_wake), and a disabled wake must not carry release_user_data.
 * Polling remains valid when the owning API permits an omitted wake.
 */
typedef struct mln_wake {
  uint32_t size;
  mln_wake_callback callback;
  void* user_data;
  mln_wake_release release_user_data;
} mln_wake;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_WAKE_H
