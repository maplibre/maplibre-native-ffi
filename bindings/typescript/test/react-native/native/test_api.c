// Test-only Node-API extensions for graphics_addon.c. Its work item is empty:
// a TSFN pump supplies the asynchronous completion without a worker pool.
#include <math.h>
#include <node_api.h>
#include <stdlib.h>

struct napi_async_work__ {
  napi_async_execute_callback execute;
  napi_async_complete_callback complete;
  void* data;
  napi_threadsafe_function function;
};

napi_status napi_create_uint32(
  napi_env env, uint32_t value, napi_value* result
) {
  return napi_create_double(env, value, result);
}
napi_status napi_get_value_uint32(
  napi_env env, napi_value value, uint32_t* result
) {
  double number;
  napi_status status = napi_get_value_double(env, value, &number);
  if (status != napi_ok) return status;
  double integer = isfinite(number) ? fmod(trunc(number), 4294967296.0) : 0;
  *result = (uint32_t)(integer < 0 ? integer + 4294967296.0 : integer);
  return napi_ok;
}
static void receive(
  napi_env env, napi_value callback, void* context, void* data
) {
  (void)callback;
  (void)data;
  napi_async_work work = context;
  work->complete(env, env ? napi_ok : napi_cancelled, work->data);
}
napi_status napi_create_async_work(
  napi_env env, napi_value resource, napi_value name,
  napi_async_execute_callback execute, napi_async_complete_callback complete,
  void* data, napi_async_work* result
) {
  napi_async_work work = calloc(1, sizeof *work);
  if (!work) return napi_generic_failure;
  work->execute = execute;
  work->complete = complete;
  work->data = data;
  napi_status status = napi_create_threadsafe_function(
    env, NULL, resource, name, 0, 1, NULL, NULL, work, receive, &work->function
  );
  if (status != napi_ok) {
    free(work);
    return status;
  }
  *result = work;
  return napi_ok;
}
napi_status napi_queue_async_work(napi_env env, napi_async_work work) {
  work->execute(env, work->data);
  napi_status status =
    napi_call_threadsafe_function(work->function, NULL, napi_tsfn_nonblocking);
  if (status != napi_ok) return status;
  return napi_release_threadsafe_function(work->function, napi_tsfn_release);
}
napi_status napi_delete_async_work(napi_env env, napi_async_work work) {
  (void)env;
  free(work);
  return napi_ok;
}
