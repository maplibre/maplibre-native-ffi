/**
 * @file maplibre_native_c/binding_support.h
 * Binding-internal C declarations for this repository's generated bindings.
 *
 * Allocation arenas, finalizer tokens, Dart release registration, and shutdown
 * abandonment let a generated binding tie native state to its host runtime's
 * memory management. Hosts that write their own integration use the public
 * headers, and callback_adapter.h where native callback threads are a problem.
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

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTBEGIN(modernize-use-using,modernize-use-trailing-return-type)

/** Creates a zero-initialized native allocation arena, or returns null. */
MLN_BINDING("ownership=owned")
MLN_API void* mln_adapter_arena_create(void) MLN_NOEXCEPT;

/** Allocates aligned zeroed memory that belongs to the arena. */
MLN_API void* mln_adapter_arena_allocate(
  void* arena MLN_BINDING("kind=context"), size_t size, size_t alignment
) MLN_NOEXCEPT;

/** Releases the arena's allocations and disposes its adopted handles. */
MLN_API void mln_adapter_arena_destroy(
  void* arena MLN_BINDING("kind=context")
) MLN_NOEXCEPT;

/** Transfers a handle on entry; failure disposes the handle immediately. */
MLN_API mln_status mln_adapter_arena_adopt_handle(
  void* arena MLN_BINDING("kind=context"), uint64_t handle,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Transfers a context release on entry; failure runs the release immediately.
 *
 * The arena calls release with context once when it is destroyed, before it
 * frees its allocations.
 */
MLN_API mln_status mln_adapter_arena_adopt_release(
  void* arena MLN_BINDING("kind=context"), mln_user_data_release release,
  void* context MLN_BINDING("kind=context"), mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Registers a retained callback context and consumes its arena on entry.
 *
 * Native release destroys the arena even if the Dart isolate has closed.
 * The release message contains a unique registration identifier. Identifiers
 * remain distinct when a later arena reuses the same context address.
 */
MLN_API mln_status mln_adapter_dart_release_register(
  void* post_cobject MLN_BINDING("lifetime=process"), int64_t port,
  void* context MLN_BINDING("kind=context"),
  void* arena MLN_BINDING("kind=context;nullable=true"),
  uint64_t* out_registration MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Removes a registration, posts its identifier and releases its native arena.
 */
MLN_API void mln_adapter_dart_release(
  void* context MLN_BINDING("kind=context")
) MLN_NOEXCEPT;

/**
 * Creates a finalizer token for one owned native handle.
 *
 * Ownership transfers to the token on entry. Explicit release destroys the
 * token; finalization disposes the native owner. Allocation failure disposes
 * the owner and returns null.
 */
MLN_BINDING("ownership=owned")
MLN_API void* mln_adapter_owner_token_create(uint64_t handle) MLN_NOEXCEPT;

/** Releases a finalizer token after explicit owner release. */
MLN_API void mln_adapter_owner_token_destroy(
  void* token MLN_BINDING("kind=context")
) MLN_NOEXCEPT;

/**
 * Disposes the token's native owner and releases the token on any thread.
 *
 * A render session's retirement keeps its graphics resources until process
 * exit and makes no graphics call after the one in flight. Other behavior
 * follows mln_render_session_dispose().
 */
MLN_API void mln_adapter_owner_finalize(
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
MLN_API mln_status mln_adapter_render_session_abandon_at_exit(
  mln_render_session session,
  mln_render_abandon_result* out_result MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

// NOLINTEND(modernize-use-using,modernize-use-trailing-return-type)

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_BINDING_SUPPORT_H
