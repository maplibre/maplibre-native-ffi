// The runtime that the generated sources call: status errors, and completions
// that return a GTask on the main context that started it.

#ifndef MAPLIBRE_NATIVE_FFI_PRIVATE_H
#define MAPLIBRE_NATIVE_FFI_PRIVATE_H

#include <gio/gio.h>
#include <maplibre_native_c.h>

G_BEGIN_DECLS

/** A failed status as an error of the library's domain. */
G_GNUC_INTERNAL GError* maplibre_native_ffi_error_new(
  mln_status status, const mln_diagnostic* diagnostic
);

/**
 * Whether a call succeeded. A failed status sets `error` from the call's
 * diagnostic.
 */
G_GNUC_INTERNAL gboolean maplibre_native_ffi_check(
  mln_status status, const mln_diagnostic* diagnostic, GError** error
);

/**
 * Writes a completion that returns `task` on the task's main context.
 *
 * The completion's delivery queue belongs to that context, which dispatches it
 * when native code queues a call. A successful completion returns TRUE, and a
 * failed one returns its status as an error. The completion holds a reference
 * to `task` until native code releases it.
 */
G_GNUC_INTERNAL gboolean maplibre_native_ffi_completion_init(
  mln_completion* completion, GTask* task, GError** error
);

/**
 * Frees a completion that native code rejected, which it never calls or
 * releases.
 */
G_GNUC_INTERNAL void maplibre_native_ffi_completion_reject(
  const mln_completion* completion
);

G_END_DECLS

#endif
