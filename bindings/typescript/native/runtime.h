// The handwritten runtime of the Node-API addon.
//
// Generated export functions convert their arguments through the helpers here,
// call the C function directly, and return a JavaScript value. Every
// completion, registration, and wake that JavaScript supplies names the realm's
// delivery queue, with a generated trampoline as its callback and a reference
// to its JavaScript value as its user data. The realm dispatches the queue on
// its event loop when the queue's wake posts to the realm's threadsafe
// function, so each trampoline runs on the JavaScript thread. A native
// registration instead carries function pointers from native code in the host,
// which the runtime copies into the C descriptor unchanged. The runtime also
// keeps the owner tokens that finalizers dispose through, and the realm's
// teardown.

#ifndef MAPLIBRE_NATIVE_FFI_NODE_RUNTIME_H
#define MAPLIBRE_NATIVE_FFI_NODE_RUNTIME_H

#include <node_api.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c.h"
#include "maplibre_native_c/binding_support.h"
#include "maplibre_native_c/delivery.h"

#define MLN_TS_MAX_ARGUMENTS 16

typedef struct mln_ts_realm mln_ts_realm;
typedef struct mln_ts_allocation mln_ts_allocation;
typedef struct mln_ts_rejection mln_ts_rejection;
typedef struct mln_ts_owner mln_ts_owner;

/**
 * Converts the value of a successful completion, as the export that submits it
 * names; NULL means an exception.
 */
typedef napi_value (*mln_ts_decoder)(
  napi_env env, const mln_completion_result* result
);

typedef struct mln_ts_export {
  const char* name;
  napi_callback function;
} mln_ts_export;

/** The state of one generated export call. */
typedef struct mln_ts_call {
  napi_env env;
  mln_ts_realm* realm;
  size_t argc;
  napi_value argv[MLN_TS_MAX_ARGUMENTS];
  napi_value result;
  mln_ts_allocation* allocations;
  // What the call releases unless native code accepts it.
  mln_ts_rejection* rejections;
  // The owner token of the handle that the call releases, or NULL.
  mln_ts_owner* owner;
} mln_ts_call;

/** Initializes the realm and defines the generated and runtime exports. */
napi_value mln_ts_module_init(
  napi_env env, napi_value exports, const mln_ts_export* generated,
  size_t generated_count
);

bool mln_ts_begin(
  mln_ts_call* call, napi_env env, napi_callback_info info, size_t arguments
);
/**
 * Ends a call: frees the registrations and completions that native code did
 * not accept, and the call's memory.
 */
napi_value mln_ts_end(mln_ts_call* call);
/** Native code accepted the call's registrations and completion. */
void mln_ts_accept(mln_ts_call* call);
/**
 * Reads the owner token of the handle that the call releases, or undefined
 * for a handle that has no owner yet.
 */
bool mln_ts_read_owner(mln_ts_call* call, napi_value value);
/**
 * The C call consumed the handle: ends its owner token within the same
 * export, so no realm teardown can finalize the consumed handle.
 */
void mln_ts_consume(mln_ts_call* call);
/** Throws a MaplibreError for a failed status and returns false. */
bool mln_ts_check(
  mln_ts_call* call, mln_status status, const mln_diagnostic* diagnostic
);
/** Writes `parent.key` into buffer, for the path that a converter reports. */
const char* mln_ts_path(
  char* buffer, size_t size, const char* parent, const char* key
);
const char* mln_ts_index_path(
  char* buffer, size_t size, const char* parent, size_t index
);
/** Zeroed memory that lives until the call ends, or NULL with a throw. */
void* mln_ts_allocate(mln_ts_call* call, size_t size);
/** Frees call memory that a call handed off, as mln_ts_start_thread does. */
void mln_ts_free_allocations(mln_ts_allocation* allocations);
/** Runs reject(data) when the call ends, unless native code accepted it. */
bool mln_ts_reject_later(
  mln_ts_call* call, void (*reject)(void* data), void* data
);

// JavaScript to C. Each reads one value and throws a TypeError or a RangeError
// that names `path` when the value has the wrong shape.
bool mln_ts_is_absent(mln_ts_call* call, napi_value value);
/** Whether a value is an object, as a native registration is. */
bool mln_ts_is_object(mln_ts_call* call, napi_value value);
bool mln_ts_member(
  mln_ts_call* call, napi_value object, const char* key, napi_value* out
);
bool mln_ts_read_record(mln_ts_call* call, napi_value value, const char* path);
bool mln_ts_read_function(
  mln_ts_call* call, napi_value value, const char* path
);
bool mln_ts_read_bool(
  mln_ts_call* call, napi_value value, bool* out, const char* path
);
bool mln_ts_read_int(
  mln_ts_call* call, napi_value value, int64_t minimum, int64_t maximum,
  int64_t* out, const char* path
);
bool mln_ts_read_double(
  mln_ts_call* call, napi_value value, double* out, const char* path
);
bool mln_ts_read_u64(
  mln_ts_call* call, napi_value value, uint64_t* out, const char* path
);
bool mln_ts_read_i64(
  mln_ts_call* call, napi_value value, int64_t* out, const char* path
);
/** A NUL-terminated copy of a string, which rejects an embedded NUL. */
bool mln_ts_read_text(
  mln_ts_call* call, napi_value value, const char** out, const char* path
);
/** A sized copy of a string, which keeps an embedded NUL. */
bool mln_ts_read_text_view(
  mln_ts_call* call, napi_value value, mln_buffer_view* out, const char* path
);
/**
 * The JSON.stringify text of a value, which lives for the call. undefined is
 * absent and leaves out empty; a value that JSON.stringify rejects throws its
 * error, and one that it skips, such as a function, throws a TypeError.
 */
bool mln_ts_read_json_view(
  mln_ts_call* call, napi_value value, mln_buffer_view* out, const char* path
);
/** A copy of the bytes of a Uint8Array, which lives for the call. */
bool mln_ts_read_bytes_view(
  mln_ts_call* call, napi_value value, mln_buffer_view* out, const char* path
);
/** Bytes or text, as mln_ts_read_bytes_view or mln_ts_read_text_view. */
bool mln_ts_read_sized(
  mln_ts_call* call, napi_value value, bool bytes, size_t maximum,
  const void** out_data, size_t* out_size, const char* path
);
/** Zeroed storage for each element of an array of at most maximum. */
bool mln_ts_read_list(
  mln_ts_call* call, napi_value value, size_t element_size, size_t maximum,
  void** out_items, size_t* out_count, const char* path
);
bool mln_ts_element(
  mln_ts_call* call, napi_value array, size_t index, napi_value* out
);
/** The index of the object's `kind` among kinds, or -1 with a throw. */
int mln_ts_read_kind(
  mln_ts_call* call, napi_value value, const char* const* kinds, int count,
  const char* path
);

// Registrations and completions. Each one names the realm's queue, so its
// trampoline runs while the realm dispatches the queue on its event loop.

/**
 * Registers a host value with the realm's current call, and writes the user
 * data, release, and queue that the registration's trampolines receive. The
 * call frees the user data unless native code accepts it.
 */
bool mln_ts_read_registration(
  mln_ts_call* call, napi_value value, void** out_user_data,
  mln_user_data_release* out_release, mln_delivery_queue* out_queue
);
/**
 * Opens the handle scope of one dispatched call of the registration that
 * user_data names, and returns its environment, or NULL when the scope could
 * not open.
 */
napi_env mln_ts_upcall_begin(void* user_data, napi_handle_scope* scope);
/**
 * Passes argc converted arguments to the host value of the registration
 * through the generated callback adapter that callback numbers, which reports
 * what the value throws, and closes the scope. Returns false when nothing
 * received the arguments: the collector reclaimed the value's owner, or an
 * argument failed to convert, which it reports. The trampoline then releases
 * what the call owns.
 */
bool mln_ts_upcall_end(
  void* user_data, napi_handle_scope scope, uint32_t callback, size_t argc,
  const napi_value* argv
);
/**
 * Takes a native registration of kind, which the realm's TypeScript half
 * consumes, into out: the function pointer of each of its callbacks, then its
 * context and its release. Throws the invalid-state error for a registration
 * that an earlier call consumed. The call releases the context unless native
 * code accepts it.
 */
bool mln_ts_read_native_registration(
  mln_ts_call* call, napi_value value, const char* kind, size_t callbacks,
  uint64_t* out, const char* path
);
/**
 * A completion that calls the function settle with the completion's status,
 * generations, and diagnostic, and the value that decoder converts, or the
 * error that converting it threw. A NULL decoder converts no value. A
 * discardable command takes an undefined settle as no completion, and *out
 * stays NULL. The call frees the completion unless native code accepts it.
 */
bool mln_ts_read_completion(
  mln_ts_call* call, napi_value settle, bool discardable,
  mln_ts_decoder decoder, mln_completion* storage, const mln_completion** out
);

/** Runs a call that waits for a thread to start, and writes its new handle. */
typedef mln_status (*mln_ts_thread_run)(
  const void* arguments, uint64_t* out_handle, mln_diagnostic* diagnostic
);
/**
 * Starts run with a copy of the size bytes at arguments on a detached thread,
 * and returns at once. The thread takes the call's memory, which the copied
 * arguments point into. When run returns, the realm calls the function settle
 * with its status, its handle, and its diagnostic. When the realm has ended by
 * then, the thread disposes a created handle with dispose. Returns false with
 * a throw when the thread could not start.
 */
bool mln_ts_start_thread(
  mln_ts_call* call, napi_value settle, mln_ts_thread_run run,
  void (*dispose)(uint64_t handle), const void* arguments, size_t size
);

// C to JavaScript. Each returns NULL after a failed Node-API call.
napi_value mln_ts_undefined(napi_env env);
napi_value mln_ts_bool(napi_env env, bool value);
napi_value mln_ts_number(napi_env env, double value);
napi_value mln_ts_u64(napi_env env, uint64_t value);
napi_value mln_ts_i64(napi_env env, int64_t value);
napi_value mln_ts_text(napi_env env, const void* data, size_t size);
/** The JSON.parse value of UTF-8 JSON, or undefined for empty text. */
napi_value mln_ts_json(napi_env env, const void* data, size_t size);
napi_value mln_ts_bytes(napi_env env, const void* data, size_t size);
napi_value mln_ts_object(napi_env env);
napi_value mln_ts_array(napi_env env, size_t length);
bool mln_ts_set(
  napi_env env, napi_value object, const char* key, napi_value value
);
bool mln_ts_set_index(
  napi_env env, napi_value array, uint32_t index, napi_value value
);
/** Throws for a successful completion that carried no value. */
napi_value mln_ts_missing_value(napi_env env);

// The realm, which realm.c keeps, for the calls in runtime.c.

/** The realm of an environment, or NULL before initialization. */
mln_ts_realm* mln_ts_realm_of(napi_env env);
/** Calls the realm's error factory and throws its error. */
void mln_ts_throw_status(napi_env env, mln_status status, const char* message);
/** Ends an owner's token, as mln_ts_consume does. */
void mln_ts_owner_consume(mln_ts_owner* owner);

#endif
