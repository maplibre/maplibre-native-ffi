#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "runtime.h"

struct mln_ts_allocation {
  mln_ts_allocation* next;
  max_align_t data[];
};

struct mln_ts_rejection {
  mln_ts_rejection* next;
  void (*reject)(void* data);
  void* data;
};

// ---------------------------------------------------------------------------
// Calls

bool mln_ts_begin(
  mln_ts_call* call, napi_env env, napi_callback_info info, size_t arguments
) {
  memset(call, 0, sizeof *call);
  call->env = env;
  call->argc = MLN_TS_MAX_ARGUMENTS;
  if (
    napi_get_cb_info(env, info, &call->argc, call->argv, NULL, NULL) != napi_ok
  ) {
    return false;
  }
  // Missing arguments read as undefined.
  for (size_t index = call->argc; index < arguments; index++) {
    call->argv[index] = mln_ts_undefined(env);
  }
  call->realm = mln_ts_realm_of(env);
  if (call->realm == NULL) {
    napi_throw_error(env, NULL, "the addon realm is not initialized");
    return false;
  }
  return true;
}

void mln_ts_accept(mln_ts_call* call) {
  while (call->rejections != NULL) {
    mln_ts_rejection* next = call->rejections->next;
    free(call->rejections);
    call->rejections = next;
  }
}

napi_value mln_ts_end(mln_ts_call* call) {
  // Native code kept nothing that the call did not accept, so this thread
  // still owns every wake context.
  while (call->rejections != NULL) {
    mln_ts_rejection* next = call->rejections->next;
    call->rejections->reject(call->rejections->data);
    free(call->rejections);
    call->rejections = next;
  }
  while (call->allocations != NULL) {
    mln_ts_allocation* next = call->allocations->next;
    free(call->allocations);
    call->allocations = next;
  }
  bool pending = false;
  napi_is_exception_pending(call->env, &pending);
  if (pending) {
    return NULL;
  }
  return call->result != NULL ? call->result : mln_ts_undefined(call->env);
}

bool mln_ts_reject_later(
  mln_ts_call* call, void (*reject)(void* data), void* data
) {
  mln_ts_rejection* rejection = malloc(sizeof *rejection);
  if (rejection == NULL) {
    reject(data);
    napi_throw_error(call->env, NULL, "registration allocation failed");
    return false;
  }
  rejection->reject = reject;
  rejection->data = data;
  rejection->next = call->rejections;
  call->rejections = rejection;
  return true;
}

void* mln_ts_allocate(mln_ts_call* call, size_t size) {
  if (size == 0) {
    size = 1;
  }
  mln_ts_allocation* allocation = calloc(1, sizeof *allocation + size);
  if (allocation == NULL) {
    napi_throw_error(call->env, NULL, "call memory allocation failed");
    return NULL;
  }
  allocation->next = call->allocations;
  call->allocations = allocation;
  return allocation->data;
}

bool mln_ts_read_owner(mln_ts_call* call, napi_value value) {
  if (mln_ts_is_absent(call, value)) {
    return true;
  }
  if (
    napi_get_value_external(call->env, value, (void**)&call->owner) != napi_ok
  ) {
    napi_throw_type_error(call->env, NULL, "token: expected an owner token");
    return false;
  }
  return true;
}

void mln_ts_consume(mln_ts_call* call) {
  if (call->owner != NULL) {
    mln_ts_owner_consume(call->owner);
  }
}

bool mln_ts_check(
  mln_ts_call* call, mln_status status, const mln_diagnostic* diagnostic
) {
  if (status == MLN_STATUS_OK) {
    return true;
  }
  mln_ts_throw_status(
    call->env, status, diagnostic != NULL ? diagnostic->message : ""
  );
  return false;
}

static bool fail_as(
  mln_ts_call* call, bool range, const char* path, const char* expected
) {
  char message[256];
  snprintf(message, sizeof message, "%s: expected %s", path, expected);
  (range ? napi_throw_range_error
         : napi_throw_type_error)(call->env, NULL, message);
  return false;
}

static bool fail_type(
  mln_ts_call* call, const char* path, const char* expected
) {
  return fail_as(call, false, path, expected);
}

const char* mln_ts_path(
  char* buffer, size_t size, const char* parent, const char* key
) {
  snprintf(buffer, size, "%s.%s", parent, key);
  return buffer;
}

const char* mln_ts_index_path(
  char* buffer, size_t size, const char* parent, size_t index
) {
  snprintf(buffer, size, "%s[%zu]", parent, index);
  return buffer;
}

// ---------------------------------------------------------------------------
// JavaScript to C

static napi_valuetype type_of(mln_ts_call* call, napi_value value) {
  napi_valuetype type = napi_undefined;
  napi_typeof(call->env, value, &type);
  return type;
}

bool mln_ts_is_absent(mln_ts_call* call, napi_value value) {
  napi_valuetype type = type_of(call, value);
  return type == napi_undefined || type == napi_null;
}

bool mln_ts_member(
  mln_ts_call* call, napi_value object, const char* key, napi_value* out
) {
  return napi_get_named_property(call->env, object, key, out) == napi_ok;
}

bool mln_ts_read_record(mln_ts_call* call, napi_value value, const char* path) {
  return type_of(call, value) == napi_object ||
         fail_type(call, path, "an object");
}

bool mln_ts_read_function(
  mln_ts_call* call, napi_value value, const char* path
) {
  return type_of(call, value) == napi_function ||
         fail_type(call, path, "a function");
}

bool mln_ts_read_bool(
  mln_ts_call* call, napi_value value, bool* out, const char* path
) {
  return (type_of(call, value) == napi_boolean &&
          napi_get_value_bool(call->env, value, out) == napi_ok) ||
         fail_type(call, path, "a boolean");
}

bool mln_ts_read_int(
  mln_ts_call* call, napi_value value, int64_t minimum, int64_t maximum,
  int64_t* out, const char* path
) {
  double number = 0;
  if (!mln_ts_read_double(call, value, &number, path)) {
    return false;
  }
  if (
    !(number == floor(number)) || number < (double)minimum ||
    number > (double)maximum
  ) {
    char range[96];
    snprintf(
      range, sizeof range, "an integer from %lld to %lld", (long long)minimum,
      (long long)maximum
    );
    return fail_as(call, true, path, range);
  }
  *out = (int64_t)number;
  return true;
}

bool mln_ts_read_double(
  mln_ts_call* call, napi_value value, double* out, const char* path
) {
  return (type_of(call, value) == napi_number &&
          napi_get_value_double(call->env, value, out) == napi_ok) ||
         fail_type(call, path, "a number");
}

bool mln_ts_read_u64(
  mln_ts_call* call, napi_value value, uint64_t* out, const char* path
) {
  bool lossless = false;
  if (
    type_of(call, value) != napi_bigint ||
    napi_get_value_bigint_uint64(call->env, value, out, &lossless) != napi_ok
  ) {
    return fail_type(call, path, "a bigint");
  }
  return lossless || fail_as(call, true, path, "a bigint from 0 to 2^64-1");
}

bool mln_ts_read_i64(
  mln_ts_call* call, napi_value value, int64_t* out, const char* path
) {
  bool lossless = false;
  if (
    type_of(call, value) != napi_bigint ||
    napi_get_value_bigint_int64(call->env, value, out, &lossless) != napi_ok
  ) {
    return fail_type(call, path, "a bigint");
  }
  return lossless || fail_as(call, true, path, "a bigint from -2^63 to 2^63-1");
}

bool mln_ts_read_text_view(
  mln_ts_call* call, napi_value value, mln_buffer_view* out, const char* path
) {
  size_t length = 0;
  if (
    type_of(call, value) != napi_string ||
    napi_get_value_string_utf8(call->env, value, NULL, 0, &length) != napi_ok
  ) {
    return fail_type(call, path, "a string");
  }
  char* text = mln_ts_allocate(call, length + 1);
  if (text == NULL) {
    return false;
  }
  napi_get_value_string_utf8(call->env, value, text, length + 1, &length);
  out->data = text;
  out->size = length;
  return true;
}

bool mln_ts_read_text(
  mln_ts_call* call, napi_value value, const char** out, const char* path
) {
  mln_buffer_view view = {0};
  if (!mln_ts_read_text_view(call, value, &view, path)) {
    return false;
  }
  if (memchr(view.data, '\0', view.size) != NULL) {
    return fail_type(call, path, "a string without NUL characters");
  }
  *out = view.data;
  return true;
}

bool mln_ts_read_bytes_view(
  mln_ts_call* call, napi_value value, mln_buffer_view* out, const char* path
) {
  bool typed = false;
  napi_typedarray_type type;
  size_t length = 0;
  void* data = NULL;
  if (
    napi_is_typedarray(call->env, value, &typed) != napi_ok || !typed ||
    napi_get_typedarray_info(
      call->env, value, &type, &length, &data, NULL, NULL
    ) != napi_ok ||
    type != napi_uint8_array
  ) {
    return fail_type(call, path, "a Uint8Array");
  }
  // A copy, because a later conversion in the same call runs getters that may
  // detach or resize the buffer.
  void* copy = mln_ts_allocate(call, length);
  if (copy == NULL) {
    return false;
  }
  if (length > 0) {
    memcpy(copy, data, length);
  }
  out->data = copy;
  out->size = length;
  return true;
}

bool mln_ts_read_sized(
  mln_ts_call* call, napi_value value, bool bytes, size_t maximum,
  const void** out_data, size_t* out_size, const char* path
) {
  mln_buffer_view view = {0};
  if (!(bytes ? mln_ts_read_bytes_view
              : mln_ts_read_text_view)(call, value, &view, path)) {
    return false;
  }
  if (view.size > maximum) {
    return fail_as(call, true, path, "a shorter value");
  }
  *out_data = view.data;
  *out_size = view.size;
  return true;
}

bool mln_ts_read_list(
  mln_ts_call* call, napi_value value, size_t element_size, size_t maximum,
  void** out_items, size_t* out_count, const char* path
) {
  bool array = false;
  uint32_t length = 0;
  if (
    napi_is_array(call->env, value, &array) != napi_ok || !array ||
    napi_get_array_length(call->env, value, &length) != napi_ok
  ) {
    return fail_type(call, path, "an array");
  }
  if (length > maximum) {
    return fail_as(call, true, path, "fewer elements");
  }
  *out_items = mln_ts_allocate(call, element_size * length);
  *out_count = length;
  return *out_items != NULL;
}

bool mln_ts_element(
  mln_ts_call* call, napi_value array, size_t index, napi_value* out
) {
  return napi_get_element(call->env, array, (uint32_t)index, out) == napi_ok;
}

int mln_ts_read_kind(
  mln_ts_call* call, napi_value value, const char* const* kinds, int count,
  const char* path
) {
  napi_value member;
  char kind[64] = "";
  size_t length = 0;
  if (
    mln_ts_read_record(call, value, path) &&
    mln_ts_member(call, value, "kind", &member) &&
    napi_get_value_string_utf8(call->env, member, kind, sizeof kind, &length) ==
      napi_ok
  ) {
    for (int index = 0; index < count; index++) {
      if (strcmp(kind, kinds[index]) == 0) {
        return index;
      }
    }
  }
  bool pending = false;
  napi_is_exception_pending(call->env, &pending);
  if (!pending) {
    char child[256];
    fail_type(
      call, mln_ts_path(child, sizeof child, path, "kind"), "a known kind"
    );
  }
  return -1;
}

// ---------------------------------------------------------------------------
// C to JavaScript

napi_value mln_ts_undefined(napi_env env) {
  napi_value value = NULL;
  napi_get_undefined(env, &value);
  return value;
}

napi_value mln_ts_bool(napi_env env, bool value) {
  napi_value result = NULL;
  napi_get_boolean(env, value, &result);
  return result;
}

napi_value mln_ts_number(napi_env env, double value) {
  napi_value result = NULL;
  napi_create_double(env, value, &result);
  return result;
}

napi_value mln_ts_u64(napi_env env, uint64_t value) {
  napi_value result = NULL;
  napi_create_bigint_uint64(env, value, &result);
  return result;
}

napi_value mln_ts_i64(napi_env env, int64_t value) {
  napi_value result = NULL;
  napi_create_bigint_int64(env, value, &result);
  return result;
}

napi_value mln_ts_text(napi_env env, const void* data, size_t size) {
  napi_value result = NULL;
  napi_create_string_utf8(env, size == 0 ? "" : data, size, &result);
  return result;
}

napi_value mln_ts_bytes(napi_env env, const void* data, size_t size) {
  void* target = NULL;
  napi_value buffer = NULL;
  napi_value result = NULL;
  if (napi_create_arraybuffer(env, size, &target, &buffer) != napi_ok) {
    return NULL;
  }
  if (size > 0) {
    memcpy(target, data, size);
  }
  napi_create_typedarray(env, napi_uint8_array, size, buffer, 0, &result);
  return result;
}

napi_value mln_ts_object(napi_env env) {
  napi_value result = NULL;
  napi_create_object(env, &result);
  return result;
}

napi_value mln_ts_array(napi_env env, size_t length) {
  napi_value result = NULL;
  napi_create_array_with_length(env, length, &result);
  return result;
}

bool mln_ts_set(
  napi_env env, napi_value object, const char* key, napi_value value
) {
  // An absent member stays out of the object.
  napi_valuetype type = napi_undefined;
  return object != NULL && value != NULL &&
         napi_typeof(env, value, &type) == napi_ok &&
         (type == napi_undefined ||
          napi_set_named_property(env, object, key, value) == napi_ok);
}

bool mln_ts_set_index(
  napi_env env, napi_value array, uint32_t index, napi_value value
) {
  return array != NULL && value != NULL &&
         napi_set_element(env, array, index, value) == napi_ok;
}

napi_value mln_ts_missing_value(napi_env env) {
  napi_throw_error(env, NULL, "the completion carried no value");
  return NULL;
}

#ifdef __OHOS__
// OpenHarmony registers modules by library name rather than through the
// exported napi_register_module_v1.
NAPI_MODULE_EXPORT napi_value
napi_register_module_v1(napi_env env, napi_value exports);
static napi_module ohos_module = {
  .nm_version = 1,
  .nm_register_func = napi_register_module_v1,
  .nm_modname = "maplibre_native_ffi",
};
__attribute__((constructor)) static void register_ohos_module(void) {
  napi_module_register(&ohos_module);
}
#endif
