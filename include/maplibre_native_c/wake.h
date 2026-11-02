/**
 * @file maplibre_native_c/wake.h
 * Learn when the library has work ready for the caller.
 */

#ifndef MAPLIBRE_NATIVE_C_WAKE_H
#define MAPLIBRE_NATIVE_C_WAKE_H

#include <stdint.h>

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Tells the caller that work is ready for it.
 *
 * One call can stand for several pieces of ready work, so take all of it each
 * time.
 *
 * Threads:
 * Can run on any thread, including the caller's own thread during a library
 * call. Return quickly and don't block.
 */
typedef void (*mln_wake_callback)(void* user_data);

/** A callback that tells the caller when work is ready. */
typedef struct mln_wake {
  /** Null to turn the wake off. */
  mln_wake_callback callback MLN_BINDING("nullable=true");
  void* user_data MLN_BINDING("kind=context");
  mln_user_data_release release_user_data;
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_wake MLN_BINDING("kind=callback_registration;release=release_user_data");

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_WAKE_H
