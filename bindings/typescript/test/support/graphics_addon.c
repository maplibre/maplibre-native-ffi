// A test-only Node-API addon over libmln_test_graphics, which creates the GPU
// objects that a host would otherwise bring. Graphics objects cross as bigint
// addresses, so one source serves every host without a foreign-function layer.
//
//   create(backend) -> { graphics, backend, eglDisplay, ..., vulkanQueue }
//   makeCurrent(graphics) -> boolean
//   destroy(graphics)

#include <node_api.h>
#include <stdint.h>

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

NAPI_MODULE_EXPORT napi_value
napi_register_module_v1(napi_env env, napi_value exports) {
  const struct {
    const char* name;
    napi_callback function;
  } entries[] = {
    {"create", create},
    {"makeCurrent", make_current},
    {"destroy", destroy},
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
