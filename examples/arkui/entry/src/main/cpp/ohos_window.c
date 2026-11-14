// Gives ArkTS the OHNativeWindow of an XComponent's surface, which the
// library's OpenHarmony window target takes. ArkTS sees only the surface's ID.

#include <napi/native_api.h>
#include <native_window/external_window.h>
#include <stdbool.h>
#include <stdint.h>

static bool bigint_argument(
  napi_env env, napi_callback_info info, const char* name, uint64_t* out
) {
  size_t argc = 1;
  napi_value argv[1];
  bool lossless = false;
  if (
    napi_get_cb_info(env, info, &argc, argv, NULL, NULL) != napi_ok ||
    argc < 1 ||
    napi_get_value_bigint_uint64(env, argv[0], out, &lossless) != napi_ok ||
    !lossless
  ) {
    napi_throw_type_error(env, NULL, name);
    return false;
  }
  return true;
}

// windowFromSurfaceId(surfaceId: bigint): bigint
//
// Returns a new reference to the surface's window, as its address.
static napi_value window_from_surface_id(
  napi_env env, napi_callback_info info
) {
  uint64_t surface_id = 0;
  if (!bigint_argument(
        env, info, "pass the surface ID as a bigint", &surface_id
      )) {
    return NULL;
  }
  OHNativeWindow* window = NULL;
  int32_t status =
    OH_NativeWindow_CreateNativeWindowFromSurfaceId(surface_id, &window);
  if (status != 0 || window == NULL) {
    napi_throw_error(env, NULL, "the surface ID names no window");
    return NULL;
  }
  napi_value result = NULL;
  if (
    napi_create_bigint_uint64(env, (uint64_t)(uintptr_t)window, &result) !=
    napi_ok
  ) {
    OH_NativeWindow_DestroyNativeWindow(window);
    return NULL;
  }
  return result;
}

// releaseWindow(window: bigint): void
//
// Releases a reference that windowFromSurfaceId returned.
static napi_value release_window(napi_env env, napi_callback_info info) {
  uint64_t window = 0;
  if (!bigint_argument(env, info, "pass the window as a bigint", &window)) {
    return NULL;
  }
  OH_NativeWindow_DestroyNativeWindow((OHNativeWindow*)(uintptr_t)window);
  return NULL;
}

static napi_value init(napi_env env, napi_value exports) {
  napi_property_descriptor properties[] = {
    {"windowFromSurfaceId", NULL, window_from_surface_id, NULL, NULL, NULL,
     napi_default, NULL},
    {"releaseWindow", NULL, release_window, NULL, NULL, NULL, napi_default,
     NULL},
  };
  napi_define_properties(
    env, exports, sizeof(properties) / sizeof(properties[0]), properties
  );
  return exports;
}

// OpenHarmony loads `libohos_window.so` for the module that pages import by
// that name, and the module registers itself from a constructor.
static napi_module module = {
  .nm_version = 1,
  .nm_register_func = init,
  .nm_modname = "ohos_window",
};

__attribute__((constructor)) static void register_module(void) {
  napi_module_register(&module);
}
