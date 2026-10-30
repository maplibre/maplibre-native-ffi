// The native probe of the suite's test addons: native code that a host would
// bring, which JavaScript cannot write. Its function pointers and context fill
// a native registration, and each call that native code makes through them
// posts its name to a listener on the JavaScript thread. The release posts
// "release" and ends the probe.
//
// It uses only Node-API, so the desktop graphics addon and the browser test
// module link the same probe.

#include <node_api.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "native_probe.h"

#include "../../native/mutex.h"

static napi_value address(napi_env env, const void* pointer) {
  napi_value value = NULL;
  napi_create_bigint_uint64(env, (uint64_t)(uintptr_t)pointer, &value);
  return value;
}

static void set(
  napi_env env, napi_value object, const char* key, napi_value v
) {
  napi_set_named_property(env, object, key, v);
}

/**
 * The context of one native probe. A call posts only while the function is
 * open, so a realm's teardown, which finalizes the function, ends every post.
 * The function's finalizer and the release each hold a reference.
 */
typedef struct probe {
  mln_ts_mutex lock;
  bool open;
  napi_threadsafe_function function;
  atomic_uint references;
} probe;

static const char* const probe_events[] = {"wake", "lock", "unlock", "release"};

static void probe_unref(probe* context) {
  if (
    atomic_fetch_sub_explicit(&context->references, 1, memory_order_acq_rel) ==
    1
  ) {
    mln_ts_mutex_destroy(&context->lock);
    free(context);
  }
}

static void probe_post(void* user_data, uintptr_t event, bool release) {
  probe* context = user_data;
  mln_ts_mutex_lock(&context->lock);
  if (context->open) {
    napi_call_threadsafe_function(
      context->function, (void*)event, napi_tsfn_nonblocking
    );
    if (release) {
      napi_release_threadsafe_function(context->function, napi_tsfn_release);
    }
  }
  mln_ts_mutex_unlock(&context->lock);
}

static void probe_wake(void* user_data) { probe_post(user_data, 0, false); }
static void probe_lock(void* user_data) { probe_post(user_data, 1, false); }
static void probe_unlock(void* user_data) { probe_post(user_data, 2, false); }

static void probe_release(void* user_data) {
  probe_post(user_data, 3, true);
  probe_unref(user_data);
}

static void probe_receive(
  napi_env env, napi_value listener, void* context, void* data
) {
  (void)context;
  if (env == NULL) {
    return;
  }
  napi_value name = NULL;
  napi_value receiver = NULL;
  napi_create_string_utf8(
    env, probe_events[(uintptr_t)data], NAPI_AUTO_LENGTH, &name
  );
  napi_get_undefined(env, &receiver);
  napi_call_function(env, receiver, listener, 1, &name, NULL);
}

static void probe_finalize(napi_env env, void* data, void* hint) {
  (void)env;
  (void)hint;
  probe* context = data;
  mln_ts_mutex_lock(&context->lock);
  context->open = false;
  mln_ts_mutex_unlock(&context->lock);
  probe_unref(context);
}

napi_value mln_ts_test_native_probe(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1] = {NULL};
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  napi_valuetype type = napi_undefined;
  if (
    argc < 1 || napi_typeof(env, argv[0], &type) != napi_ok ||
    type != napi_function
  ) {
    napi_throw_type_error(env, NULL, "expected a listener function");
    return NULL;
  }
  probe* context = calloc(1, sizeof *context);
  if (context == NULL) {
    napi_throw_error(env, NULL, "native probe allocation failed");
    return NULL;
  }
  mln_ts_mutex_init(&context->lock);
  atomic_init(&context->references, 2);
  napi_value name = NULL;
  napi_create_string_utf8(env, "native_probe", NAPI_AUTO_LENGTH, &name);
  if (
    napi_create_threadsafe_function(
      env, argv[0], NULL, name, 0, 1, context, probe_finalize, NULL,
      probe_receive, &context->function
    ) != napi_ok
  ) {
    mln_ts_mutex_destroy(&context->lock);
    free(context);
    napi_throw_error(env, NULL, "native probe function creation failed");
    return NULL;
  }
  context->open = true;
  // A case waits for a post with a referenced bound, so the probe holds
  // nothing open.
  napi_unref_threadsafe_function(env, context->function);
  napi_value object = NULL;
  napi_create_object(env, &object);
  set(env, object, "wake", address(env, (const void*)(uintptr_t)probe_wake));
  set(env, object, "lock", address(env, (const void*)(uintptr_t)probe_lock));
  set(
    env, object, "unlock", address(env, (const void*)(uintptr_t)probe_unlock)
  );
  set(
    env, object, "release", address(env, (const void*)(uintptr_t)probe_release)
  );
  set(env, object, "userData", address(env, context));
  return object;
}
