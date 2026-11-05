// Each main context that starts asynchronous calls or registers callbacks gets
// one dispatcher: a GSource attached to the context, and a delivery queue
// whose wake marks the source ready. Native threads only queue calls and wake
// the source, and the context runs every completion and callback when it
// dispatches the source, on the thread that iterates it.
//
// A dispatcher lives while it has pending registrations. The last release,
// which native code runs during a dispatch, retires it after the dispatch
// returns, so a context that no longer starts calls keeps no source.

#include <gio/gio.h>
#include <maplibre_native_c.h>
#include <string.h>

#include "maplibre-native-ffi-command-completion.h"
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
  MaplibreNativeFfiReady* ready;
  MaplibreNativeFfiDecode decode;
  GDestroyNotify destroy;
  Dispatcher* dispatcher;
} Completion;

typedef struct {
  // Null for a registration whose callbacks native code calls on its own
  // threads.
  Dispatcher* dispatcher;
  guint count;
  MaplibreNativeFfiClosure* closures[];
} Registration;

struct MaplibreNativeFfiReady {
  gatomicrefcount refs;
  GMutex lock;
  gboolean settled;
  GError* error;
  // The tasks that wait for the handle, until it settles.
  GPtrArray* waiting;
};

static GMutex dispatchers_lock;
// The dispatcher of each main context, by context. Guarded by
// dispatchers_lock.
static GHashTable* dispatchers;

G_DEFINE_BOXED_TYPE(
  MaplibreNativeFfiCommandCompletion, maplibre_native_ffi_command_completion,
  maplibre_native_ffi_command_completion_copy,
  maplibre_native_ffi_command_completion_free
)

MaplibreNativeFfiCommandCompletion* maplibre_native_ffi_command_completion_copy(
  const MaplibreNativeFfiCommandCompletion* self
) {
  g_return_val_if_fail(self != NULL, NULL);
  return g_memdup2(self, sizeof *self);
}

void maplibre_native_ffi_command_completion_free(
  MaplibreNativeFfiCommandCompletion* self
) {
  g_free(self);
}

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

void maplibre_native_ffi_set_borrowed_error(GError** error) {
  g_set_error_literal(
    error, MAPLIBRE_NATIVE_FFI_ERROR, MLN_STATUS_INVALID_STATE,
    "A callback lends this handle, so it can't be released"
  );
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

char* maplibre_native_ffi_text_new(mln_buffer_view view) {
  return maplibre_native_ffi_text_new_sized(view.data, view.size);
}

char* maplibre_native_ffi_text_new_sized(const void* data, size_t size) {
  if (data == NULL) return g_strdup("");
  return g_strndup(data, size);
}

mln_buffer_view maplibre_native_ffi_text_view(const char* text) {
  return (mln_buffer_view){
    .data = text,
    .size = text != NULL ? strlen(text) : 0,
  };
}

GBytes* maplibre_native_ffi_bytes_new(const void* data, size_t size) {
  if (data == NULL) return NULL;
  return g_bytes_new(data, size);
}

mln_buffer_view maplibre_native_ffi_bytes_view(GBytes* bytes) {
  gsize size = 0;
  const void* data = bytes != NULL ? g_bytes_get_data(bytes, &size) : NULL;
  return (mln_buffer_view){.data = data, .size = size};
}

gboolean maplibre_native_ffi_bytes_equal(GBytes* a, GBytes* b) {
  if (a == NULL || b == NULL) return a == b;
  return g_bytes_equal(a, b);
}

gboolean maplibre_native_ffi_strv_equal(char** a, char** b) {
  if (a == NULL || b == NULL) return a == b;
  return g_strv_equal((const char* const*)a, (const char* const*)b);
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
 * The dispatcher of the thread-default main context, created on first use,
 * with one more pending registration.
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

MaplibreNativeFfiClosure* maplibre_native_ffi_closure_new(
  GCallback func, gpointer user_data, GDestroyNotify notify
) {
  MaplibreNativeFfiClosure* closure = g_new0(MaplibreNativeFfiClosure, 1);
  g_atomic_ref_count_init(&closure->refs);
  closure->func = func;
  closure->user_data = user_data;
  closure->notify = notify;
  return closure;
}

MaplibreNativeFfiClosure* maplibre_native_ffi_closure_ref(
  MaplibreNativeFfiClosure* closure
) {
  if (closure != NULL) g_atomic_ref_count_inc(&closure->refs);
  return closure;
}

void maplibre_native_ffi_closure_unref(MaplibreNativeFfiClosure* closure) {
  if (closure == NULL || !g_atomic_ref_count_dec(&closure->refs)) return;
  if (closure->notify != NULL) closure->notify(closure->user_data);
  g_free(closure);
}

MaplibreNativeFfiClosure* maplibre_native_ffi_registration_closure(
  void* context, guint index
) {
  Registration* registration = context;
  return index < registration->count ? registration->closures[index] : NULL;
}

void maplibre_native_ffi_registration_release(void* context) {
  Registration* registration = context;
  for (guint index = 0; index < registration->count; index++) {
    maplibre_native_ffi_closure_unref(registration->closures[index]);
  }
  Dispatcher* dispatcher = registration->dispatcher;
  g_free(registration);
  if (dispatcher != NULL) dispatcher_release(dispatcher);
}

// A call creates its arrays on first use, since most calls convert nothing
// that needs them.
void maplibre_native_ffi_call_init(MaplibreNativeFfiCall* call) {
  call->allocations = NULL;
  call->registrations = NULL;
  call->error = NULL;
}

gpointer maplibre_native_ffi_call_alloc(
  MaplibreNativeFfiCall* call, gsize count, gsize size
) {
  if (count == 0) return NULL;
  gpointer storage = g_malloc0_n(count, size);
  if (call->allocations == NULL) {
    call->allocations = g_ptr_array_new_with_free_func(g_free);
  }
  g_ptr_array_add(call->allocations, storage);
  return storage;
}

void* maplibre_native_ffi_call_register(
  MaplibreNativeFfiCall* call, MaplibreNativeFfiClosure* const* closures,
  guint count, mln_delivery_queue* out_queue
) {
  if (call->error != NULL) return NULL;
  Dispatcher* dispatcher = NULL;
  if (out_queue != NULL) {
    GMainContext* context = g_main_context_ref_thread_default();
    dispatcher = dispatcher_acquire(context, &call->error);
    g_main_context_unref(context);
    if (dispatcher == NULL) return NULL;
    *out_queue = dispatcher->queue;
  }
  Registration* registration =
    g_malloc0(sizeof *registration + count * sizeof *closures);
  registration->dispatcher = dispatcher;
  registration->count = count;
  for (guint index = 0; index < count; index++) {
    registration->closures[index] =
      maplibre_native_ffi_closure_ref(closures[index]);
  }
  if (call->registrations == NULL) call->registrations = g_ptr_array_new();
  g_ptr_array_add(call->registrations, registration);
  return registration;
}

gboolean maplibre_native_ffi_call_check(
  MaplibreNativeFfiCall* call, GError** error
) {
  if (call->error == NULL) return TRUE;
  GError* failure = g_steal_pointer(&call->error);
  maplibre_native_ffi_call_end(call, FALSE);
  g_propagate_error(error, failure);
  return FALSE;
}

void maplibre_native_ffi_call_end(
  MaplibreNativeFfiCall* call, gboolean accepted
) {
  // A rejected call hands nothing to native code, which then never calls or
  // releases its registrations.
  if (!accepted && call->registrations != NULL) {
    for (guint index = 0; index < call->registrations->len; index++) {
      maplibre_native_ffi_registration_release(
        call->registrations->pdata[index]
      );
    }
  }
  g_clear_pointer(&call->registrations, g_ptr_array_unref);
  g_clear_pointer(&call->allocations, g_ptr_array_unref);
  g_clear_error(&call->error);
}

static void completion_callback(
  void* user_data, const mln_completion_result* result
) {
  Completion* completion = user_data;
  if (completion->ready != NULL) {
    MaplibreNativeFfiReady* ready = completion->ready;
    g_mutex_lock(&ready->lock);
    ready->settled = TRUE;
    if (result->status != MLN_STATUS_OK) {
      ready->error = error_from_view(result->status, result->diagnostic);
    }
    GPtrArray* waiting = g_steal_pointer(&ready->waiting);
    g_mutex_unlock(&ready->lock);
    for (guint index = 0; index < waiting->len; index++) {
      maplibre_native_ffi_ready_wait(ready, waiting->pdata[index]);
    }
    g_ptr_array_unref(waiting);
    return;
  }
  if (result->status != MLN_STATUS_OK) {
    g_task_return_error(
      completion->task, error_from_view(result->status, result->diagnostic)
    );
    return;
  }
  if (completion->decode == NULL) {
    g_task_return_boolean(completion->task, TRUE);
    return;
  }
  g_task_return_pointer(
    completion->task, completion->decode(result), completion->destroy
  );
}

static void completion_release(void* user_data) {
  Completion* completion = user_data;
  Dispatcher* dispatcher = completion->dispatcher;
  g_clear_object(&completion->task);
  g_clear_pointer(&completion->ready, maplibre_native_ffi_ready_unref);
  g_free(completion);
  dispatcher_release(dispatcher);
}

static gboolean completion_start(
  mln_completion* completion, Completion* data, GMainContext* context,
  GError** error
) {
  Dispatcher* dispatcher = dispatcher_acquire(context, error);
  if (dispatcher == NULL) {
    g_clear_object(&data->task);
    g_clear_pointer(&data->ready, maplibre_native_ffi_ready_unref);
    g_free(data);
    return FALSE;
  }
  data->dispatcher = dispatcher;
  memset(completion, 0, sizeof *completion);
  completion->size = sizeof *completion;
  completion->callback = completion_callback;
  completion->user_data = data;
  completion->release_user_data = completion_release;
  completion->queue = dispatcher->queue;
  return TRUE;
}

gboolean maplibre_native_ffi_completion_init(
  mln_completion* completion, GTask* task, MaplibreNativeFfiDecode decode,
  GDestroyNotify destroy, GError** error
) {
  Completion* data = g_new0(Completion, 1);
  data->task = g_object_ref(task);
  data->decode = decode;
  data->destroy = destroy;
  return completion_start(completion, data, g_task_get_context(task), error);
}

gpointer maplibre_native_ffi_command_decode(
  const mln_completion_result* result
) {
  MaplibreNativeFfiCommandCompletion* value =
    g_new0(MaplibreNativeFfiCommandCompletion, 1);
  value->generation = result->generation;
  value->render_update_generation = result->render_update_generation;
  return value;
}

void maplibre_native_ffi_completion_reject(const mln_completion* completion) {
  completion_release(completion->user_data);
}

void maplibre_native_ffi_async_init(
  MaplibreNativeFfiAsync* async, gpointer source_object, gpointer source_tag,
  GAsyncReadyCallback callback, gpointer user_data, gboolean discardable
) {
  async->task = NULL;
  if (callback != NULL || !discardable) {
    async->task = g_task_new(source_object, NULL, callback, user_data);
    g_task_set_source_tag(async->task, source_tag);
  }
  maplibre_native_ffi_call_init(&async->call);
  async->completion_argument = NULL;
  memset(&async->diagnostic, 0, sizeof async->diagnostic);
  async->diagnostic.size = sizeof async->diagnostic;
}

/** Returns `error` through the call's task, or drops it without one. */
static void async_fail(MaplibreNativeFfiAsync* async, GError* error) {
  if (async->task == NULL) {
    g_error_free(error);
    return;
  }
  g_task_return_error(async->task, error);
  g_clear_object(&async->task);
}

gboolean maplibre_native_ffi_async_begin(
  MaplibreNativeFfiAsync* async, MaplibreNativeFfiDecode decode,
  GDestroyNotify destroy
) {
  GError* error = NULL;
  if (!maplibre_native_ffi_call_check(&async->call, &error)) {
    async_fail(async, error);
    return FALSE;
  }
  if (async->task == NULL) return TRUE;
  if (!maplibre_native_ffi_completion_init(
        &async->completion, async->task, decode, destroy, &error
      )) {
    maplibre_native_ffi_call_end(&async->call, FALSE);
    async_fail(async, error);
    return FALSE;
  }
  async->completion_argument = &async->completion;
  return TRUE;
}

void maplibre_native_ffi_async_end(
  MaplibreNativeFfiAsync* async, mln_status status
) {
  maplibre_native_ffi_call_end(&async->call, status == MLN_STATUS_OK);
  if (status != MLN_STATUS_OK && async->completion_argument != NULL) {
    maplibre_native_ffi_completion_reject(&async->completion);
    async_fail(
      async, maplibre_native_ffi_error_new(status, &async->diagnostic)
    );
  }
  g_clear_object(&async->task);
}

MaplibreNativeFfiReady* maplibre_native_ffi_ready_new(void) {
  MaplibreNativeFfiReady* ready = g_new0(MaplibreNativeFfiReady, 1);
  g_atomic_ref_count_init(&ready->refs);
  g_mutex_init(&ready->lock);
  ready->waiting = g_ptr_array_new_with_free_func(g_object_unref);
  return ready;
}

MaplibreNativeFfiReady* maplibre_native_ffi_ready_ref(
  MaplibreNativeFfiReady* ready
) {
  g_atomic_ref_count_inc(&ready->refs);
  return ready;
}

void maplibre_native_ffi_ready_unref(MaplibreNativeFfiReady* ready) {
  if (!g_atomic_ref_count_dec(&ready->refs)) return;
  g_clear_pointer(&ready->waiting, g_ptr_array_unref);
  g_clear_error(&ready->error);
  g_mutex_clear(&ready->lock);
  g_free(ready);
}

gboolean maplibre_native_ffi_completion_init_ready(
  mln_completion* completion, MaplibreNativeFfiReady* ready, GError** error
) {
  Completion* data = g_new0(Completion, 1);
  data->ready = maplibre_native_ffi_ready_ref(ready);
  GMainContext* context = g_main_context_ref_thread_default();
  gboolean started = completion_start(completion, data, context, error);
  g_main_context_unref(context);
  return started;
}

void maplibre_native_ffi_ready_wait(
  MaplibreNativeFfiReady* ready, GTask* task
) {
  if (ready == NULL) {
    g_task_return_boolean(task, TRUE);
    return;
  }
  g_mutex_lock(&ready->lock);
  if (!ready->settled) {
    g_ptr_array_add(ready->waiting, g_object_ref(task));
    g_mutex_unlock(&ready->lock);
    return;
  }
  GError* error = ready->error != NULL ? g_error_copy(ready->error) : NULL;
  g_mutex_unlock(&ready->lock);
  if (error != NULL) {
    g_task_return_error(task, error);
  } else {
    g_task_return_boolean(task, TRUE);
  }
}
