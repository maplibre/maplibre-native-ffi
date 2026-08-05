/**
 * @file maplibre_native_c/diagnostics.h
 * Public C API declarations for diagnostics.
 */

#ifndef MAPLIBRE_NATIVE_C_DIAGNOSTICS_H
#define MAPLIBRE_NATIVE_C_DIAGNOSTICS_H

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Returns the last thread-local diagnostic message.
 *
 * The returned string is empty when no diagnostic is available. The pointer is
 * owned by the C API and remains valid until the next C API call on the same
 * thread that writes a thread-local diagnostic.
 */
MLN_API const char* mln_thread_last_error_message(void) MLN_NOEXCEPT;

/**
 * Returns an opaque token identifying the calling native thread.
 *
 * The token is stable for the life of the thread and differs between live
 * threads. It carries no meaning beyond comparison, and a token from a thread
 * that has exited may be reused by a later one.
 *
 * Owner-thread checks in this API key on the native thread. A host whose unit
 * of execution is not pinned to one, such as a Dart isolate or an unlocked
 * goroutine, compares this token across calls to detect that it moved.
 *
 * Callable from any thread.
 */
MLN_API uint64_t mln_thread_token(void) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_DIAGNOSTICS_H
