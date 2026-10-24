// The realm of one Node-API environment: its wake shim, its delivery queue, the
// batch that it dispatches, its owner tokens, and its teardown.
//
// Native threads reach a realm only through its wake shim, which holds the
// realm's threadsafe function. The delivery queue's wake posts token 0, which
// asks the realm to drain the queue; a wake registration posts its own token,
// and its release posts the token once more so the realm drops the root. The
// realm drains on its event loop, after the native callback that queued each
// delivery has returned, and its TypeScript half dispatches each delivery by
// its token and type. Native code copies every payload, answers every
// synchronous decision itself, and retires each registration with
// MLN_DELIVERY_TYPE_RETIRED, so the addon keeps nothing per callback.

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

// What the threadsafe function's data carries: a token shifted left by two
// with the message kind in the low bits, or, with both bits clear, a pointer
// to a leak report. The TypeScript half's receive() takes the same kinds.
enum {
  MLN_TS_MESSAGE_WAKE = 1,
  MLN_TS_MESSAGE_RELEASE = 2,
  MLN_TS_MESSAGE_LEAK = 3,
  MLN_TS_MESSAGE_MASK = 3,
};

// Tokens stay below 2^31, so a shifted token fits the data on every target.
#define MLN_TS_MAX_TOKEN 0x7fffffff

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

/** The user_data of one wake: its realm's shim and its token. */
typedef struct mln_ts_wake_context {
  mln_ts_shim* shim;
  uint32_t token;
} mln_ts_wake_context;

/** An owner that the collector reclaimed while open. */
typedef struct mln_ts_leak {
  uint64_t handle;
  char name[64];
} mln_ts_leak;

struct mln_ts_realm {
  napi_env env;
  mln_ts_shim* shim;
  // Created by the first completion or queued registration.
  mln_delivery_queue queue;
  // The batch whose deliveries the realm dispatches now, or zero.
  mln_delivery_batch batch;
  mln_delivery_batch_view view;
  napi_ref receive;
  napi_ref register_value;
  napi_ref make_error;
  napi_ref take_native;
  bool referenced;
  bool cleaned;
  // The owners whose tokens this realm's teardown finalizes. The exit path
  // reads them from another thread, so they are guarded.
  pthread_mutex_t owners_lock;
  mln_ts_owner* owners;
  mln_ts_realm* next_realm;
  const char* digest;
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

static void* message(uint32_t token, uintptr_t kind) {
  return (void*)(((uintptr_t)token << 2) | kind);
}

static mln_ts_wake_context* wake_context_create(
  mln_ts_shim* shim, uint32_t token
) {
  mln_ts_wake_context* context = malloc(sizeof *context);
  if (context != NULL) {
    context->shim = shim_retain(shim);
    context->token = token;
  }
  return context;
}

static void wake_context_free(void* user_data) {
  mln_ts_wake_context* context = user_data;
  shim_release(context->shim);
  free(context);
}

void mln_ts_wake(void* user_data) {
  mln_ts_wake_context* context = user_data;
  shim_post(context->shim, message(context->token, MLN_TS_MESSAGE_WAKE));
}

void mln_ts_wake_release(void* user_data) {
  mln_ts_wake_context* context = user_data;
  shim_post(context->shim, message(context->token, MLN_TS_MESSAGE_RELEASE));
  wake_context_free(context);
}

/** Reports an exception that a delivery left pending, as an uncaught one. */
static void surface_exception(napi_env env) {
  bool pending = false;
  if (napi_is_exception_pending(env, &pending) == napi_ok && pending) {
    napi_value error;
    napi_get_and_clear_last_exception(env, &error);
    napi_fatal_exception(env, error);
  }
}

/**
 * Receives one message on the realm's JavaScript thread and passes it to the
 * TypeScript half, which catches every error of its own. A host that tears the
 * function down passes no environment, and a leak report is then only freed.
 */
static void receive(
  napi_env env, napi_value callback, void* context, void* data
) {
  (void)callback;
  uintptr_t bits = (uintptr_t)data;
  mln_ts_leak* leak = (bits & MLN_TS_MESSAGE_MASK) == 0 ? data : NULL;
  mln_ts_realm* realm = context;
  napi_value target = env != NULL ? reference_value(env, realm->receive) : NULL;
  napi_value global;
  if (target != NULL && napi_get_global(env, &global) == napi_ok) {
    napi_handle_scope scope;
    if (napi_open_handle_scope(env, &scope) == napi_ok) {
      napi_value argv[3];
      size_t argc = 2;
      if (leak != NULL) {
        argv[0] = mln_ts_number(env, MLN_TS_MESSAGE_LEAK);
        argv[1] = mln_ts_text(env, leak->name, strlen(leak->name));
        argv[2] = mln_ts_u64(env, leak->handle);
        argc = 3;
      } else {
        argv[0] = mln_ts_number(env, (double)(bits & MLN_TS_MESSAGE_MASK));
        argv[1] = mln_ts_number(env, (double)(bits >> 2));
      }
      if (
        napi_call_function(env, global, target, argc, argv, NULL) != napi_ok
      ) {
        surface_exception(env);
      }
      napi_close_handle_scope(env, scope);
    }
  }
  free(leak);
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
    mln_ts_wake_context* context = wake_context_create(realm->shim, 0);
    if (context == NULL) {
      napi_throw_error(call->env, NULL, "delivery queue allocation failed");
      return false;
    }
    mln_delivery_queue_options options = {.size = sizeof options};
    options.wake.callback = mln_ts_wake;
    options.wake.user_data = context;
    // The queue's own release needs no message: the realm releases it.
    options.wake.release_user_data = wake_context_free;
    mln_diagnostic diagnostic = {.size = sizeof diagnostic};
    mln_delivery_queue queue = MLN_HANDLE_NULL;
    mln_status status =
      mln_delivery_queue_create(&options, &queue, &diagnostic);
    if (status != MLN_STATUS_OK) {
      // A rejected wake stays with the caller.
      wake_context_free(context);
      return mln_ts_check(call, status, &diagnostic);
    }
    realm->queue = queue;
  }
  *out = realm->queue;
  return true;
}

/** Registers a host value with the TypeScript half's current call. */
static bool register_value(
  mln_ts_call* call, napi_value value, uint32_t* out_token, const char* path
) {
  napi_value target = reference_value(call->env, call->realm->register_value);
  napi_value global;
  napi_value token;
  if (
    target == NULL || napi_get_global(call->env, &global) != napi_ok ||
    napi_call_function(call->env, global, target, 1, &value, &token) != napi_ok
  ) {
    return false;
  }
  int64_t number = 0;
  if (!mln_ts_read_int(call, token, 1, MLN_TS_MAX_TOKEN, &number, path)) {
    return false;
  }
  *out_token = (uint32_t)number;
  return true;
}

bool mln_ts_read_registration(
  mln_ts_call* call, napi_value value, void** out_user_data,
  mln_delivery_queue* out_queue, const char* path
) {
  uint32_t token = 0;
  if (
    !register_value(call, value, &token, path) || !realm_queue(call, out_queue)
  ) {
    return false;
  }
  *out_user_data = (void*)(uintptr_t)token;
  return true;
}

bool mln_ts_read_wake(
  mln_ts_call* call, napi_value value, void** out_user_data, const char* path
) {
  *out_user_data = NULL;
  if (mln_ts_is_absent(call, value)) {
    return true;
  }
  uint32_t token = 0;
  if (
    !mln_ts_read_function(call, value, path) ||
    !register_value(call, value, &token, path)
  ) {
    return false;
  }
  mln_ts_wake_context* context = wake_context_create(call->realm->shim, token);
  if (context == NULL) {
    napi_throw_error(call->env, NULL, "wake allocation failed");
    return false;
  }
  if (!mln_ts_reject_later(call, wake_context_free, context)) {
    return false;
  }
  *out_user_data = context;
  return true;
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
  napi_value target = reference_value(env, call->realm->take_native);
  napi_value global;
  napi_value fields;
  napi_value argv[3] = {
    value, mln_ts_text(env, kind, strlen(kind)),
    mln_ts_text(env, path, strlen(path))
  };
  if (
    target == NULL || argv[1] == NULL || argv[2] == NULL ||
    napi_get_global(env, &global) != napi_ok ||
    napi_call_function(env, global, target, 3, argv, &fields) != napi_ok
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

bool mln_ts_read_completion(
  mln_ts_call* call, napi_value token, bool discardable,
  mln_completion* storage, const mln_completion** out
) {
  *out = NULL;
  if (discardable && mln_ts_is_absent(call, token)) {
    return true;
  }
  int64_t number = 0;
  mln_delivery_queue queue = MLN_HANDLE_NULL;
  if (
    !mln_ts_read_int(call, token, 1, MLN_TS_MAX_TOKEN, &number, "completion") ||
    !realm_queue(call, &queue)
  ) {
    return false;
  }
  // A rejected submission queues nothing, so the completion needs no cleanup.
  memset(storage, 0, sizeof *storage);
  storage->size = sizeof *storage;
  storage->user_data = (void*)(uintptr_t)number;
  storage->queue = queue;
  *out = storage;
  return true;
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
 * drain() moves every queued delivery into a batch that the realm keeps until
 * batchRelease(), and returns one [type, token, payload, error] entry per
 * delivery: a completion's status, generations, and diagnostic,
 * or another delivery's copied arguments, or the error that converting them
 * threw. It returns undefined when nothing is queued, and null when the batch
 * could not be allocated, in which case the deliveries stay queued.
 */
static napi_value drain(napi_env env, napi_callback_info info) {
  mln_ts_call call;
  if (!mln_ts_begin(&call, env, info, 0)) {
    return NULL;
  }
  mln_ts_realm* realm = call.realm;
  if (realm->batch != MLN_HANDLE_NULL) {
    napi_throw_error(env, NULL, "the realm already holds a drained batch");
    return mln_ts_end(&call);
  }
  if (realm->queue == MLN_HANDLE_NULL) {
    return mln_ts_end(&call);
  }
  mln_delivery_batch batch = MLN_HANDLE_NULL;
  mln_delivery_batch_view view = {.size = sizeof view};
  mln_diagnostic diagnostic = {.size = sizeof diagnostic};
  mln_status status =
    mln_delivery_queue_drain(realm->queue, &batch, &view, &diagnostic);
  if (status == MLN_STATUS_NOT_READY) {
    return mln_ts_end(&call);
  }
  if (status == MLN_STATUS_NATIVE_ERROR) {
    napi_get_null(env, &call.result);
    return mln_ts_end(&call);
  }
  if (!mln_ts_check(&call, status, &diagnostic)) {
    return mln_ts_end(&call);
  }
  if (view.delivery_size < sizeof(mln_delivery)) {
    mln_delivery_batch_release(batch);
    napi_throw_range_error(env, NULL, "native delivery stride is too small");
    return mln_ts_end(&call);
  }
  realm->batch = batch;
  realm->view = view;
  napi_value entries = mln_ts_array(env, view.delivery_count);
  for (size_t index = 0; entries != NULL && index < view.delivery_count;
       index++) {
    const mln_delivery* delivery =
      (const mln_delivery*)((const char*)view.deliveries +
                            index * view.delivery_size);
    // The TypeScript half names the two protocol types, which carry no
    // callback's arguments, and dispatches the rest by number.
    napi_value type = mln_ts_number(env, delivery->type);
    napi_value payload = mln_ts_undefined(env);
    if (delivery->type == MLN_DELIVERY_TYPE_COMPLETION) {
      type = mln_ts_text(env, "completion", strlen("completion"));
      payload = completion_header(env, &delivery->payload.completion);
    } else if (delivery->type == MLN_DELIVERY_TYPE_RETIRED) {
      type = mln_ts_text(env, "retired", strlen("retired"));
    } else {
      payload = mln_ts_delivery_payload(env, delivery);
    }
    napi_value entry = mln_ts_array(env, 4);
    if (payload == NULL) {
      napi_value error;
      napi_get_and_clear_last_exception(env, &error);
      mln_ts_set_index(env, entry, 3, error);
      payload = mln_ts_undefined(env);
    }
    if (
      !mln_ts_set_index(env, entry, 0, type) ||
      !mln_ts_set_index(
        env, entry, 1,
        mln_ts_number(env, (double)(uintptr_t)delivery->user_data)
      ) ||
      !mln_ts_set_index(env, entry, 2, payload) ||
      !mln_ts_set_index(env, entries, (uint32_t)index, entry)
    ) {
      entries = NULL;
    }
  }
  call.result = entries;
  return mln_ts_end(&call);
}

/** The drained delivery at the call's first argument. */
static const mln_delivery* delivery_at(mln_ts_call* call, size_t* out_index) {
  mln_ts_realm* realm = call->realm;
  int64_t index = 0;
  if (realm->batch == MLN_HANDLE_NULL || realm->view.delivery_count == 0) {
    napi_throw_error(call->env, NULL, "the realm holds no drained batch");
    return NULL;
  }
  if (!mln_ts_read_int(
        call, call->argv[0], 0, (int64_t)realm->view.delivery_count - 1, &index,
        "index"
      )) {
    return NULL;
  }
  *out_index = (size_t)index;
  return (const mln_delivery*)((const char*)realm->view.deliveries +
                               (size_t)index * realm->view.delivery_size);
}

static bool take(mln_ts_call* call, size_t index) {
  mln_diagnostic diagnostic = {.size = sizeof diagnostic};
  return mln_ts_check(
    call, mln_delivery_batch_take(call->realm->batch, index, &diagnostic),
    &diagnostic
  );
}

/**
 * completionValue(index, decoder, take) converts a drained completion's value
 * through the decoder that the TypeScript half names, then takes its owned
 * handle from the batch when take is set.
 */
static napi_value completion_value(napi_env env, napi_callback_info info) {
  mln_ts_call call;
  if (!mln_ts_begin(&call, env, info, 3)) {
    return NULL;
  }
  size_t index = 0;
  int64_t decoder = 0;
  bool taken = false;
  const mln_delivery* delivery = delivery_at(&call, &index);
  if (
    delivery != NULL &&
    mln_ts_read_int(
      &call, call.argv[1], 0, (int64_t)mln_ts_decoder_count - 1, &decoder,
      "decoder"
    ) &&
    mln_ts_read_bool(&call, call.argv[2], &taken, "take")
  ) {
    if (delivery->type != MLN_DELIVERY_TYPE_COMPLETION) {
      napi_throw_error(env, NULL, "the delivery is not a completion");
    } else {
      // Decode before taking, so a value that fails to convert stays in the
      // batch, and batchRelease disposes it.
      napi_value value =
        mln_ts_decoders[decoder](env, &delivery->payload.completion);
      if (value != NULL && (!taken || take(&call, index))) {
        call.result = value;
      }
    }
  }
  return mln_ts_end(&call);
}

/** deliveryTake(index) takes a drained delivery's owned handles. */
static napi_value delivery_take(napi_env env, napi_callback_info info) {
  mln_ts_call call;
  if (!mln_ts_begin(&call, env, info, 1)) {
    return NULL;
  }
  size_t index = 0;
  if (delivery_at(&call, &index) != NULL) {
    take(&call, index);
  }
  return mln_ts_end(&call);
}

/**
 * batchRelease() releases the drained batch, which disposes what the realm
 * did not take: an owned completion value, or a request, which then fails or
 * keeps its URL.
 */
static napi_value batch_release(napi_env env, napi_callback_info info) {
  (void)info;
  mln_ts_realm* realm = mln_ts_realm_of(env);
  if (realm != NULL && realm->batch != MLN_HANDLE_NULL) {
    mln_delivery_batch_release(realm->batch);
    realm->batch = MLN_HANDLE_NULL;
  }
  return mln_ts_undefined(env);
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
  // Releasing the queue disposes every undelivered owned value and fails
  // every undelivered request, and later pushes are refused.
  if (realm->batch != MLN_HANDLE_NULL) {
    mln_delivery_batch_release(realm->batch);
    realm->batch = MLN_HANDLE_NULL;
  }
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
  napi_delete_reference(realm->env, realm->receive);
  napi_delete_reference(realm->env, realm->register_value);
  napi_delete_reference(realm->env, realm->make_error);
  napi_delete_reference(realm->env, realm->take_native);
  realm->receive = realm->register_value = realm->make_error = NULL;
  realm->take_native = NULL;
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

// install(receive, register, makeError, takeNative)
static napi_value install(napi_env env, napi_callback_info info) {
  mln_ts_call call;
  if (!mln_ts_begin(&call, env, info, 4)) {
    return NULL;
  }
  // A second copy of the binding in this realm would number its tokens from
  // the same start, and the copies would receive each other's deliveries.
  if (call.realm->receive != NULL) {
    napi_throw_error(
      env, NULL,
      "another copy of the binding already uses the addon in this realm; "
      "load one copy of @maplibre/native-ffi per realm"
    );
    return mln_ts_end(&call);
  }
  napi_ref* targets[4] = {
    &call.realm->receive, &call.realm->register_value, &call.realm->make_error,
    &call.realm->take_native
  };
  for (size_t index = 0; index < 4; index++) {
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

static napi_value abi_digest(napi_env env, napi_callback_info info) {
  (void)info;
  const char* digest = mln_ts_realm_of(env)->digest;
  return mln_ts_text(env, digest, strlen(digest));
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

// Native threads of the embedded core outlive any one realm, so the module
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
  size_t generated_count, const char* digest
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
  realm->digest = digest;
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
  // A wake or a registration keeps nothing alive; pending Promises reference
  // the function through setReferenced().
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
    {"abiDigest", abi_digest},
    {"setReferenced", set_referenced},
    {"ownerTokenCreate", owner_token_create},
    {"shutdownAtExit", shutdown_at_exit},
    {"drain", drain},
    {"completionValue", completion_value},
    {"deliveryTake", delivery_take},
    {"batchRelease", batch_release},
  };
  define(
    env, exports, runtime_exports,
    sizeof runtime_exports / sizeof runtime_exports[0]
  );
  define(env, exports, generated, generated_count);
  return exports;
}
