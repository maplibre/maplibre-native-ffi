// Each main context that starts asynchronous calls gets one dispatcher: a
// GSource attached to the context, and a delivery queue whose wake marks the
// source ready. Native threads only queue calls and wake the source, and the
// context runs every completion when it dispatches the source, on the thread
// that iterates it.
//
// A dispatcher lives while it has pending registrations. The last release,
// which native code runs during a dispatch, retires it after the dispatch
// returns, so a context that no longer starts calls keeps no source.

#include <gio/gio.h>
#include <maplibre_native_c.h>
#include <string.h>

#include "maplibre-native-ffi-enums.h"
#include "maplibre-native-ffi-private.h"

typedef struct {
  GSource source;
  GMainContext* context;
  mln_delivery_queue queue;
  // Guarded by dispatchers_lock.
  guint pending;
  gboolean dispatching;
} Dispatcher;

typedef struct {
  GTask* task;
  Dispatcher* dispatcher;
} Completion;

static GMutex dispatchers_lock;
// The dispatcher of each main context, by context. Guarded by
// dispatchers_lock.
static GHashTable* dispatchers;

GError* maplibre_native_ffi_error_new(
  mln_status status, const mln_diagnostic* diagnostic
) {
  const char* message = diagnostic != NULL ? diagnostic->message : "";
  if (message[0] == '\0') {
    return g_error_new(
      MAPLIBRE_NATIVE_FFI_ERROR, status, "MapLibre Native status %d", status
    );
  }
  return g_error_new_literal(MAPLIBRE_NATIVE_FFI_ERROR, status, message);
}

gboolean maplibre_native_ffi_check(
  mln_status status, const mln_diagnostic* diagnostic, GError** error
) {
  if (status == MLN_STATUS_OK) return TRUE;
  g_propagate_error(error, maplibre_native_ffi_error_new(status, diagnostic));
  return FALSE;
}

static GError* error_from_view(mln_status status, mln_buffer_view message) {
  if (message.size == 0) {
    return g_error_new(
      MAPLIBRE_NATIVE_FFI_ERROR, status, "MapLibre Native status %d", status
    );
  }
  return g_error_new(
    MAPLIBRE_NATIVE_FFI_ERROR, status, "%.*s", (int)message.size,
    (const char*)message.data
  );
}

/** Takes the dispatcher out of service, when nothing names its queue. */
static void dispatcher_retire(Dispatcher* dispatcher) {
  g_source_destroy(&dispatcher->source);
  // The queue's release ends its wake, which drops the wake's reference.
  mln_delivery_queue_release(dispatcher->queue);
  g_main_context_unref(dispatcher->context);
  g_source_unref(&dispatcher->source);
}

/**
 * Whether the dispatcher has no pending registrations and runs no dispatch, in
 * which case it leaves the table. Call with dispatchers_lock held.
 */
static gboolean dispatcher_take_if_idle(Dispatcher* dispatcher) {
  if (dispatcher->pending != 0 || dispatcher->dispatching) return FALSE;
  g_hash_table_remove(dispatchers, dispatcher->context);
  return TRUE;
}

static gboolean dispatcher_dispatch(
  GSource* source, GSourceFunc callback, gpointer user_data
) {
  (void)callback;
  (void)user_data;
  Dispatcher* dispatcher = (Dispatcher*)source;
  // A wake during the dispatch makes the source ready again.
  g_source_set_ready_time(source, -1);
  g_mutex_lock(&dispatchers_lock);
  dispatcher->dispatching = TRUE;
  g_mutex_unlock(&dispatchers_lock);

  mln_diagnostic diagnostic = {.size = sizeof diagnostic};
  // Only this context dispatches the queue, so it can't be busy.
  (void)mln_delivery_queue_dispatch(dispatcher->queue, &diagnostic);

  g_mutex_lock(&dispatchers_lock);
  dispatcher->dispatching = FALSE;
  gboolean idle = dispatcher_take_if_idle(dispatcher);
  g_mutex_unlock(&dispatchers_lock);
  if (idle) dispatcher_retire(dispatcher);
  return G_SOURCE_CONTINUE;
}

static GSourceFuncs dispatcher_funcs = {
  .dispatch = dispatcher_dispatch,
};

/** Runs on any thread, including the caller's during a library call. */
static void dispatcher_wake(void* user_data) {
  g_source_set_ready_time(user_data, 0);
}

static void dispatcher_wake_release(void* user_data) {
  g_source_unref(user_data);
}

/**
 * The dispatcher of `context`, created on first use, with one more pending
 * registration.
 */
static Dispatcher* dispatcher_acquire(GMainContext* context, GError** error) {
  g_mutex_lock(&dispatchers_lock);
  if (dispatchers == NULL) dispatchers = g_hash_table_new(NULL, NULL);
  Dispatcher* dispatcher = g_hash_table_lookup(dispatchers, context);
  if (dispatcher == NULL) {
    dispatcher =
      (Dispatcher*)g_source_new(&dispatcher_funcs, sizeof *dispatcher);
    g_source_set_static_name(&dispatcher->source, "MapLibre Native delivery");
    dispatcher->context = g_main_context_ref(context);
    g_source_attach(&dispatcher->source, context);

    mln_delivery_queue_options options = {.size = sizeof options};
    options.wake.callback = dispatcher_wake;
    options.wake.user_data = g_source_ref(&dispatcher->source);
    options.wake.release_user_data = dispatcher_wake_release;
    mln_diagnostic diagnostic = {.size = sizeof diagnostic};
    mln_status status =
      mln_delivery_queue_create(&options, &dispatcher->queue, &diagnostic);
    if (status != MLN_STATUS_OK) {
      g_mutex_unlock(&dispatchers_lock);
      // A rejected wake stays with the caller.
      g_source_unref(&dispatcher->source);
      g_source_destroy(&dispatcher->source);
      g_main_context_unref(dispatcher->context);
      g_source_unref(&dispatcher->source);
      (void)maplibre_native_ffi_check(status, &diagnostic, error);
      return NULL;
    }
    g_hash_table_insert(dispatchers, context, dispatcher);
  }
  dispatcher->pending++;
  g_mutex_unlock(&dispatchers_lock);
  return dispatcher;
}

static void dispatcher_release(Dispatcher* dispatcher) {
  g_mutex_lock(&dispatchers_lock);
  dispatcher->pending--;
  gboolean idle = dispatcher_take_if_idle(dispatcher);
  g_mutex_unlock(&dispatchers_lock);
  if (idle) dispatcher_retire(dispatcher);
}

static void completion_callback(
  void* user_data, const mln_completion_result* result
) {
  Completion* completion = user_data;
  if (result->status != MLN_STATUS_OK) {
    g_task_return_error(
      completion->task, error_from_view(result->status, result->diagnostic)
    );
    return;
  }
  g_task_return_boolean(completion->task, TRUE);
}

static void completion_release(void* user_data) {
  Completion* completion = user_data;
  Dispatcher* dispatcher = completion->dispatcher;
  g_object_unref(completion->task);
  g_free(completion);
  dispatcher_release(dispatcher);
}

gboolean maplibre_native_ffi_completion_init(
  mln_completion* completion, GTask* task, GError** error
) {
  Dispatcher* dispatcher = dispatcher_acquire(g_task_get_context(task), error);
  if (dispatcher == NULL) return FALSE;
  Completion* data = g_new0(Completion, 1);
  data->task = g_object_ref(task);
  data->dispatcher = dispatcher;
  memset(completion, 0, sizeof *completion);
  completion->size = sizeof *completion;
  completion->callback = completion_callback;
  completion->user_data = data;
  completion->release_user_data = completion_release;
  completion->queue = dispatcher->queue;
  return TRUE;
}

void maplibre_native_ffi_completion_reject(const mln_completion* completion) {
  completion_release(completion->user_data);
}
