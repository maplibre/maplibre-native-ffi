// The realm of one Node-API environment: its wake shim, its delivery queue, its
// owner tokens, and its teardown.
//
// Native threads reach a realm only through its wake shim, which holds the
// realm's threadsafe function. The delivery queue's wake posts to the shim,
// and the realm dispatches the queue on its event loop, after the native
// callback that queued each call has returned. Every completion, registration,
// and wake of the realm names the queue, so its trampoline runs there and
// calls the JavaScript value that its user data references. Native code copies
// every argument and answers every synchronous decision itself.

// dladdr is a Darwin extension.
#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE
#endif

#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runtime.h"

#ifdef __APPLE__
#include <dlfcn.h>
#endif

// What the threadsafe function's data carries: NULL asks the realm to dispatch
// its queue, and a pointer is a leak report, or, with MLN_TS_MESSAGE_THREAD
// set, a finished thread start.
enum {
  MLN_TS_MESSAGE_THREAD = 1,
  MLN_TS_MESSAGE_MASK = 1,
};

/**
 * What native threads reach of a realm. A poster calls the threadsafe function
 * only under the lock and only while the shim is open, so a realm's teardown
 * ends every call before the host frees the function. The realm, the
 * function's finalizer, the queue's wake, and each wake registration hold a
 * reference.
 */
typedef struct mln_ts_shim {
  pthread_mutex_t lock;
  bool open;
  napi_threadsafe_function function;
  atomic_uint references;
} mln_ts_shim;

/**
 * The user data of one registration or completion: its realm's environment,
 * where its trampoline runs, and a reference to its JavaScript value. A
 * completion's value is the function that settles its Promise, which it
 * decodes its result for. A registration's is the root that its owner keeps,
 * which reaches its host value only while the owner lives.
 */
typedef struct mln_ts_host {
  mln_ts_realm* realm;
  napi_ref reference;
  mln_ts_decoder decoder;
} mln_ts_host;

/** An owner that the collector reclaimed while open. */
typedef struct mln_ts_leak {
  uint64_t handle;
  char name[64];
} mln_ts_leak;

/** A call on a thread of its own, and its outcome once it returns. */
typedef struct mln_ts_thread_start {
  mln_ts_shim* shim;
  // The function that settles the call's Promise.
  napi_ref settle;
  mln_ts_thread_run run;
  void (*dispose)(uint64_t handle);
  // The call memory that the copied arguments point into.
  mln_ts_allocation* allocations;
  mln_status status;
  uint64_t handle;
  mln_diagnostic diagnostic;
  max_align_t arguments[];
} mln_ts_thread_start;

struct mln_ts_realm {
  napi_env env;
  mln_ts_shim* shim;
  // Created by the first completion or registration.
  mln_delivery_queue queue;
  napi_ref register_value;
  napi_ref invoke;
  napi_ref retire;
  napi_ref make_error;
  napi_ref take_native;
  napi_ref report_leak;
  bool referenced;
  bool cleaned;
  // The owners whose tokens this realm's teardown finalizes. The exit path
  // reads them from another thread, so they are guarded.
  pthread_mutex_t owners_lock;
  mln_ts_owner* owners;
  mln_ts_realm* next_realm;
};

struct mln_ts_owner {
  mln_ts_owner* previous;
  mln_ts_owner* next;
  mln_ts_realm* realm;
  // Taken by exchange: the exit path ends session tokens from another thread
  // while the realm's own thread may close them.
  void* _Atomic token;
  uint64_t handle;
  bool session;
  bool linked;
  char name[64];
};

// The realms that the exit path visits.
static pthread_mutex_t registry_lock = PTHREAD_MUTEX_INITIALIZER;
static mln_ts_realm* registry;

mln_ts_realm* mln_ts_realm_of(napi_env env) {
  void* data = NULL;
  return napi_get_instance_data(env, &data) == napi_ok ? data : NULL;
}

static napi_value reference_value(napi_env env, napi_ref reference) {
  napi_value value = NULL;
  if (reference != NULL) {
    napi_get_reference_value(env, reference, &value);
  }
  return value;
}

void mln_ts_throw_status(napi_env env, mln_status status, const char* message) {
  mln_ts_realm* realm = mln_ts_realm_of(env);
  napi_value make_error =
    realm != NULL ? reference_value(env, realm->make_error) : NULL;
  napi_value global;
  napi_value error = NULL;
  napi_value argv[2] = {
    mln_ts_number(env, status), mln_ts_text(env, message, strlen(message))
  };
  if (
    make_error != NULL && napi_get_global(env, &global) == napi_ok &&
    napi_call_function(env, global, make_error, 2, argv, &error) == napi_ok
  ) {
    napi_throw(env, error);
    return;
  }
  bool pending = false;
  napi_is_exception_pending(env, &pending);
  if (!pending) {
    napi_throw_error(env, NULL, message);
  }
}

// ---------------------------------------------------------------------------
// The wake shim

static mln_ts_shim* shim_retain(mln_ts_shim* shim) {
  atomic_fetch_add_explicit(&shim->references, 1, memory_order_relaxed);
  return shim;
}

static void shim_release(mln_ts_shim* shim) {
  if (
    atomic_fetch_sub_explicit(&shim->references, 1, memory_order_acq_rel) == 1
  ) {
    pthread_mutex_destroy(&shim->lock);
    free(shim);
  }
}

/** Posts one message, and reports whether the realm will receive it. */
static bool shim_post(mln_ts_shim* shim, void* data) {
  bool posted = false;
  pthread_mutex_lock(&shim->lock);
  if (shim->open) {
    posted = napi_call_threadsafe_function(
               shim->function, data, napi_tsfn_nonblocking
             ) == napi_ok;
  }
  pthread_mutex_unlock(&shim->lock);
  return posted;
}

/** Closes the shim, and returns its function if this call closed it. */
static napi_threadsafe_function shim_close(mln_ts_shim* shim) {
  pthread_mutex_lock(&shim->lock);
  napi_threadsafe_function function = shim->open ? shim->function : NULL;
  shim->open = false;
  pthread_mutex_unlock(&shim->lock);
  return function;
}

/** The queue's wake: asks the realm to dispatch the queue. */
static void queue_wake(void* user_data) { shim_post(user_data, NULL); }

static void queue_wake_release(void* user_data) { shim_release(user_data); }

// ---------------------------------------------------------------------------
// Thread starts

/**
 * Disposes what a thread start created for a realm that cannot take it. The
 * realm's environment is gone, and its settle reference with it.
 */
static void thread_start_abandon(mln_ts_thread_start* start) {
  if (start->status == MLN_STATUS_OK && start->handle != 0) {
    start->dispose(start->handle);
  }
  free(start);
}

static void* thread_start_main(void* argument) {
  mln_ts_thread_start* start = argument;
  start->diagnostic.size = sizeof start->diagnostic;
  start->diagnostic.message[0] = '\0';
  start->status =
    start->run(start->arguments, &start->handle, &start->diagnostic);
  mln_ts_free_allocations(start->allocations);
  start->allocations = NULL;
  // The realm may free the start as soon as the post lands.
  mln_ts_shim* shim = start->shim;
  if (!shim_post(shim, (void*)((uintptr_t)start | MLN_TS_MESSAGE_THREAD))) {
    thread_start_abandon(start);
  }
  shim_release(shim);
  return NULL;
}

bool mln_ts_start_thread(
  mln_ts_call* call, napi_value settle, mln_ts_thread_run run,
  void (*dispose)(uint64_t handle), const void* arguments, size_t size
) {
  if (!mln_ts_read_function(call, settle, "start")) {
    return false;
  }
  mln_ts_thread_start* start = calloc(1, sizeof *start + size);
  if (
    start == NULL ||
    napi_create_reference(call->env, settle, 1, &start->settle) != napi_ok
  ) {
    free(start);
    napi_throw_error(call->env, NULL, "thread start allocation failed");
    return false;
  }
  memcpy(start->arguments, arguments, size);
  start->shim = shim_retain(call->realm->shim);
  start->run = run;
  start->dispose = dispose;
  start->allocations = call->allocations;
  call->allocations = NULL;
  // Nothing joins the thread, so a browser page that never blocks can start
  // it. It runs once a pool worker takes it.
  pthread_attr_t attributes;
  pthread_attr_init(&attributes);
  pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
  pthread_t thread;
  int result = pthread_create(&thread, &attributes, thread_start_main, start);
  pthread_attr_destroy(&attributes);
  if (result != 0) {
    call->allocations = start->allocations;
    shim_release(start->shim);
    napi_delete_reference(call->env, start->settle);
    free(start);
    mln_ts_throw_status(
      call->env, MLN_STATUS_NATIVE_ERROR, "the call's thread did not start"
    );
    return false;
  }
  return true;
}

/** Reports an exception that a call left pending, as an uncaught one. */
static void surface_exception(napi_env env) {
  bool pending = false;
  if (napi_is_exception_pending(env, &pending) == napi_ok && pending) {
    napi_value error;
    napi_get_and_clear_last_exception(env, &error);
    napi_fatal_exception(env, error);
  }
}

/** Calls a function of the realm's TypeScript half with the global receiver. */
static bool call_realm(
  napi_env env, napi_ref function, size_t argc, const napi_value* argv,
  napi_value* result
) {
  napi_value target = reference_value(env, function);
  napi_value global;
  return target != NULL && napi_get_global(env, &global) == napi_ok &&
         napi_call_function(env, global, target, argc, argv, result) == napi_ok;
}

/** Settles a finished thread start's Promise, and frees the start. */
static void thread_start_settle(napi_env env, mln_ts_thread_start* start) {
  napi_value settle = reference_value(env, start->settle);
  napi_value global;
  napi_value argv[3] = {
    mln_ts_number(env, start->status), mln_ts_u64(env, start->handle),
    mln_ts_text(
      env, start->diagnostic.message, strlen(start->diagnostic.message)
    )
  };
  napi_delete_reference(env, start->settle);
  // The TypeScript half owns the handle from here.
  free(start);
  if (
    settle == NULL || napi_get_global(env, &global) != napi_ok ||
    napi_call_function(env, global, settle, 3, argv, NULL) != napi_ok
  ) {
    surface_exception(env);
  }
}

/**
 * Receives one message on the realm's JavaScript thread. A host that tears the
 * function down passes no environment: the realm then dispatches nothing, a
 * leak report is only freed, and a thread start disposes what it created.
 */
static void receive(
  napi_env env, napi_value callback, void* context, void* data
) {
  (void)callback;
  mln_ts_realm* realm = context;
  uintptr_t bits = (uintptr_t)data;
  mln_ts_thread_start* start =
    (bits & MLN_TS_MESSAGE_MASK) == MLN_TS_MESSAGE_THREAD
      ? (mln_ts_thread_start*)(bits & ~(uintptr_t)MLN_TS_MESSAGE_MASK)
      : NULL;
  mln_ts_leak* leak = data != NULL && start == NULL ? data : NULL;
  napi_handle_scope scope;
  if (env == NULL || napi_open_handle_scope(env, &scope) != napi_ok) {
    free(leak);
    if (start != NULL) {
      thread_start_abandon(start);
    }
    return;
  }
  if (data == NULL) {
    // Each call opens a scope of its own, and the TypeScript half catches
    // what a host value throws. Calls that arrive meanwhile wake again.
    mln_diagnostic diagnostic = {.size = sizeof diagnostic};
    mln_delivery_queue_dispatch(realm->queue, &diagnostic);
  } else if (start != NULL) {
    thread_start_settle(env, start);
  } else {
    napi_value argv[2] = {
      mln_ts_text(env, leak->name, strlen(leak->name)),
      mln_ts_u64(env, leak->handle)
    };
    if (!call_realm(env, realm->report_leak, 2, argv, NULL)) {
      surface_exception(env);
    }
    free(leak);
  }
  napi_close_handle_scope(env, scope);
}

static void function_finalize(napi_env env, void* data, void* hint) {
  (void)env;
  (void)hint;
  // The host frees the function after this returns. Deno never runs this
  // finalizer, and the realm's cleanup hook closes the shim there.
  shim_close(data);
  shim_release(data);
}

// ---------------------------------------------------------------------------
// The delivery queue

/** The realm's queue, which the first completion or registration creates. */
static bool realm_queue(mln_ts_call* call, mln_delivery_queue* out) {
  mln_ts_realm* realm = call->realm;
  if (realm->queue == MLN_HANDLE_NULL) {
    mln_delivery_queue_options options = {.size = sizeof options};
    options.wake.callback = queue_wake;
    options.wake.user_data = shim_retain(realm->shim);
    options.wake.release_user_data = queue_wake_release;
    // The references of the hosts that release abandons die with the
    // environment.
    options.release_abandoned_user_data = free;
    mln_diagnostic diagnostic = {.size = sizeof diagnostic};
    mln_delivery_queue queue = MLN_HANDLE_NULL;
    mln_status status =
      mln_delivery_queue_create(&options, &queue, &diagnostic);
    if (status != MLN_STATUS_OK) {
      // A rejected wake stays with the caller.
      shim_release(realm->shim);
      return mln_ts_check(call, status, &diagnostic);
    }
    realm->queue = queue;
  }
  *out = realm->queue;
  return true;
}

static void host_free(void* user_data) {
  mln_ts_host* host = user_data;
  napi_delete_reference(host->realm->env, host->reference);
  free(host);
}

/**
 * The user data that references value for the call's realm, which the call
 * frees unless native code accepts it.
 */
static mln_ts_host* host_create(
  mln_ts_call* call, napi_value value, mln_ts_decoder decoder
) {
  mln_ts_host* host = calloc(1, sizeof *host);
  if (
    host == NULL ||
    napi_create_reference(call->env, value, 1, &host->reference) != napi_ok
  ) {
    free(host);
    napi_throw_error(call->env, NULL, "registration allocation failed");
    return NULL;
  }
  host->realm = call->realm;
  host->decoder = decoder;
  return mln_ts_reject_later(call, host_free, host) ? host : NULL;
}

/**
 * A registration's release, which runs during dispatch after its last call:
 * drops the root that the realm's TypeScript half keeps.
 */
static void host_release(void* user_data) {
  mln_ts_host* host = user_data;
  napi_env env = host->realm->env;
  napi_handle_scope scope;
  if (napi_open_handle_scope(env, &scope) == napi_ok) {
    napi_value root = reference_value(env, host->reference);
    if (root == NULL || !call_realm(env, host->realm->retire, 1, &root, NULL)) {
      surface_exception(env);
    }
    napi_close_handle_scope(env, scope);
  }
  host_free(host);
}

bool mln_ts_read_registration(
  mln_ts_call* call, napi_value value, void** out_user_data,
  mln_user_data_release* out_release, mln_delivery_queue* out_queue
) {
  napi_value root;
  mln_ts_host* host = NULL;
  if (
    !call_realm(call->env, call->realm->register_value, 1, &value, &root) ||
    !realm_queue(call, out_queue) ||
    (host = host_create(call, root, NULL)) == NULL
  ) {
    return false;
  }
  *out_user_data = host;
  *out_release = host_release;
  return true;
}

napi_env mln_ts_upcall_begin(void* user_data, napi_handle_scope* scope) {
  napi_env env = ((mln_ts_host*)user_data)->realm->env;
  return napi_open_handle_scope(env, scope) == napi_ok ? env : NULL;
}

bool mln_ts_upcall_end(
  void* user_data, napi_handle_scope scope, uint32_t callback, size_t argc,
  const napi_value* argv
) {
  mln_ts_host* host = user_data;
  napi_env env = host->realm->env;
  napi_value values[MLN_TS_MAX_ARGUMENTS + 3];
  values[0] = mln_ts_number(env, callback);
  values[1] = reference_value(env, host->reference);
  values[2] = mln_ts_undefined(env);
  if (argc > MLN_TS_MAX_ARGUMENTS) {
    argc = MLN_TS_MAX_ARGUMENTS;
  }
  for (size_t index = 0; index < argc; index++) {
    values[index + 3] = argv[index];
    if (argv[index] == NULL) {
      // A failed conversion leaves its error pending, which the TypeScript
      // half reports in place of the call.
      napi_get_and_clear_last_exception(env, &values[2]);
      values[index + 3] = mln_ts_undefined(env);
    }
  }
  napi_value result;
  bool received = false;
  if (call_realm(env, host->realm->invoke, argc + 3, values, &result)) {
    napi_get_value_bool(env, result, &received);
  } else {
    surface_exception(env);
  }
  napi_close_handle_scope(env, scope);
  return received;
}

/** The release that a call runs for a native registration it does not pass
 * on. */
typedef struct mln_ts_native_release {
  mln_user_data_release release;
  void* user_data;
} mln_ts_native_release;

static void native_release(void* data) {
  mln_ts_native_release* entry = data;
  entry->release(entry->user_data);
}

bool mln_ts_read_native_registration(
  mln_ts_call* call, napi_value value, const char* kind, size_t callbacks,
  uint64_t* out, const char* path
) {
  napi_env env = call->env;
  napi_value fields;
  napi_value argv[3] = {
    value, mln_ts_text(env, kind, strlen(kind)),
    mln_ts_text(env, path, strlen(path))
  };
  if (
    argv[1] == NULL || argv[2] == NULL ||
    !call_realm(env, call->realm->take_native, 3, argv, &fields)
  ) {
    return false;
  }
  if (mln_ts_is_absent(call, fields)) {
    char message[256];
    snprintf(
      message, sizeof message,
      "%s: an earlier call already consumed this native registration", path
    );
    mln_ts_throw_status(env, MLN_STATUS_INVALID_STATE, message);
    return false;
  }
  // The callbacks, then the context and its release.
  size_t count = callbacks + 2;
  uint32_t length = 0;
  if (
    napi_get_array_length(env, fields, &length) != napi_ok || length != count
  ) {
    napi_throw_type_error(
      env, NULL, "a native registration has the wrong number of fields"
    );
    return false;
  }
  for (size_t index = 0; index < count; index++) {
    napi_value field;
    if (
      !mln_ts_element(call, fields, index, &field) ||
      !mln_ts_read_u64(call, field, &out[index], path)
    ) {
      return false;
    }
  }
  if (out[callbacks + 1] == 0) {
    return true;
  }
  mln_ts_native_release* entry = mln_ts_allocate(call, sizeof *entry);
  if (entry == NULL) {
    // This call took the registration, so it releases the context even
    // though it fails before native code sees it.
    ((mln_user_data_release)(uintptr_t)out[callbacks + 1])(
      (void*)(uintptr_t)out[callbacks]
    );
    return false;
  }
  entry->release = (mln_user_data_release)(uintptr_t)out[callbacks + 1];
  entry->user_data = (void*)(uintptr_t)out[callbacks];
  // The call's rejections run before its allocations are freed.
  return mln_ts_reject_later(call, native_release, entry);
}

static napi_value completion_header(
  napi_env env, const mln_completion_result* result
) {
  napi_value header = mln_ts_object(env);
  if (
    !mln_ts_set(env, header, "status", mln_ts_number(env, result->status)) ||
    !mln_ts_set(
      env, header, "generation", mln_ts_u64(env, result->generation)
    ) ||
    !mln_ts_set(
      env, header, "renderUpdateGeneration",
      mln_ts_u64(env, result->render_update_generation)
    ) ||
    !mln_ts_set(
      env, header, "diagnostic",
      mln_ts_text(env, result->diagnostic.data, result->diagnostic.size)
    )
  ) {
    return NULL;
  }
  return header;
}

/**
 * A completion's callback, which runs once during dispatch: settles its
 * Promise with the header and the decoded value of a successful result, or
 * with the error that converting them threw.
 */
static void complete(void* user_data, const mln_completion_result* result) {
  mln_ts_host* host = user_data;
  napi_env env = host->realm->env;
  napi_handle_scope scope;
  if (napi_open_handle_scope(env, &scope) == napi_ok) {
    napi_value argv[3] = {
      completion_header(env, result), mln_ts_undefined(env),
      mln_ts_undefined(env)
    };
    if (
      argv[0] != NULL && result->status == MLN_STATUS_OK &&
      host->decoder != NULL
    ) {
      argv[1] = host->decoder(env, result);
    }
    if (argv[0] == NULL || argv[1] == NULL) {
      napi_get_and_clear_last_exception(env, &argv[2]);
      argv[0] = argv[1] = mln_ts_undefined(env);
    }
    napi_value settle = reference_value(env, host->reference);
    napi_value global;
    if (
      settle == NULL || napi_get_global(env, &global) != napi_ok ||
      napi_call_function(env, global, settle, 3, argv, NULL) != napi_ok
    ) {
      surface_exception(env);
    }
    napi_close_handle_scope(env, scope);
  }
  host_free(host);
}

bool mln_ts_read_completion(
  mln_ts_call* call, napi_value settle, bool discardable,
  mln_ts_decoder decoder, mln_completion* storage, const mln_completion** out
) {
  *out = NULL;
  if (discardable && mln_ts_is_absent(call, settle)) {
    return true;
  }
  mln_delivery_queue queue = MLN_HANDLE_NULL;
  mln_ts_host* host = NULL;
  if (
    !mln_ts_read_function(call, settle, "completion") ||
    !realm_queue(call, &queue) ||
    (host = host_create(call, settle, decoder)) == NULL
  ) {
    return false;
  }
  memset(storage, 0, sizeof *storage);
  storage->size = sizeof *storage;
  storage->callback = complete;
  storage->user_data = host;
  storage->queue = queue;
  *out = storage;
  return true;
}

// ---------------------------------------------------------------------------
// Owner tokens

static void owner_unlink(mln_ts_owner* owner) {
  mln_ts_realm* realm = owner->realm;
  pthread_mutex_lock(&realm->owners_lock);
  if (owner->linked) {
    *(owner->previous ? &owner->previous->next : &realm->owners) = owner->next;
    if (owner->next != NULL) {
      owner->next->previous = owner->previous;
    }
    owner->linked = false;
  }
  pthread_mutex_unlock(&realm->owners_lock);
}

/**
 * Ends an owner's token: finalizes its handle, or destroys a closed one.
 * Returns whether this call ended it.
 */
static bool owner_end(mln_ts_owner* owner, bool finalize) {
  void* token = atomic_exchange(&owner->token, NULL);
  if (token == NULL) {
    return false;
  }
  if (finalize) {
    mln_binding_owner_finalize(token);
  } else {
    mln_binding_owner_token_destroy(token);
  }
  return true;
}

void mln_ts_owner_consume(mln_ts_owner* owner) {
  // The owner stays in its realm's list until the collector finalizes its
  // external, so native memory, not only the JavaScript heap, refers to it
  // until then.
  owner_end(owner, false);
}

static void owner_finalize(napi_env env, void* data, void* hint) {
  (void)env;
  (void)hint;
  mln_ts_owner* owner = data;
  // An owner that its realm's teardown unlinked has no token left, and its
  // realm may be gone.
  if (owner->linked) {
    owner_unlink(owner);
    // Collected without close(): dispose the handle, which runs no
    // JavaScript, and report the leak on the event loop.
    if (owner_end(owner, true)) {
      mln_ts_leak* leak = calloc(1, sizeof *leak);
      if (leak != NULL) {
        leak->handle = owner->handle;
        memcpy(leak->name, owner->name, sizeof leak->name);
        if (!shim_post(owner->realm->shim, leak)) {
          free(leak);
        }
      }
    }
  }
  free(owner);
}

// ownerTokenCreate(handle, name, session) -> external
static napi_value owner_token_create(napi_env env, napi_callback_info info) {
  mln_ts_call call;
  if (!mln_ts_begin(&call, env, info, 3)) {
    return NULL;
  }
  uint64_t handle = 0;
  bool session = false;
  size_t length = 0;
  napi_value external;
  mln_ts_owner* owner = calloc(1, sizeof *owner);
  if (
    owner == NULL || !mln_ts_read_u64(&call, call.argv[0], &handle, "handle") ||
    napi_get_value_string_utf8(
      env, call.argv[1], owner->name, sizeof owner->name, &length
    ) != napi_ok ||
    !mln_ts_read_bool(&call, call.argv[2], &session, "session")
  ) {
    free(owner);
    return mln_ts_end(&call);
  }
  // Ownership moves to the token on entry, and a failed allocation disposes
  // the handle.
  void* token = mln_binding_owner_token_create(handle);
  atomic_init(&owner->token, token);
  if (token == NULL) {
    free(owner);
    napi_throw_error(env, NULL, "owner token allocation failed");
    return mln_ts_end(&call);
  }
  owner->handle = handle;
  owner->session = session;
  owner->realm = call.realm;
  if (
    napi_create_external(env, owner, owner_finalize, NULL, &external) != napi_ok
  ) {
    owner_end(owner, true);
    free(owner);
    return mln_ts_end(&call);
  }
  pthread_mutex_lock(&call.realm->owners_lock);
  owner->next = call.realm->owners;
  if (owner->next != NULL) {
    owner->next->previous = owner;
  }
  call.realm->owners = owner;
  owner->linked = true;
  pthread_mutex_unlock(&call.realm->owners_lock);
  call.result = external;
  return mln_ts_end(&call);
}

/**
 * Ends the graphics calls of every live render session of a realm. Abandon
 * waits for a session's in-flight core-worker driver call, so a session that
 * is mid-frame ends before this returns.
 */
static void realm_abandon_sessions(mln_ts_realm* realm) {
  pthread_mutex_lock(&realm->owners_lock);
  for (mln_ts_owner* owner = realm->owners; owner != NULL;
       owner = owner->next) {
    mln_render_abandon_result result = {.size = sizeof result};
    if (
      owner->session && owner->token != NULL &&
      mln_binding_render_session_abandon_at_exit(
        owner->handle, &result, NULL
      ) == MLN_STATUS_OK
    ) {
      owner_end(owner, false);
    }
  }
  pthread_mutex_unlock(&realm->owners_lock);
}

// ---------------------------------------------------------------------------
// Realm teardown and process exit

static void registry_remove(mln_ts_realm* realm) {
  pthread_mutex_lock(&registry_lock);
  for (mln_ts_realm** link = &registry; *link != NULL;
       link = &(*link)->next_realm) {
    if (*link == realm) {
      *link = realm->next_realm;
      break;
    }
  }
  pthread_mutex_unlock(&registry_lock);
}

static void realm_cleanup(void* data) {
  mln_ts_realm* realm = data;
  if (realm->cleaned) {
    return;
  }
  realm->cleaned = true;
  registry_remove(realm);
  // Close the shim first, so no native thread calls the function once the
  // host starts to free it, then release it with abort.
  napi_threadsafe_function function = shim_close(realm->shim);
  if (function != NULL) {
    napi_release_threadsafe_function(function, napi_tsfn_abort);
  }
  // Releasing the queue frees the hosts it abandons, fails every undelivered
  // request, and refuses later pushes.
  if (realm->queue != MLN_HANDLE_NULL) {
    mln_delivery_queue_release(realm->queue);
    realm->queue = MLN_HANDLE_NULL;
  }
  realm_abandon_sessions(realm);
  pthread_mutex_lock(&realm->owners_lock);
  while (realm->owners != NULL) {
    // The external's own finalizer, if the host still runs it, frees the
    // owner and finds no token. Bun 1.4.3 and Deno 2.9.7 never run it for an
    // external that outlives a worker, so that owner's record leaks.
    mln_ts_owner* owner = realm->owners;
    realm->owners = owner->next;
    owner->linked = false;
    owner_end(owner, true);
  }
  pthread_mutex_unlock(&realm->owners_lock);
  // OpenHarmony runs instance-data finalizers after it disposed the realm's
  // VM, where deleting a reference crashes, so the hook deletes them.
  napi_ref* references[] = {
    &realm->register_value, &realm->invoke,      &realm->retire,
    &realm->make_error,     &realm->take_native, &realm->report_leak,
  };
  for (size_t index = 0; index < sizeof references / sizeof *references;
       index++) {
    napi_delete_reference(realm->env, *references[index]);
    *references[index] = NULL;
  }
}

static void realm_finalize(napi_env env, void* data, void* hint) {
  (void)env;
  (void)hint;
  mln_ts_realm* realm = data;
  realm_cleanup(realm);
  shim_release(realm->shim);
  pthread_mutex_destroy(&realm->owners_lock);
  free(realm);
}

/**
 * shutdownAtExit(): closes every realm's shim and abandons every live render
 * session before exit() runs. The main realm calls it from a synchronous exit
 * listener: Node's process.exit() skips the environment cleanup hooks, and
 * Mesa's EGL can register its own exit handler after any that the addon
 * registers, so the listener is the last point at which graphics calls can
 * end. A caller-driver session that another realm is servicing reports
 * MLN_STATUS_BUSY and stays live; that realm's own teardown ends it.
 */
static napi_value shutdown_at_exit(napi_env env, napi_callback_info info) {
  (void)env;
  (void)info;
  pthread_mutex_lock(&registry_lock);
  for (mln_ts_realm* realm = registry; realm != NULL;
       realm = realm->next_realm) {
    shim_close(realm->shim);
    realm_abandon_sessions(realm);
  }
  pthread_mutex_unlock(&registry_lock);
  return NULL;
}

// ---------------------------------------------------------------------------
// Runtime exports

// install(register, invoke, retire, makeError, takeNative, reportLeak)
static napi_value install(napi_env env, napi_callback_info info) {
  mln_ts_call call;
  if (!mln_ts_begin(&call, env, info, 6)) {
    return NULL;
  }
  // A second copy of the binding in this realm would install a second set of
  // callback adapters and roots, and the copies would share one queue.
  if (call.realm->register_value != NULL) {
    napi_throw_error(
      env, NULL,
      "another copy of the binding already uses the addon in this realm; "
      "load one copy of @maplibre/native-ffi per realm"
    );
    return mln_ts_end(&call);
  }
  napi_ref* targets[6] = {
    &call.realm->register_value, &call.realm->invoke,
    &call.realm->retire,         &call.realm->make_error,
    &call.realm->take_native,    &call.realm->report_leak,
  };
  for (size_t index = 0; index < 6; index++) {
    napi_create_reference(env, call.argv[index], 1, targets[index]);
  }
  return mln_ts_end(&call);
}

// setReferenced(referenced): pending binding Promises hold the event loop.
static napi_value set_referenced(napi_env env, napi_callback_info info) {
  mln_ts_call call;
  bool referenced = false;
  if (!mln_ts_begin(&call, env, info, 1)) {
    return NULL;
  }
  mln_ts_shim* shim = call.realm->shim;
  if (
    mln_ts_read_bool(&call, call.argv[0], &referenced, "referenced") &&
    referenced != call.realm->referenced
  ) {
    // The exit path closes every realm's shim from the main thread, so the
    // flag is read under the lock.
    pthread_mutex_lock(&shim->lock);
    if (shim->open) {
      (referenced ? napi_ref_threadsafe_function
                  : napi_unref_threadsafe_function)(env, shim->function);
      call.realm->referenced = referenced;
    }
    pthread_mutex_unlock(&shim->lock);
  }
  return mln_ts_end(&call);
}

static void define(
  napi_env env, napi_value exports, const mln_ts_export* entries, size_t count
) {
  for (size_t index = 0; index < count; index++) {
    napi_value function;
    napi_create_function(
      env, entries[index].name, NAPI_AUTO_LENGTH, entries[index].function, NULL,
      &function
    );
    napi_set_named_property(env, exports, entries[index].name, function);
  }
}

// Native threads of the C library outlive any one realm, so the module
// never unloads. ELF builds link with -z nodelete, and Darwin builds mark the
// loaded image when the first realm initializes.
static void pin_module(void) {
  static atomic_flag pinned = ATOMIC_FLAG_INIT;
  if (atomic_flag_test_and_set(&pinned)) {
    return;
  }
#ifdef __APPLE__
  Dl_info info;
  if (dladdr((void*)&pin_module, &info) != 0 && info.dli_fname != NULL) {
    dlopen(info.dli_fname, RTLD_NOW | RTLD_NOLOAD | RTLD_NODELETE);
  }
#endif
}

napi_value mln_ts_module_init(
  napi_env env, napi_value exports, const mln_ts_export* generated,
  size_t generated_count
) {
  pin_module();
  mln_ts_realm* realm = calloc(1, sizeof *realm);
  mln_ts_shim* shim = realm ? calloc(1, sizeof *shim) : NULL;
  if (shim == NULL) {
    free(realm);
    napi_throw_error(env, NULL, "realm allocation failed");
    return NULL;
  }
  pthread_mutex_init(&shim->lock, NULL);
  // One reference for the realm and one for the function's finalizer.
  atomic_init(&shim->references, 2);
  realm->env = env;
  realm->shim = shim;
  pthread_mutex_init(&realm->owners_lock, NULL);
  napi_value name;
  napi_create_string_utf8(env, "maplibre_native_ffi", NAPI_AUTO_LENGTH, &name);
  if (
    napi_create_threadsafe_function(
      env, NULL, NULL, name, 0, 1, shim, function_finalize, realm, receive,
      &shim->function
    ) != napi_ok
  ) {
    pthread_mutex_destroy(&shim->lock);
    pthread_mutex_destroy(&realm->owners_lock);
    free(shim);
    free(realm);
    return NULL;
  }
  shim->open = true;
  // A registration keeps nothing alive; pending Promises reference the
  // function through setReferenced().
  napi_unref_threadsafe_function(env, shim->function);
  if (napi_set_instance_data(env, realm, realm_finalize, NULL) != napi_ok) {
    return NULL;
  }
  // Registered after the function, so it runs before the host's own cleanup
  // of the function on hosts that run hooks in reverse order.
  napi_add_env_cleanup_hook(env, realm_cleanup, realm);
  pthread_mutex_lock(&registry_lock);
  realm->next_realm = registry;
  registry = realm;
  pthread_mutex_unlock(&registry_lock);
  static const mln_ts_export runtime_exports[] = {
    {"install", install},
    {"setReferenced", set_referenced},
    {"ownerTokenCreate", owner_token_create},
    {"shutdownAtExit", shutdown_at_exit},
  };
  define(
    env, exports, runtime_exports,
    sizeof runtime_exports / sizeof runtime_exports[0]
  );
  define(env, exports, generated, generated_count);
  return exports;
}
