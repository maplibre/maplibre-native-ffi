/**
 * @file maplibre_native_c/binding_support.h
 * Binding-internal C declarations for this repository's generated bindings.
 *
 * Finalizer tokens and shutdown abandonment tie native handles to a host
 * runtime's memory management. A Dart wake posts to a Dart isolate, which then
 * drains its delivery queue or services the woken session. Hosts that write
 * their own integration use the public headers, including queue delivery in
 * delivery.h for every callback a host need not answer on a native thread.
 *
 * This header is not installed and carries no compatibility promise. Bindings
 * may resolve its exported functions from the shipped library.
 *
 * This header targets C23.
 */

#ifndef MAPLIBRE_NATIVE_C_BINDING_SUPPORT_H
#define MAPLIBRE_NATIVE_C_BINDING_SUPPORT_H

#include <stddef.h>
#include <stdint.h>

#include "base.h"            // IWYU pragma: export
#include "render_session.h"  // IWYU pragma: export
#include "wake.h"            // IWYU pragma: export

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTBEGIN(modernize-use-using,modernize-use-trailing-return-type)

/**
 * Creates a finalizer token for one owned native handle.
 *
 * Ownership transfers to the token on entry. Explicit release destroys the
 * token; finalization disposes the native owner. If token allocation fails,
 * the function disposes the owner and returns null. Allocation failure during
 * disposal terminates the process.
 */
MLN_BINDING("ownership=owned")
MLN_API void* mln_binding_owner_token_create(uint64_t handle) MLN_NOEXCEPT;

/** Releases a finalizer token after explicit owner release. */
MLN_API void mln_binding_owner_token_destroy(
  void* token MLN_BINDING("kind=context")
) MLN_NOEXCEPT;

/**
 * Disposes the token's native owner and releases the token on any thread.
 *
 * A render session's retirement keeps its graphics resources until process
 * exit and makes no graphics call after the one in flight. Other behavior
 * follows mln_render_session_dispose().
 */
MLN_API void mln_binding_owner_finalize(
  void* token MLN_BINDING("kind=context")
) MLN_NOEXCEPT;

/**
 * Abandons a render session during a host runtime's shutdown, keeping its
 * graphics objects.
 *
 * Follows mln_render_session_abandon(), including its wait for an in-flight
 * core-worker driver call, but makes no graphics call and keeps every graphics
 * object until process exit. The host may call this after its runtime's
 * shutdown has destroyed the graphics objects behind the session.
 *
 * Returns the statuses that mln_render_session_abandon() returns.
 */
MLN_API mln_status mln_binding_render_session_abandon_at_exit(
  mln_render_session session,
  mln_render_abandon_result* out_result MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Creates a wake that posts integer messages through Dart native API version 2.
 *
 * post_cobject is NativeApi.postCObject and port is a Dart SendPort.nativePort.
 * Callback messages contain 0; release messages contain 1. Posting to a closed
 * port safely discards the message. The native release callback frees the
 * descriptor context after posting. A rejected owning call invokes that release
 * callback directly to release the context.
 *
 * Returns:
 * - MLN_STATUS_OK when out_wake receives the wake.
 * - MLN_STATUS_INVALID_ARGUMENT when post_cobject or out_wake is null, or port
 *   is zero.
 * - MLN_STATUS_NATIVE_ERROR when the context could not be allocated.
 */
MLN_API mln_status mln_binding_dart_wake_create(
  void* post_cobject MLN_BINDING("lifetime=process"), int64_t port,
  mln_wake* out_wake MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

// NOLINTEND(modernize-use-using,modernize-use-trailing-return-type)

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_BINDING_SUPPORT_H
