// A test-only Node-API addon over libmln_test_graphics, which creates the GPU
// objects that a host would otherwise bring. Graphics objects cross as bigint
// addresses, so one source serves every host without a foreign-function layer.
//
//   create(backend) -> { graphics, backend, eglDisplay, ..., vulkanQueue }
//   makeCurrent(graphics) -> boolean
//   destroy(graphics)
//   nativeProbe(listener) -> { wake, lock, unlock, release, userData }
//   turn(callback)
//
// nativeProbe() is test/support/native_probe.c's.
//
// turn() queues an empty async work item and calls callback when the event
// loop completes it, which is the ArkTS bundle's setImmediate().

#include <node_api.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "mln_test_graphics.h"
#include "native_probe.h"

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
  double backend = 0;
  if (argc < 1 || napi_get_value_double(env, argv[0], &backend) != napi_ok) {
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
  napi_create_double(env, context.backend, &number);
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
  napi_create_double(env, context.vulkan_queue_family_index, &number);
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

#ifdef __OHOS__
/** A turn's empty work item and the callback that its completion calls. */
typedef struct turn_work {
  napi_async_work work;
  napi_ref callback;
} turn_work;

static void turn_execute(napi_env env, void* data) {
  (void)env;
  (void)data;
}

static void turn_complete(napi_env env, napi_status status, void* data) {
  (void)status;
  turn_work* turn = data;
  napi_value callback = NULL;
  napi_value global = NULL;
  if (
    napi_get_reference_value(env, turn->callback, &callback) == napi_ok &&
    callback != NULL && napi_get_global(env, &global) == napi_ok
  ) {
    napi_call_function(env, global, callback, 0, NULL, NULL);
  }
  napi_delete_reference(env, turn->callback);
  napi_delete_async_work(env, turn->work);
  free(turn);
}

static napi_value turn(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1] = {NULL};
  napi_get_cb_info(env, info, &argc, argv, NULL, NULL);
  napi_valuetype type = napi_undefined;
  if (
    argc < 1 || napi_typeof(env, argv[0], &type) != napi_ok ||
    type != napi_function
  ) {
    napi_throw_type_error(env, NULL, "expected a callback function");
    return NULL;
  }
  turn_work* context = calloc(1, sizeof *context);
  napi_value name = NULL;
  napi_create_string_utf8(env, "turn", NAPI_AUTO_LENGTH, &name);
  if (
    context == NULL ||
    napi_create_reference(env, argv[0], 1, &context->callback) != napi_ok ||
    napi_create_async_work(
      env, NULL, name, turn_execute, turn_complete, context, &context->work
    ) != napi_ok ||
    napi_queue_async_work(env, context->work) != napi_ok
  ) {
    if (context != NULL && context->work != NULL) {
      napi_delete_async_work(env, context->work);
    }
    if (context != NULL && context->callback != NULL) {
      napi_delete_reference(env, context->callback);
    }
    free(context);
    napi_throw_error(env, NULL, "turn work could not be queued");
  }
  return NULL;
}
#endif

NAPI_MODULE_EXPORT napi_value
napi_register_module_v1(napi_env env, napi_value exports) {
  const struct {
    const char* name;
    napi_callback function;
  } entries[] = {
    {"create", create},   {"makeCurrent", make_current},
    {"destroy", destroy}, {"nativeProbe", mln_ts_test_native_probe},
#ifdef __OHOS__
    {"turn", turn},
#endif
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
