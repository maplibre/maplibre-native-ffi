#include <node_api.h>
#include <stdint.h>
#include <stdlib.h>

static void check(napi_status status) {
  if (status != napi_ok) abort();
}

static napi_value argument(napi_env env, napi_callback_info info) {
  napi_value value;
  size_t count = 1;
  check(napi_get_cb_info(env, info, &count, &value, NULL, NULL));
  if (count != 1) abort();
  return value;
}

static napi_value signed_bigint(napi_env env, napi_callback_info info) {
  int64_t value;
  bool lossless;
  check(
    napi_get_value_bigint_int64(env, argument(env, info), &value, &lossless)
  );
  if (!lossless) {
    napi_throw_range_error(env, NULL, "lossy signed bigint");
    return NULL;
  }
  napi_value result;
  check(napi_create_bigint_int64(env, value, &result));
  return result;
}

static napi_value unsigned_bigint(napi_env env, napi_callback_info info) {
  uint64_t value;
  bool lossless;
  check(
    napi_get_value_bigint_uint64(env, argument(env, info), &value, &lossless)
  );
  if (!lossless) {
    napi_throw_range_error(env, NULL, "lossy unsigned bigint");
    return NULL;
  }
  napi_value result;
  check(napi_create_bigint_uint64(env, value, &result));
  return result;
}

static napi_value bytes(napi_env env, napi_callback_info info) {
  napi_value view = argument(env, info);
  bool typed;
  check(napi_is_typedarray(env, view, &typed));
  if (!typed) {
    napi_throw_type_error(env, NULL, "expected a typed array");
    return NULL;
  }
  size_t length, offset;
  void* data;
  napi_typedarray_type type;
  check(
    napi_get_typedarray_info(env, view, &type, &length, &data, NULL, &offset)
  );
  napi_value result, item;
  check(napi_create_array_with_length(env, 4, &result));
  double values[] = {
    (double)type, (double)length, (double)offset,
    length ? ((uint8_t*)data)[0] : -1
  };
  for (uint32_t i = 0; i < 4; ++i) {
    check(napi_create_double(env, values[i], &item));
    check(napi_set_element(env, result, i, item));
  }
  return result;
}

static napi_value create_bytes(napi_env env, napi_callback_info info) {
  (void)info;
  napi_value buffer, result;
  void* data;
  check(napi_create_arraybuffer(env, 8, &data, &buffer));
  for (size_t i = 0; i < 8; ++i) ((uint8_t*)data)[i] = (uint8_t)i;
  check(napi_create_typedarray(env, napi_uint8_array, 3, buffer, 2, &result));
  return result;
}

static napi_value pending(napi_env env, napi_callback_info info) {
  napi_value value = argument(env, info);
  napi_handle_scope scope;
  check(napi_open_handle_scope(env, &scope));
  napi_value ignored;
  if (
    napi_get_named_property(env, value, "throwing", &ignored) !=
    napi_pending_exception
  )
    abort();
  bool is_pending;
  check(napi_is_exception_pending(env, &is_pending));
  if (!is_pending) abort();
  check(napi_close_handle_scope(env, scope));
  napi_value error;
  check(napi_get_and_clear_last_exception(env, &error));
  check(napi_throw(env, error));
  return NULL;
}

static napi_value call(napi_env env, napi_callback_info info) {
  napi_value function = argument(env, info);
  napi_value global, result;
  check(napi_get_global(env, &global));
  if (napi_call_function(env, global, function, 0, NULL, &result) != napi_ok)
    return NULL;
  return result;
}

static napi_value external(napi_env env, napi_callback_info info) {
  void* data;
  napi_value result;
  if (napi_get_value_external(env, argument(env, info), &data) != napi_ok) {
    napi_throw_type_error(env, NULL, "expected our external");
    return NULL;
  }
  check(napi_get_boolean(env, data == (void*)(uintptr_t)42, &result));
  return result;
}

napi_value mln_rn_test_init(napi_env env, napi_value exports) {
  const char* names[] = {"signed",  "unsigned", "bytes",   "createBytes",
                         "pending", "call",     "external"};
  napi_callback callbacks[] = {signed_bigint, unsigned_bigint, bytes,
                               create_bytes,  pending,         call,
                               external};
  for (size_t i = 0; i < sizeof names / sizeof *names; ++i) {
    napi_value function;
    check(napi_create_function(
      env, names[i], NAPI_AUTO_LENGTH, callbacks[i], NULL, &function
    ));
    check(napi_set_named_property(env, exports, names[i], function));
  }
  napi_value token;
  check(napi_create_external(env, (void*)(uintptr_t)42, NULL, NULL, &token));
  check(napi_set_named_property(env, exports, "token", token));
  return exports;
}
