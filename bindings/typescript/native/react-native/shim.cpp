// Only the Node-API surface used by the C addon. All VM access is on the JS
// thread; externals and queued pumps may outlive both cleanup and the VM.
#include <algorithm>
#include <array>
#include <cstring>
#include <deque>
#include <limits>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

#include "shim.h"

namespace jsi = facebook::jsi;

struct napi_value__ {
  jsi::Value value;
  explicit napi_value__(jsi::Value v) : value(std::move(v)) {}
};

struct napi_handle_scope__ {
  napi_handle_scope__* parent;
  std::deque<napi_value__> values;
  explicit napi_handle_scope__(napi_handle_scope__* p) : parent(p) {}
};

struct napi_ref__ {
  jsi::Value value;
  explicit napi_ref__(jsi::Value v) : value(std::move(v)) {}
};

struct napi_callback_info__ {
  std::vector<napi_value> args;
  napi_value receiver;
  void* data;
};

struct Finalizer {
  napi_finalize callback;
  void* data;
  void* hint;
};

struct napi_env__ : std::enable_shared_from_this<napi_env__> {
  jsi::Runtime& runtime;
  const std::thread::id jsThread = std::this_thread::get_id();
  std::shared_ptr<facebook::react::CallInvoker> invoker;
  napi_handle_scope arena = nullptr;
  std::unique_ptr<jsi::Value> pending;
  std::unordered_set<napi_ref> references;
  std::vector<std::pair<void (*)(void*), void*>> hooks;
  Finalizer instance = {};
  std::mutex mutex;
  bool closed = false;
  bool pumpPending = false;
  std::vector<Finalizer> finalizers;
  std::vector<std::shared_ptr<napi_threadsafe_function__>> functions;
  // Captured intrinsics, never properties read from a caller's typed array.
  std::array<std::unique_ptr<jsi::Function>, 5> viewGetters;
  std::array<std::unique_ptr<jsi::Function>, 11> viewConstructors;
  std::array<std::unique_ptr<jsi::Function>, 3> errors;

  napi_env__(
    jsi::Runtime& rt, std::shared_ptr<facebook::react::CallInvoker> callInvoker
  )
      : runtime(rt), invoker(std::move(callInvoker)) {}

  napi_value value(jsi::Value v) {
    arena->values.emplace_back(std::move(v));
    return &arena->values.back();
  }
  napi_value copy(const jsi::Value& v) { return value(jsi::Value(runtime, v)); }
  void checkThread() const {
    if (jsThread != std::this_thread::get_id()) std::terminate();
  }
  void throwPending() {
    if (pending) {
      auto exception = std::move(pending);
      throw jsi::JSError(runtime, std::move(*exception));
    }
  }
  void scheduleFinalizer(Finalizer finalizer);
};

struct Scope {
  napi_env env;
  napi_handle_scope__ arena;
  explicit Scope(napi_env e) : env(e), arena(e->arena) { e->arena = &arena; }
  ~Scope() { env->arena = arena.parent; }
};

// No exception may unwind through a C callback. Keep JavaScript's original
// thrown value and rethrow it only after returning to the host-function frame.
template <typename F>
static napi_status protect(napi_env env, F&& body, bool allowPending = false) {
  try {
    if (!env) return napi_invalid_arg;
    env->checkThread();
    if (env->closed) return napi_closing;
    if (env->pending && !allowPending) return napi_pending_exception;
    return body();
  } catch (const jsi::JSError& error) {
    try {
      env->pending = std::make_unique<jsi::Value>(env->runtime, error.value());
    } catch (...) {
      return napi_generic_failure;
    }
    return napi_pending_exception;
  } catch (const std::exception& error) {
    try {
      env->pending = std::make_unique<jsi::Value>(
        env->runtime, jsi::JSError(env->runtime, error.what()).value()
      );
    } catch (...) {
      return napi_generic_failure;
    }
    return napi_pending_exception;
  } catch (...) {
    return napi_generic_failure;
  }
}

#define API(name, signature, ...)              \
  napi_status name signature {                 \
    return protect(env, [&]() -> napi_status { \
      __VA_ARGS__;                             \
      return napi_ok;                          \
    });                                        \
  }

struct External final : jsi::NativeState {
  std::shared_ptr<napi_env__> env;
  Finalizer finalizer;
  External(std::shared_ptr<napi_env__> e, Finalizer f)
      : env(std::move(e)), finalizer(f) {}
  ~External() override { env->scheduleFinalizer(finalizer); }
};

void napi_env__::scheduleFinalizer(Finalizer finalizer) {
  if (!finalizer.callback) return;
  std::unique_lock lock(mutex);
  if (closed) {
    lock.unlock();
    finalizer.callback(nullptr, finalizer.data, finalizer.hint);
    return;
  }
  finalizers.push_back(finalizer);
  if (pumpPending) return;
  pumpPending = true;
  invoker->invokeAsync([env = shared_from_this()]() {
    std::vector<Finalizer> work;
    {
      std::lock_guard pumpLock(env->mutex);
      if (env->closed) return;
      env->pumpPending = false;
      work.swap(env->finalizers);
    }
    Scope scope(env.get());
    for (auto f : work) f.callback(env.get(), f.data, f.hint);
    env->throwPending();
  });
}

struct napi_threadsafe_function__
    : std::enable_shared_from_this<napi_threadsafe_function__> {
  std::shared_ptr<napi_env__> env;
  std::mutex mutex;
  std::deque<void*> queue;
  napi_threadsafe_function_call_js callback;
  void* context;
  Finalizer finalizer;
  size_t maxQueue;
  bool closed = false;
  bool pumpPending = false;
  bool releasing = false;
  bool pumping = false;
  bool finalized = false;
  std::unique_ptr<jsi::Value> listener;

  void finalize() {
    if (finalized) return;
    finalized = true;
    listener.reset();
    if (finalizer.callback)
      finalizer.callback(env.get(), finalizer.data, finalizer.hint);
  }
  void abort() {
    std::deque<void*> work;
    {
      std::lock_guard lock(mutex);
      if (closed) return;
      closed = true;
      work.swap(queue);
    }
    for (auto data : work) callback(nullptr, nullptr, context, data);
    // A C callback may abort while pump() still owns messages. Their null-env
    // delivery needs the context alive until that pump finishes.
    if (!pumping) finalize();
  }

  void pump() {
    env->checkThread();
    std::deque<void*> work;
    {
      std::lock_guard lock(mutex);
      if (closed) return;
      pumping = true;
      // Keep pumpPending set during callbacks: work they enqueue is picked up
      // by the next pump, without a second pump running alongside this one.
      work.swap(queue);
    }
    Scope scope(env.get());
    for (auto data : work) {
      // C callbacks may themselves abort this function.
      bool aborting;
      {
        std::lock_guard lock(mutex);
        aborting = closed;
      }
      auto function = !aborting && listener ? env->copy(*listener) : nullptr;
      callback(aborting ? nullptr : env.get(), function, context, data);
    }
    bool finish;
    {
      std::lock_guard lock(mutex);
      pumpPending = false;
      pumping = false;
      finish = !closed && releasing && queue.empty();
      if (!closed && !queue.empty()) schedule();
    }
    if (finish) abort();
    if (closed) finalize();
    env->throwPending();
  }

  void schedule() {
    pumpPending = true;
    env->invoker->invokeAsync([self = shared_from_this()] { self->pump(); });
  }
};

namespace mln::react_native {

Environment::Environment(
  jsi::Runtime& runtime, std::shared_ptr<facebook::react::CallInvoker> invoker
)
    : env_(std::make_shared<napi_env__>(runtime, std::move(invoker))) {
  const char* names[] = {"Int8Array",     "Uint8Array",    "Uint8ClampedArray",
                         "Int16Array",    "Uint16Array",   "Int32Array",
                         "Uint32Array",   "Float32Array",  "Float64Array",
                         "BigInt64Array", "BigUint64Array"};
  for (size_t i = 0; i < env_->viewConstructors.size(); ++i) {
    env_->viewConstructors[i] = std::make_unique<jsi::Function>(
      runtime.global().getPropertyAsFunction(runtime, names[i])
    );
  }
  // The getters do their own receiver checks, including for forged objects,
  // proxies, subclasses and views with shadowing own properties.
  auto getters = runtime
                   .evaluateJavaScript(
                     std::make_shared<jsi::StringBuffer>(R"JS(
    (() => {
      const p = Object.getPrototypeOf(Uint8Array.prototype);
      const get = key => Object.getOwnPropertyDescriptor(p, key).get;
      return [get('buffer'), get('byteOffset'), get('byteLength'), get('length'),
              get(Symbol.toStringTag)];
    })()
  )JS"),
                     "maplibre-native-ffi:typed-array-intrinsics"
                   )
                   .asObject(runtime)
                   .asArray(runtime);
  for (size_t i = 0; i < env_->viewGetters.size(); ++i) {
    env_->viewGetters[i] = std::make_unique<jsi::Function>(
      getters.getValueAtIndex(runtime, i).asObject(runtime).asFunction(runtime)
    );
  }
  const char* errors[] = {"Error", "TypeError", "RangeError"};
  for (size_t i = 0; i < env_->errors.size(); ++i) {
    env_->errors[i] = std::make_unique<jsi::Function>(
      runtime.global().getPropertyAsFunction(runtime, errors[i])
    );
  }
}

Environment::~Environment() {
  // Platform invalidation, rather than destruction on an arbitrary thread,
  // must have released every JSI value first.
  if (!env_->closed) std::terminate();
}

napi_env Environment::get() const { return env_.get(); }

jsi::Object Environment::install(napi_value (*module)(napi_env, napi_value)) {
  env_->checkThread();
  Scope scope(env_.get());
  auto exports = env_->value(jsi::Object(env_->runtime));
  auto result = module(env_.get(), exports);
  env_->throwPending();
  if (!result)
    throw jsi::JSError(env_->runtime, "the addon could not initialize");
  return result->value.asObject(env_->runtime);
}

void Environment::cleanup() {
  auto env = env_;
  env->checkThread();
  Scope scope(env.get());
  {
    std::lock_guard lock(env->mutex);
    if (env->closed) return;
  }
  // A realm's hook aborts its TSFN and unlinks every owner before its instance
  // finalizer frees the realm. Externals reclaimed meanwhile stay queued until
  // both have finished, so owner_finalize never races the realm's owner list.
  for (auto it = env->hooks.rbegin(); it != env->hooks.rend(); ++it) {
    it->first(it->second);
  }
  env->hooks.clear();
  for (auto function : env->functions) function->abort();
  env->functions.clear();
  if (env->instance.callback) {
    env->instance.callback(env.get(), env->instance.data, env->instance.hint);
  }
  env->instance = {};
  for (auto ref : env->references) delete ref;
  env->references.clear();
  env->pending.reset();
  for (auto& getter : env->viewGetters) getter.reset();
  for (auto& constructor : env->viewConstructors) constructor.reset();
  for (auto& error : env->errors) error.reset();
  std::vector<Finalizer> finalizers;
  {
    std::lock_guard lock(env->mutex);
    env->closed = true;
    finalizers.swap(env->finalizers);
  }
  for (auto f : finalizers) f.callback(nullptr, f.data, f.hint);
}

}  // namespace mln::react_native

API(
  napi_get_undefined, (napi_env env, napi_value* result),
  *result = env->value(jsi::Value::undefined())
)
API(
  napi_get_global, (napi_env env, napi_value* result),
  *result = env->value(env->runtime.global())
)
API(
  napi_get_boolean, (napi_env env, bool v, napi_value* result),
  *result = env->value(jsi::Value(v))
)
API(
  napi_create_double, (napi_env env, double v, napi_value* result),
  *result = env->value(jsi::Value(v))
)
API(
  napi_create_object, (napi_env env, napi_value* result),
  *result = env->value(jsi::Object(env->runtime))
)
API(
  napi_create_array_with_length,
  (napi_env env, size_t length, napi_value* result),
  *result = env->value(jsi::Array(env->runtime, length))
)
API(
  napi_create_bigint_int64, (napi_env env, int64_t v, napi_value* result),
  *result = env->value(jsi::BigInt::fromInt64(env->runtime, v))
)
API(
  napi_create_bigint_uint64, (napi_env env, uint64_t v, napi_value* result),
  *result = env->value(jsi::BigInt::fromUint64(env->runtime, v))
)

API(
  napi_get_value_double, (napi_env env, napi_value v, double* result),
  if (!v->value.isNumber()) return napi_number_expected;
  *result = v->value.getNumber()
)
API(
  napi_get_value_bool, (napi_env env, napi_value v, bool* result),
  if (!v->value.isBool()) return napi_boolean_expected;
  *result = v->value.getBool()
)
API(
  napi_get_value_bigint_int64,
  (napi_env env, napi_value v, int64_t* result, bool* lossless),
  if (!v->value.isBigInt()) return napi_bigint_expected;
  auto bigint = v->value.getBigInt(env->runtime);
  *result = bigint.getInt64(env->runtime);
  *lossless = bigint.isInt64(env->runtime)
)
API(
  napi_get_value_bigint_uint64,
  (napi_env env, napi_value v, uint64_t* result, bool* lossless),
  if (!v->value.isBigInt()) return napi_bigint_expected;
  auto bigint = v->value.getBigInt(env->runtime);
  *result = bigint.getUint64(env->runtime);
  *lossless = bigint.isUint64(env->runtime)
)

API(
  napi_create_string_utf8,
  (napi_env env, const char* text, size_t length, napi_value* result),
  if (length == NAPI_AUTO_LENGTH) length = std::strlen(text);
  *result = env->value(
    jsi::String::createFromUtf8(
      env->runtime, reinterpret_cast<const uint8_t*>(text ? text : ""), length
    )
  )
)
API(
  napi_get_value_string_utf8,
  (napi_env env, napi_value v, char* buffer, size_t capacity, size_t* result),
  if (!v->value.isString()) return napi_string_expected;
  auto text = v->value.getString(env->runtime).utf8(env->runtime);
  size_t copied = text.size(); if (buffer) {
    copied = capacity ? std::min(copied, capacity - 1) : 0;
    // A truncated UTF-8 sequence is not a valid string.
    while (copied < text.size() && copied &&
           (static_cast<unsigned char>(text[copied]) & 0xc0) == 0x80)
      --copied;
    if (capacity) {
      std::memcpy(buffer, text.data(), copied);
      buffer[copied] = '\0';
    }
  } if (result) * result = copied
)

API(
  napi_typeof, (napi_env env, napi_value v, napi_valuetype* result),
  const auto& value = v->value;
  if (value.isUndefined()) * result = napi_undefined;
  else if (value.isNull()) * result = napi_null;
  else if (value.isBool()) * result = napi_boolean;
  else if (value.isNumber()) * result = napi_number;
  else if (value.isString()) * result = napi_string;
  else if (value.isSymbol()) * result = napi_symbol;
  else if (value.isBigInt()) * result = napi_bigint; else {
    auto object = value.getObject(env->runtime);
    if (object.isFunction(env->runtime))
      *result = napi_function;
    else if (object.hasNativeState<External>(env->runtime))
      *result = napi_external;
    else
      *result = napi_object;
  }
)

API(
  napi_get_named_property,
  (napi_env env, napi_value object, const char* name, napi_value* result),
  if (!object->value.isObject()) return napi_object_expected;
  *result = env->value(object->value.getObject(env->runtime)
                         .getProperty(env->runtime, name))
)
API(
  napi_set_named_property,
  (napi_env env, napi_value object, const char* name, napi_value v),
  if (!object->value.isObject()) return napi_object_expected;
  object->value.getObject(env->runtime)
    .setProperty(env->runtime, name, v->value)
)
API(
  napi_is_array, (napi_env env, napi_value v, bool* result),
  *result = v->value.isObject() &&
            v->value.getObject(env->runtime).isArray(env->runtime)
)
API(
  napi_get_array_length, (napi_env env, napi_value v, uint32_t* result),
  if (!v->value.isObject()) return napi_array_expected;
  auto object = v->value.getObject(env->runtime);
  if (!object.isArray(env->runtime)) return napi_array_expected;
  *result =
    static_cast<uint32_t>(object.getArray(env->runtime).size(env->runtime))
)
API(
  napi_get_element,
  (napi_env env, napi_value object, uint32_t index, napi_value* result),
  if (!object->value.isObject()) return napi_object_expected;
  *result =
    env->value(object->value.getObject(env->runtime)
                 .getProperty(
                   env->runtime,
                   jsi::PropNameID::forUtf8(env->runtime, std::to_string(index))
                 ))
)
API(
  napi_set_element,
  (napi_env env, napi_value object, uint32_t index, napi_value v),
  if (!object->value.isObject()) return napi_object_expected;
  object->value.getObject(env->runtime)
    .setProperty(
      env->runtime,
      jsi::PropNameID::forUtf8(env->runtime, std::to_string(index)), v->value
    )
)

API(
  napi_create_function,
  (napi_env env, const char* name, size_t length, napi_callback callback,
   void* data, napi_value* result),
  if (length == NAPI_AUTO_LENGTH) length = std::strlen(name);
  auto function = jsi::Function::createFromHostFunction(
    env->runtime,
    jsi::PropNameID::forUtf8(env->runtime, std::string(name, length)), 0,
    [state = env->shared_from_this(), callback, data](
      jsi::Runtime& runtime, const jsi::Value& receiver, const jsi::Value* args,
      size_t count
    ) {
      state->checkThread();
      if (state->closed)
        throw jsi::JSError(runtime, "the addon realm is closed");
      Scope scope(state.get());
      napi_callback_info__ info{{}, state->copy(receiver), data};
      for (size_t i = 0; i < count; ++i)
        info.args.push_back(state->copy(args[i]));
      auto result = callback(state.get(), &info);
      state->throwPending();
      return result ? jsi::Value(runtime, result->value)
                    : jsi::Value::undefined();
    }
  );
  *result = env->value(std::move(function))
)
API(
  napi_get_cb_info,
  (napi_env env, napi_callback_info info, size_t* count, napi_value* args,
   napi_value* receiver, void** data),
  if (count) {
    const size_t copied =
      args ? std::min(*count, info->args.size()) : info->args.size();
    if (args) std::copy_n(info->args.begin(), copied, args);
    *count = copied;
  } if (receiver) * receiver = info->receiver;
  if (data) * data = info->data
)
API(
  napi_call_function,
  (napi_env env, napi_value receiver, napi_value function, size_t count,
   const napi_value* args, napi_value* result),
  if (
    !function->value.isObject() ||
    !function->value.getObject(env->runtime).isFunction(env->runtime)
  ) return napi_function_expected;
  std::vector<jsi::Value> values;
  for (size_t i = 0; i < count; ++i)
    values.emplace_back(env->runtime, args[i]->value);
  auto fn = function->value.getObject(env->runtime).getFunction(env->runtime);
  auto value = receiver->value.isObject()
                 ? fn.callWithThis(
                     env->runtime, receiver->value.asObject(env->runtime),
                     static_cast<const jsi::Value*>(values.data()), count
                   )
                 : fn.call(
                     env->runtime,
                     static_cast<const jsi::Value*>(values.data()), count
                   );
  if (result) * result = env->value(std::move(value))
)

API(
  napi_create_reference,
  (napi_env env, napi_value v, uint32_t count, napi_ref* result),
  if (count != 1) return napi_invalid_arg;
  auto reference =
    std::make_unique<napi_ref__>(jsi::Value(env->runtime, v->value));
  env->references.insert(reference.get()); *result = reference.release()
)
API(
  napi_get_reference_value, (napi_env env, napi_ref ref, napi_value* result),
  if (!env->references.count(ref)) return napi_invalid_arg;
  *result = env->copy(ref->value)
)
napi_status napi_delete_reference(napi_env env, napi_ref ref) {
  return protect(
    env,
    [&]() {
      if (ref && !env->references.erase(ref)) return napi_invalid_arg;
      delete ref;
      return napi_ok;
    },
    true
  );
}
API(
  napi_open_handle_scope, (napi_env env, napi_handle_scope* result),
  auto scope = new napi_handle_scope__(env->arena);
  env->arena = scope; *result = scope
)
napi_status napi_close_handle_scope(napi_env env, napi_handle_scope scope) {
  return protect(
    env,
    [&]() {
      if (env->arena != scope) return napi_handle_scope_mismatch;
      env->arena = scope->parent;
      delete scope;
      return napi_ok;
    },
    true
  );
}

API(
  napi_create_external,
  (napi_env env, void* data, napi_finalize callback, void* hint,
   napi_value* result),
  auto object = jsi::Object(env->runtime);
  object.setNativeState(
    env->runtime, std::make_shared<External>(
                    env->shared_from_this(), Finalizer{callback, data, hint}
                  )
  );
  *result = env->value(std::move(object))
)
API(
  napi_get_value_external, (napi_env env, napi_value v, void** result),
  if (!v->value.isObject()) return napi_invalid_arg;
  auto object = v->value.getObject(env->runtime);
  if (!object.hasNativeState<External>(env->runtime)) return napi_invalid_arg;
  auto external = object.getNativeState<External>(env->runtime);
  if (external->env.get() != env) return napi_invalid_arg;
  *result = external->finalizer.data
)

API(
  napi_add_env_cleanup_hook, (napi_env env, void (*hook)(void*), void* data),
  env->hooks.emplace_back(hook, data)
)
API(
  napi_set_instance_data,
  (napi_env env, void* data, napi_finalize callback, void* hint),
  if (env->instance.data) return napi_invalid_arg;
  env->instance = {callback, data, hint}
)
API(
  napi_get_instance_data, (napi_env env, void** data),
  *data = env->instance.data
)

napi_status napi_is_exception_pending(napi_env env, bool* result) {
  return protect(
    env,
    [&]() {
      *result = bool(env->pending);
      return napi_ok;
    },
    true
  );
}
napi_status napi_get_and_clear_last_exception(
  napi_env env, napi_value* result
) {
  return protect(
    env,
    [&]() {
      *result = env->pending ? env->copy(*env->pending)
                             : env->value(jsi::Value::undefined());
      env->pending.reset();
      return napi_ok;
    },
    true
  );
}
napi_status napi_throw(napi_env env, napi_value error) {
  return protect(env, [&]() {
    env->pending = std::make_unique<jsi::Value>(env->runtime, error->value);
    return napi_ok;
  });
}
static napi_status throwError(
  napi_env env, size_t type, const char* code, const char* message
) {
  return protect(env, [&]() {
    auto error =
      env->errors[type]
        ->callAsConstructor(
          env->runtime, jsi::String::createFromUtf8(env->runtime, message)
        )
        .asObject(env->runtime);
    if (code)
      error.setProperty(
        env->runtime, "code", jsi::String::createFromUtf8(env->runtime, code)
      );
    env->pending = std::make_unique<jsi::Value>(std::move(error));
    return napi_ok;
  });
}
napi_status napi_throw_error(
  napi_env env, const char* code, const char* message
) {
  return throwError(env, 0, code, message);
}
napi_status napi_throw_type_error(
  napi_env env, const char* code, const char* message
) {
  return throwError(env, 1, code, message);
}
napi_status napi_throw_range_error(
  napi_env env, const char* code, const char* message
) {
  return throwError(env, 2, code, message);
}
napi_status napi_fatal_exception(napi_env env, napi_value error) {
  return napi_throw(env, error);
}

API(
  napi_create_threadsafe_function,
  (napi_env env, napi_value function, napi_value, napi_value, size_t maxQueue,
   size_t count, void* finalizeData, napi_finalize finalizeCallback,
   void* context, napi_threadsafe_function_call_js callback,
   napi_threadsafe_function* result),
  // The shared runtime and graphics probe each create one producer.
  if (count != 1 || !callback) return napi_invalid_arg;
  auto tsfn = std::make_shared<napi_threadsafe_function__>();
  tsfn->env = env->shared_from_this();
  if (function) tsfn->listener =
    std::make_unique<jsi::Value>(env->runtime, function->value);
  tsfn->callback = callback; tsfn->context = context; tsfn->maxQueue = maxQueue;
  tsfn->finalizer = {finalizeCallback, finalizeData, context};
  env->functions.push_back(tsfn); *result = tsfn.get()
)

napi_status napi_call_threadsafe_function(
  napi_threadsafe_function function, void* data,
  napi_threadsafe_function_call_mode mode
) {
  try {
    std::lock_guard lock(function->mutex);
    if (function->closed || function->releasing) return napi_closing;
    if (mode != napi_tsfn_nonblocking) return napi_invalid_arg;
    if (function->maxQueue && function->queue.size() >= function->maxQueue)
      return napi_queue_full;
    function->queue.push_back(data);
    if (!function->pumpPending) {
      try {
        function->schedule();
      } catch (...) {
        function->pumpPending = false;
        function->queue.pop_back();
        throw;
      }
    }
    return napi_ok;
  } catch (...) {
    return napi_generic_failure;
  }
}
napi_status napi_release_threadsafe_function(
  napi_threadsafe_function function, napi_threadsafe_function_release_mode mode
) {
  try {
    if (mode == napi_tsfn_abort) {
      function->env->checkThread();
      function->abort();
    } else if (mode == napi_tsfn_release) {
      std::lock_guard lock(function->mutex);
      if (function->closed || function->releasing) return napi_closing;
      function->releasing = true;
      if (!function->pumpPending) function->schedule();
    } else
      return napi_invalid_arg;
    return napi_ok;
  } catch (...) {
    return napi_generic_failure;
  }
}
API(napi_ref_threadsafe_function, (napi_env env, napi_threadsafe_function), )
API(napi_unref_threadsafe_function, (napi_env env, napi_threadsafe_function), )

struct Buffer final : jsi::MutableBuffer {
  std::vector<uint8_t> bytes;
  explicit Buffer(size_t size) : bytes(size) {}
  size_t size() const override { return bytes.size(); }
  uint8_t* data() override { return bytes.data(); }
};
API(
  napi_create_arraybuffer,
  (napi_env env, size_t length, void** data, napi_value* result),
  auto bytes = std::make_shared<Buffer>(length);
  if (data) * data = bytes->data();
  *result = env->value(jsi::ArrayBuffer(env->runtime, bytes))
)

static constexpr size_t widths[] = {1, 1, 1, 2, 2, 4, 4, 4, 8, 8, 8};
API(
  napi_create_typedarray,
  (napi_env env, napi_typedarray_type type, size_t length, napi_value buffer,
   size_t offset, napi_value* result),
  auto index = static_cast<size_t>(type);
  if (index >= env->viewConstructors.size()) return napi_invalid_arg;
  *result = env->value(env->viewConstructors[index]->callAsConstructor(
    env->runtime, buffer->value, static_cast<double>(offset),
    static_cast<double>(length)
  ))
)

static bool typedArray(napi_env env, napi_value v) {
  if (!v->value.isObject()) return false;
  // The captured tag getter returns undefined for a forged receiver. It does
  // not invoke a user getter or the receiver's own Symbol.toStringTag.
  return env->viewGetters[4]
    ->callWithThis(env->runtime, v->value.getObject(env->runtime))
    .isString();
}
API(
  napi_is_typedarray, (napi_env env, napi_value v, bool* result),
  *result = typedArray(env, v)
)
API(
  napi_get_typedarray_info,
  (napi_env env, napi_value v, napi_typedarray_type* type, size_t* length,
   void** data, napi_value* buffer, size_t* offset),
  if (!typedArray(env, v)) return napi_invalid_arg;
  auto object = v->value.getObject(env->runtime);
  auto get =
    [&](size_t index) {
      return env->viewGetters[index]->callWithThis(env->runtime, object);
    };
  auto backing = get(0).asObject(env->runtime).getArrayBuffer(env->runtime);
  auto byteOffset = static_cast<size_t>(get(1).asNumber());
  auto byteLength = static_cast<size_t>(get(2).asNumber());
  auto count = static_cast<size_t>(get(3).asNumber());
  auto tag = get(4).asString(env->runtime).utf8(env->runtime);
  const char* names[] =
    {"Int8Array", "Uint8Array", "Uint8ClampedArray", "Int16Array",
     "Uint16Array", "Int32Array", "Uint32Array", "Float32Array", "Float64Array",
     "BigInt64Array", "BigUint64Array"};
  size_t index = 0;
  while (index < std::size(names) && tag != names[index])++ index;
  if (index == std::size(names)) return napi_invalid_arg;
  auto capacity = backing.size(env->runtime); if (
    byteOffset > capacity || byteLength > capacity - byteOffset ||
    count > byteLength / widths[index] || count * widths[index] != byteLength
  ) return napi_invalid_arg;
  if (type) * type = static_cast<napi_typedarray_type>(index);
  if (length) * length = count; if (offset) * offset = byteOffset; if (data) {
    auto start = backing.data(env->runtime);
    *data = start ? start + byteOffset : nullptr;
  } if (buffer) * buffer = env->value(std::move(backing))
)

#undef API
