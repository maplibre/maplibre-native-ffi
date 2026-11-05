// The runtime that the generated sources call: status errors, text and bytes,
// the scope of one native call, the closures of callback registrations, and
// completions that return a GTask or ready a handle on the main context that
// started them.

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

/** Sets `error` for a call on a handle that a callback only borrows. */
G_GNUC_INTERNAL void maplibre_native_ffi_set_borrowed_error(GError** error);

/** A copy of native text, which is empty when `view` is. */
G_GNUC_INTERNAL char* maplibre_native_ffi_text_new(mln_buffer_view view);

/** A copy of native text of `size` bytes, which is empty when `data` is null. */
G_GNUC_INTERNAL char* maplibre_native_ffi_text_new_sized(
  const void* data, size_t size
);

/** The view of `text`, which is empty when `text` is null. */
G_GNUC_INTERNAL mln_buffer_view maplibre_native_ffi_text_view(const char* text);

/** A copy of native bytes, or null when `data` is. */
G_GNUC_INTERNAL GBytes* maplibre_native_ffi_bytes_new(
  const void* data, size_t size
);

/** The view of `bytes`, which is empty when `bytes` is null. */
G_GNUC_INTERNAL mln_buffer_view maplibre_native_ffi_bytes_view(GBytes* bytes);

/** Whether two byte sequences, either of which may be null, are equal. */
G_GNUC_INTERNAL gboolean maplibre_native_ffi_bytes_equal(GBytes* a, GBytes* b);

/** Whether two text arrays, either of which may be null, are equal. */
G_GNUC_INTERNAL gboolean maplibre_native_ffi_strv_equal(char** a, char** b);

/**
 * A host callback with its data and the function that frees the data, which
 * runs once the last reference goes. A registration's copies and the native
 * registrations made from them share it.
 */
typedef struct {
  gatomicrefcount refs;
  GCallback func;
  gpointer user_data;
  GDestroyNotify notify;
} MaplibreNativeFfiClosure;

G_GNUC_INTERNAL MaplibreNativeFfiClosure* maplibre_native_ffi_closure_new(
  GCallback func, gpointer user_data, GDestroyNotify notify
);
G_GNUC_INTERNAL MaplibreNativeFfiClosure* maplibre_native_ffi_closure_ref(
  MaplibreNativeFfiClosure* closure
);
G_GNUC_INTERNAL void maplibre_native_ffi_closure_unref(
  MaplibreNativeFfiClosure* closure
);

/**
 * The closure at `index` of a native registration's context, or null for a
 * callback that the registration leaves unset.
 */
G_GNUC_INTERNAL MaplibreNativeFfiClosure*
maplibre_native_ffi_registration_closure(void* context, guint index);

/**
 * Frees a native registration's context. Native code calls it as the
 * registration's release, once after the last call.
 */
G_GNUC_INTERNAL void maplibre_native_ffi_registration_release(void* context);

/**
 * The scope of one native call: the storage that its converted inputs borrow,
 * and the registrations that it hands to native code.
 */
typedef struct {
  GPtrArray* allocations;
  GPtrArray* registrations;
  GError* error;
} MaplibreNativeFfiCall;

G_GNUC_INTERNAL void maplibre_native_ffi_call_init(MaplibreNativeFfiCall* call);

/** Zeroed storage for `count` items of `size` bytes, freed when the call ends. */
G_GNUC_INTERNAL gpointer maplibre_native_ffi_call_alloc(
  MaplibreNativeFfiCall* call, gsize count, gsize size
);

/**
 * The context of a native registration of `closures`, which the call hands to
 * native code. With `out_queue`, it also writes the delivery queue of the
 * thread-default main context, whose dispatch runs the callbacks. When that
 * fails, the call records the error and this returns null.
 */
G_GNUC_INTERNAL void* maplibre_native_ffi_call_register(
  MaplibreNativeFfiCall* call, MaplibreNativeFfiClosure* const* closures,
  guint count, mln_delivery_queue* out_queue
);

/**
 * Whether converting the call's inputs succeeded. Otherwise it ends the call
 * as rejected and sets `error`.
 */
G_GNUC_INTERNAL gboolean
maplibre_native_ffi_call_check(MaplibreNativeFfiCall* call, GError** error);

/**
 * Frees the call's storage. Unless native code accepted the call, it also
 * frees the registrations that native code never calls or releases.
 */
G_GNUC_INTERNAL void maplibre_native_ffi_call_end(
  MaplibreNativeFfiCall* call, gboolean accepted
);

/** Copies an asynchronous call's result while native code lends it. */
typedef gpointer (*MaplibreNativeFfiDecode)(
  const mln_completion_result* result
);

/**
 * Writes a completion that returns `task` on the task's main context.
 *
 * The completion's delivery queue belongs to that context, which dispatches it
 * when native code queues a call. A successful completion returns what
 * `decode` copies, which `destroy` frees, or TRUE without `decode`. A failed
 * one returns its status as an error. The completion holds a reference to
 * `task` until native code releases it.
 */
G_GNUC_INTERNAL gboolean maplibre_native_ffi_completion_init(
  mln_completion* completion, GTask* task, MaplibreNativeFfiDecode decode,
  GDestroyNotify destroy, GError** error
);

/** Copies a command's generations into a MaplibreNativeFfiCommandCompletion. */
G_GNUC_INTERNAL gpointer
maplibre_native_ffi_command_decode(const mln_completion_result* result);

/**
 * Frees a completion that native code rejected, which it never calls or
 * releases.
 */
G_GNUC_INTERNAL void maplibre_native_ffi_completion_reject(
  const mln_completion* completion
);

/**
 * One asynchronous call: the scope of its native call, the task that it
 * returns, and the completion that native code receives.
 */
typedef struct {
  MaplibreNativeFfiCall call;
  // Null for a call that may go unreported and started without a callback.
  GTask* task;
  mln_completion completion;
  // The completion argument of the native call: null without a task.
  const mln_completion* completion_argument;
  mln_diagnostic diagnostic;
} MaplibreNativeFfiAsync;

/**
 * Starts an asynchronous call of `source_object`, or of no object, whose
 * task finishes on the thread-default main context. A call that may go
 * unreported (`discardable`) and starts without `callback` has no task and
 * passes native code no completion.
 */
G_GNUC_INTERNAL void maplibre_native_ffi_async_init(
  MaplibreNativeFfiAsync* async, gpointer source_object, gpointer source_tag,
  GAsyncReadyCallback callback, gpointer user_data, gboolean discardable
);

/**
 * Whether the call's inputs converted and its completion, which returns what
 * `decode` copies, started. Otherwise it ends the call as rejected, returns
 * the error through the task, and the caller returns.
 */
G_GNUC_INTERNAL gboolean maplibre_native_ffi_async_begin(
  MaplibreNativeFfiAsync* async, MaplibreNativeFfiDecode decode,
  GDestroyNotify destroy
);

/**
 * Ends the call after native code returned `status`. A rejected call frees
 * its registrations and its completion, and returns its error through the
 * task.
 */
G_GNUC_INTERNAL void maplibre_native_ffi_async_end(
  MaplibreNativeFfiAsync* async, mln_status status
);

/**
 * Whether a handle that a call creates at once is ready: the call's
 * completion settles it, and any number of waits read it.
 */
typedef struct MaplibreNativeFfiReady MaplibreNativeFfiReady;

G_GNUC_INTERNAL MaplibreNativeFfiReady* maplibre_native_ffi_ready_new(void);
G_GNUC_INTERNAL MaplibreNativeFfiReady* maplibre_native_ffi_ready_ref(
  MaplibreNativeFfiReady* ready
);
G_GNUC_INTERNAL void maplibre_native_ffi_ready_unref(
  MaplibreNativeFfiReady* ready
);

/** Writes a completion that settles `ready`, which it holds a reference to. */
G_GNUC_INTERNAL gboolean maplibre_native_ffi_completion_init_ready(
  mln_completion* completion, MaplibreNativeFfiReady* ready, GError** error
);

/**
 * Returns TRUE or the readiness error from `task` once `ready` settles. A
 * null `ready` belongs to a handle that is ready at once.
 */
G_GNUC_INTERNAL void maplibre_native_ffi_ready_wait(
  MaplibreNativeFfiReady* ready, GTask* task
);

G_END_DECLS

#endif
