// A test-only Node-API addon over libmln_test_graphics, which creates the GPU
// objects that a host would otherwise bring. Graphics objects cross as bigint
// addresses, so one source serves every host without a foreign-function layer.
//
//   create(backend) -> { graphics, backend, eglDisplay, ..., vulkanQueue }
//   makeCurrent(graphics) -> boolean
//   destroy(graphics)
//   nativeProbe(listener) -> { wake, lock, unlock, release, userData }
//
// A native probe stands for native code in the host, which JavaScript cannot
// write: its function pointers and context fill a native registration. Each
// call posts its name to listener on the JavaScript thread, and the release
// posts "release" and ends the probe.

#include <node_api.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "mln_test_graphics.h"

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

static mln_test_graphics* argument(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1] = {NULL};
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  uint64_t value = 0;
  bool lossless = false;
  if (
    argc < 1 ||
    napi_get_value_bigint_uint64(env, argv[0], &value, &lossless) != napi_ok ||
    value == 0
  ) {
    napi_throw_type_error(env, NULL, "expected a graphics object address");
    return NULL;
  }
  return (mln_test_graphics*)(uintptr_t)value;
}

static napi_value create(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1] = {NULL};
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  uint32_t backend = 0;
  if (argc < 1 || napi_get_value_uint32(env, argv[0], &backend) != napi_ok) {
    napi_throw_type_error(env, NULL, "expected a backend number");
    return NULL;
  }
  mln_test_graphics* graphics = mln_test_graphics_create(backend);
  mln_test_graphics_context context = {0};
  if (graphics == NULL || !mln_test_graphics_get_context(graphics, &context)) {
    napi_throw_error(env, NULL, mln_test_graphics_last_error());
    mln_test_graphics_destroy(graphics);
    return NULL;
  }
  napi_value object = NULL;
  napi_value number = NULL;
  napi_create_object(env, &object);
  set(env, object, "graphics", address(env, graphics));
  napi_create_uint32(env, context.backend, &number);
  set(env, object, "backend", number);
  set(env, object, "metalDevice", address(env, context.metal_device));
  set(env, object, "eglDisplay", address(env, context.egl_display));
  set(env, object, "eglConfig", address(env, context.egl_config));
  set(env, object, "eglContext", address(env, context.egl_context));
  set(env, object, "getProcAddress", address(env, context.get_proc_address));
  set(env, object, "vulkanInstance", address(env, context.vulkan_instance));
  set(
    env, object, "vulkanPhysicalDevice",
    address(env, context.vulkan_physical_device)
  );
  set(env, object, "vulkanDevice", address(env, context.vulkan_device));
  set(env, object, "vulkanQueue", address(env, context.vulkan_queue));
  napi_create_uint32(env, context.vulkan_queue_family_index, &number);
  set(env, object, "vulkanQueueFamilyIndex", number);
  set(
    env, object, "vulkanGetInstanceProcAddr",
    address(env, context.vulkan_get_instance_proc_addr)
  );
  set(
    env, object, "vulkanGetDeviceProcAddr",
    address(env, context.vulkan_get_device_proc_addr)
  );
  return object;
}

static napi_value make_current(napi_env env, napi_callback_info info) {
  mln_test_graphics* graphics = argument(env, info);
  if (graphics == NULL) {
    return NULL;
  }
  napi_value result = NULL;
  napi_get_boolean(env, mln_test_graphics_make_current(graphics), &result);
  return result;
}

static napi_value destroy(napi_env env, napi_callback_info info) {
  mln_test_graphics* graphics = argument(env, info);
  if (graphics != NULL) {
    mln_test_graphics_destroy(graphics);
  }
  return NULL;
}

/**
 * The context of one native probe. A call posts only while the function is
 * open, so a realm's teardown, which finalizes the function, ends every post.
 * The function's finalizer and the release each hold a reference.
 */
typedef struct probe {
  pthread_mutex_t lock;
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
    pthread_mutex_destroy(&context->lock);
    free(context);
  }
}

static void probe_post(void* user_data, uintptr_t event, bool release) {
  probe* context = user_data;
  pthread_mutex_lock(&context->lock);
  if (context->open) {
    napi_call_threadsafe_function(
      context->function, (void*)event, napi_tsfn_nonblocking
    );
    if (release) {
      napi_release_threadsafe_function(context->function, napi_tsfn_release);
    }
  }
  pthread_mutex_unlock(&context->lock);
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
  pthread_mutex_lock(&context->lock);
  context->open = false;
  pthread_mutex_unlock(&context->lock);
  probe_unref(context);
}

static napi_value native_probe(napi_env env, napi_callback_info info) {
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
  pthread_mutex_init(&context->lock, NULL);
  atomic_init(&context->references, 2);
  napi_value name = NULL;
  napi_create_string_utf8(env, "native_probe", NAPI_AUTO_LENGTH, &name);
  if (
    napi_create_threadsafe_function(
      env, argv[0], NULL, name, 0, 1, context, probe_finalize, NULL,
      probe_receive, &context->function
    ) != napi_ok
  ) {
    pthread_mutex_destroy(&context->lock);
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

NAPI_MODULE_EXPORT napi_value
napi_register_module_v1(napi_env env, napi_value exports) {
  const struct {
    const char* name;
    napi_callback function;
  } entries[] = {
    {"create", create},
    {"makeCurrent", make_current},
    {"destroy", destroy},
    {"nativeProbe", native_probe},
  };
  for (size_t index = 0; index < sizeof entries / sizeof entries[0]; index++) {
    napi_value function = NULL;
    napi_create_function(
      env, entries[index].name, NAPI_AUTO_LENGTH, entries[index].function, NULL,
      &function
    );
    napi_set_named_property(env, exports, entries[index].name, function);
  }
  return exports;
}

#ifdef __OHOS__
static napi_module ohos_module = {
  .nm_version = 1,
  .nm_register_func = napi_register_module_v1,
  .nm_modname = "maplibre_native_ffi_test_graphics",
};
__attribute__((constructor)) static void register_ohos_module(void) {
  napi_module_register(&ohos_module);
}
#endif
