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
 * Native code may invoke callback from any thread, including a MapLibre Native
 * thread, and calls may coalesce or overlap. The invoking thread can hold
 * internal locks of MapLibre Native or of this library, such as the lock of the
 * mailbox that it posts work to. Work that the owner runs on another thread can
 * wait for those locks, as driver service does when its work closes that
 * mailbox. The callback therefore must not wait for another thread, either
 * directly or through a host lock that the other thread holds while it calls
 * this library. A callback that dispatches synchronously to the graphics
 * thread deadlocks when driver service on that thread waits for a lock that the
 * invoking thread holds.
 *
 * A callback may call the owner's functions that return without waiting, such
 * as one that drains its queue. On the graphics thread, a callback may also
 * service its own caller-driver session inline. A service of that session that
 * already runs further up the stack returns MLN_STATUS_BUSY, and the running
 * service takes the new work. Inline service nests, though. A service that
 * stops with work still queued raises the wake again before it returns, so a
 * small max_work adds one service to the stack for each wake. A wake for one
 * session can also arrive inside the service of another session on the same
 * thread, where an inline service renders inside that other session's driver
 * call. A host that drives several sessions on one thread, or that limits
 * max_work, schedules the service instead.
 *
 * The callback must not destroy the object that owns the wake. Native code
 * calls release_user_data after all callback invocations have returned. It can
 * call release_user_data on any thread that invoked callback, under the same
 * locks, so release_user_data also returns without waiting for another thread.
 *
 * A descriptor whose callback is null disables waking, and a disabled wake
 * must not carry release_user_data.
 * When the owning API permits an omitted wake, the receiver services its queue
 * or driver on its own schedule instead.
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
