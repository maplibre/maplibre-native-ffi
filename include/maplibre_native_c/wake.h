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
 * Receiver wake callback copied by a successful owning call. The callback
 * schedules service by the receiver and returns without waiting for another
 * thread.
 *
 * Native code may invoke callback from any thread, and calls may coalesce or
 * overlap. The callback must not wait for another thread, directly or through
 * a host lock that the other thread holds while it calls this library.
 *
 * A callback may call the owner's functions that return without waiting, such
 * as one that drains its queue. On the graphics thread, a callback may also
 * service its own caller-driver session inline. A service of that session that
 * already runs further up the stack returns MLN_STATUS_BUSY, and the running
 * service takes the new work. When several sessions share a graphics thread,
 * the host schedules service after the callback returns to keep calls from
 * nesting across sessions.
 *
 * The callback may release a delivery queue that owns its wake. For every
 * other owner, the callback must not destroy the object that owns the wake.
 * Native code calls release_user_data after all callback invocations have
 * returned. It may
 * run on any thread that invoked callback and must not wait for another thread.
 *
 * A descriptor whose callback is null disables waking, and a disabled wake
 * must not carry release_user_data. When the owning API permits a disabled
 * wake, the receiver services its queue or driver on its own schedule instead.
 */
typedef struct mln_wake {
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
