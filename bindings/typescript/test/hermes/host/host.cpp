// The Hermes test host: runs one script on a Hermes runtime with an event loop
// that serves Node-API threadsafe functions and timers, as an application that
// embeds Hermes would.
//
//   maplibre_native_ffi_hermes_host <script.js>
//
// The script sees these globals beside the language's own:
//
//   setTimeout, clearTimeout, setImmediate, clearImmediate, console, gc
//   hermesHost.loadAddon(path)   loads a Node-API addon into its own env
//   hermesHost.unrefTimer(id)    keeps a timer from holding the loop open
//   hermesHost.env               the process environment
//   hermesHost.exit(code)        sets the exit code for when the loop ends
//
// The process exits with the code the script set once its event loop is empty.
// A script that never sets one exits with 13, as Node does for a top-level
// await that never settles, so a loop that empties early fails the run.

#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <unistd.h>

#include "hermes/Public/RuntimeConfig.h"
#include "hermes/VM/Runtime.h"
#include "llvh/Support/raw_ostream.h"
#include "napi/hermes_napi.h"
#include "napi/hermes_napi_compile.h"

extern char** environ;

using namespace hermes;
using Clock = std::chrono::steady_clock;

namespace {

#define CHECK(call)                                                            \
  do {                                                                         \
    if ((call) != napi_ok) {                                                   \
      std::fprintf(                                                            \
        stderr, "hermes host: %s failed at %s:%d\n", #call, __FILE__, __LINE__ \
      );                                                                       \
      std::abort();                                                            \
    }                                                                          \
  } while (0)

struct Timer {
  Clock::time_point deadline;
  napi_ref callback;
  bool referenced;
};

// The event loop. Native threads post tasks and hold the loop through the
// Node-API host; timers belong to the JavaScript thread.
class Loop {
 public:
  hermes_napi_host host{};

  Loop() {
    host.data = this;
    host.post_work = &postWork;
    host.cancel_work = [](void*, void*) { return false; };
    host.post_task = &postTask;
    host.fatal_exception = &fatalException;
    host.ref_loop = [](void* data) {
      auto* self = static_cast<Loop*>(data);
      std::lock_guard<std::mutex> lock(self->mutex_);
      ++self->refs_;
    };
    host.unref_loop = [](void* data) {
      auto* self = static_cast<Loop*>(data);
      std::lock_guard<std::mutex> lock(self->mutex_);
      --self->refs_;
      self->wake_.notify_all();
    };
  }

  uint32_t addTimer(napi_env env, napi_value callback, double milliseconds) {
    napi_ref ref;
    CHECK(napi_create_reference(env, callback, 1, &ref));
    if (!(milliseconds > 0)) {
      milliseconds = 0;
    }
    auto deadline =
      Clock::now() +
      std::chrono::microseconds(static_cast<int64_t>(milliseconds * 1000));
    uint32_t id = ++lastTimer_;
    timers_[id] = Timer{deadline, ref, true};
    order_.insert({deadline, id});
    return id;
  }

  void clearTimer(napi_env env, uint32_t id) {
    auto found = timers_.find(id);
    if (found == timers_.end()) {
      return;
    }
    CHECK(napi_delete_reference(env, found->second.callback));
    order_.erase({found->second.deadline, id});
    timers_.erase(found);
  }

  void unrefTimer(uint32_t id) {
    auto found = timers_.find(id);
    if (found != timers_.end()) {
      found->second.referenced = false;
    }
  }

  // Runs due timers and posted tasks until nothing holds the loop.
  void run(napi_env env, vm::Runtime& runtime) {
    for (;;) {
      // Timers that are due when the pass starts run in deadline order. A
      // timer that a callback adds waits for the next pass.
      auto now = Clock::now();
      uint32_t last = lastTimer_;
      std::vector<uint32_t> due;
      for (auto& [deadline, id] : order_) {
        if (deadline > now) {
          break;
        }
        if (id <= last) {
          due.push_back(id);
        }
      }
      for (uint32_t id : due) {
        auto found = timers_.find(id);
        if (found == timers_.end()) {
          continue;
        }
        napi_ref ref = found->second.callback;
        order_.erase({found->second.deadline, id});
        timers_.erase(found);
        napi_handle_scope scope;
        CHECK(napi_open_handle_scope(env, &scope));
        napi_value callback, global, ignored;
        CHECK(napi_get_reference_value(env, ref, &callback));
        CHECK(napi_delete_reference(env, ref));
        CHECK(napi_get_global(env, &global));
        if (
          napi_call_function(env, global, callback, 0, nullptr, &ignored) !=
          napi_ok
        ) {
          reportUncaught(env);
        }
        CHECK(napi_close_handle_scope(env, scope));
        checkpoint(runtime);
      }

      std::deque<std::function<void()>> tasks;
      {
        std::lock_guard<std::mutex> lock(mutex_);
        tasks.swap(tasks_);
      }
      for (auto& task : tasks) {
        task();
        checkpoint(runtime);
      }
      if (!tasks.empty() || !due.empty()) {
        continue;
      }

      std::unique_lock<std::mutex> lock(mutex_);
      if (!tasks_.empty()) {
        continue;
      }
      bool referencedTimer = false;
      for (auto& [id, timer] : timers_) {
        referencedTimer = referencedTimer || timer.referenced;
      }
      if (refs_ == 0 && !referencedTimer) {
        return;
      }
      if (order_.empty()) {
        wake_.wait(lock);
      } else {
        wake_.wait_until(lock, order_.begin()->first);
      }
    }
  }

  // A microtask checkpoint, which the embedder performs when the stack
  // empties: Hermes releases the targets that WeakRef reads kept and runs
  // FinalizationRegistry callbacks only when asked, before the job queue.
  static void checkpoint(vm::Runtime& runtime) {
    runtime.clearKeptObjects();
    if (
      runtime.cleanUpFinalizationCallbacks() ==
        vm::ExecutionStatus::EXCEPTION ||
      runtime.drainJobs() == vm::ExecutionStatus::EXCEPTION
    ) {
      vm::GCScope scope(runtime);
      auto thrown = runtime.makeHandle(runtime.getThrownValue());
      runtime.clearThrownValue();
      runtime.printException(llvh::errs(), thrown);
      llvh::errs().flush();
      std::_Exit(1);
    }
  }

  // An exception that nothing caught ends the process, as it does in Node.
  static void reportUncaught(napi_env env) {
    napi_value error;
    CHECK(napi_get_and_clear_last_exception(env, &error));
    printError(env, error);
    std::_Exit(1);
  }

  static void printError(napi_env env, napi_value error) {
    napi_value text = error;
    napi_valuetype type;
    CHECK(napi_typeof(env, error, &type));
    if (type == napi_object) {
      napi_value stack;
      napi_valuetype stackType;
      if (
        napi_get_named_property(env, error, "stack", &stack) == napi_ok &&
        napi_typeof(env, stack, &stackType) == napi_ok &&
        stackType == napi_string
      ) {
        text = stack;
      }
    }
    std::fprintf(stderr, "Uncaught %s\n", toString(env, text).c_str());
  }

  static std::string toString(napi_env env, napi_value value) {
    napi_value string;
    if (napi_coerce_to_string(env, value, &string) != napi_ok) {
      napi_value ignored;
      napi_get_and_clear_last_exception(env, &ignored);
      return "<a value that cannot convert to a string>";
    }
    size_t length = 0;
    CHECK(napi_get_value_string_utf8(env, string, nullptr, 0, &length));
    std::string text(length, '\0');
    CHECK(
      napi_get_value_string_utf8(env, string, text.data(), length + 1, &length)
    );
    return text;
  }

 private:
  std::mutex mutex_;
  std::condition_variable wake_;
  std::deque<std::function<void()>> tasks_;
  // Loop references from referenced threadsafe functions and in-flight work.
  int refs_ = 0;
  uint32_t lastTimer_ = 0;
  std::map<uint32_t, Timer> timers_;
  std::set<std::pair<Clock::time_point, uint32_t>> order_;

  void post(std::function<void()> task) {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push_back(std::move(task));
    wake_.notify_all();
  }

  static void postTask(void* data, void* taskData, void (*callback)(void*)) {
    static_cast<Loop*>(data)->post([=] { callback(taskData); });
  }

  static void postWork(
    void* data, void* workData, void (*execute)(void*),
    void (*complete)(void*, napi_status)
  ) {
    auto* self = static_cast<Loop*>(data);
    {
      std::lock_guard<std::mutex> lock(self->mutex_);
      ++self->refs_;
    }
    std::thread([=] {
      execute(workData);
      self->post([=] {
        complete(workData, napi_ok);
        std::lock_guard<std::mutex> lock(self->mutex_);
        --self->refs_;
      });
    }).detach();
  }

  static void fatalException(void*, napi_env env, napi_value error) {
    printError(env, error);
    std::_Exit(1);
  }
};

struct Host {
  vm::Runtime* runtime;
  Loop* loop;
  int exitCode = 13;
};

Host* host(
  napi_env env, napi_callback_info info, size_t* argc, napi_value* argv
) {
  void* data;
  CHECK(napi_get_cb_info(env, info, argc, argv, nullptr, &data));
  return static_cast<Host*>(data);
}

napi_value undefined(napi_env env) {
  napi_value value;
  CHECK(napi_get_undefined(env, &value));
  return value;
}

napi_value number(napi_env env, double value) {
  napi_value result;
  CHECK(napi_create_double(env, value, &result));
  return result;
}

double argNumber(napi_env env, size_t argc, napi_value* argv, size_t index) {
  double value = 0;
  if (index < argc) {
    napi_valuetype type;
    CHECK(napi_typeof(env, argv[index], &type));
    if (type == napi_number) {
      CHECK(napi_get_value_double(env, argv[index], &value));
    }
  }
  return value;
}

napi_value schedule(napi_env env, napi_callback_info info, bool immediate) {
  size_t argc = 2;
  napi_value argv[2];
  Host* self = host(env, info, &argc, argv);
  napi_valuetype type = napi_undefined;
  if (argc > 0) {
    CHECK(napi_typeof(env, argv[0], &type));
  }
  if (type != napi_function) {
    napi_throw_type_error(env, nullptr, "the callback is not a function");
    return nullptr;
  }
  double milliseconds = immediate ? 0 : argNumber(env, argc, argv, 1);
  return number(env, self->loop->addTimer(env, argv[0], milliseconds));
}

napi_value setTimeout(napi_env env, napi_callback_info info) {
  return schedule(env, info, false);
}

napi_value setImmediate(napi_env env, napi_callback_info info) {
  return schedule(env, info, true);
}

napi_value clearTimer(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  Host* self = host(env, info, &argc, argv);
  self->loop->clearTimer(
    env, static_cast<uint32_t>(argNumber(env, argc, argv, 0))
  );
  return undefined(env);
}

napi_value unrefTimer(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  Host* self = host(env, info, &argc, argv);
  self->loop->unrefTimer(static_cast<uint32_t>(argNumber(env, argc, argv, 0)));
  return undefined(env);
}

napi_value collectGarbage(napi_env env, napi_callback_info info) {
  size_t argc = 0;
  Host* self = host(env, info, &argc, nullptr);
  self->runtime->collect("forced");
  return undefined(env);
}

napi_value setExitCode(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  Host* self = host(env, info, &argc, argv);
  self->exitCode = static_cast<int>(argNumber(env, argc, argv, 0));
  return undefined(env);
}

napi_value write(napi_env env, napi_callback_info info, FILE* stream) {
  size_t argc = 16;
  napi_value argv[16];
  host(env, info, &argc, argv);
  std::string line;
  for (size_t i = 0; i < argc && i < 16; ++i) {
    if (i > 0) {
      line += ' ';
    }
    line += Loop::toString(env, argv[i]);
  }
  std::fprintf(stream, "%s\n", line.c_str());
  std::fflush(stream);
  return undefined(env);
}

napi_value log(napi_env env, napi_callback_info info) {
  return write(env, info, stdout);
}

napi_value warn(napi_env env, napi_callback_info info) {
  return write(env, info, stderr);
}

// Loads an addon into an env of its own, as Node gives each addon its own
// instance data and cleanup hooks.
napi_value loadAddon(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1];
  Host* self = host(env, info, &argc, argv);
  if (argc < 1) {
    napi_throw_type_error(env, nullptr, "loadAddon takes a path");
    return nullptr;
  }
  std::string path = Loop::toString(env, argv[0]);
  napi_env addonEnv = hermes_napi_create_env(self->runtime, &self->loop->host);
  napi_handle_scope scope;
  CHECK(napi_open_handle_scope(addonEnv, &scope));
  napi_value exports;
  napi_status status =
    hermes_napi_load_module(addonEnv, path.c_str(), &exports);
  napi_value result = nullptr;
  if (status == napi_ok) {
    // A value is a slot in the env's scope, so it moves through an array in
    // this env before the addon env's scope closes.
    napi_value holder;
    CHECK(napi_create_array_with_length(env, 1, &holder));
    CHECK(napi_set_element(env, holder, 0, exports));
    CHECK(napi_close_handle_scope(addonEnv, scope));
    CHECK(napi_get_element(env, holder, 0, &result));
    return result;
  }
  napi_value error = nullptr;
  bool pending = false;
  napi_is_exception_pending(addonEnv, &pending);
  if (pending) {
    CHECK(napi_get_and_clear_last_exception(addonEnv, &error));
    napi_value holder;
    CHECK(napi_create_array_with_length(env, 1, &holder));
    CHECK(napi_set_element(env, holder, 0, error));
    CHECK(napi_close_handle_scope(addonEnv, scope));
    CHECK(napi_get_element(env, holder, 0, &error));
    napi_throw(env, error);
    return nullptr;
  }
  CHECK(napi_close_handle_scope(addonEnv, scope));
  std::string message = "could not load " + path;
  napi_throw_error(env, nullptr, message.c_str());
  return nullptr;
}

void define(
  napi_env env, napi_value object, const char* name, napi_callback callback,
  Host* self
) {
  napi_value function;
  CHECK(
    napi_create_function(env, name, NAPI_AUTO_LENGTH, callback, self, &function)
  );
  CHECK(napi_set_named_property(env, object, name, function));
}

void installGlobals(napi_env env, Host* self) {
  napi_value global;
  CHECK(napi_get_global(env, &global));
  define(env, global, "setTimeout", setTimeout, self);
  define(env, global, "clearTimeout", clearTimer, self);
  define(env, global, "setImmediate", setImmediate, self);
  define(env, global, "clearImmediate", clearTimer, self);
  define(env, global, "gc", collectGarbage, self);

  napi_value console;
  CHECK(napi_create_object(env, &console));
  define(env, console, "log", log, self);
  define(env, console, "info", log, self);
  define(env, console, "debug", log, self);
  define(env, console, "warn", warn, self);
  define(env, console, "error", warn, self);
  CHECK(napi_set_named_property(env, global, "console", console));

  napi_value object;
  CHECK(napi_create_object(env, &object));
  define(env, object, "loadAddon", loadAddon, self);
  define(env, object, "unrefTimer", unrefTimer, self);
  define(env, object, "exit", setExitCode, self);
  napi_value variables;
  CHECK(napi_create_object(env, &variables));
  for (char** entry = environ; *entry != nullptr; ++entry) {
    std::string pair = *entry;
    size_t equals = pair.find('=');
    if (equals == std::string::npos) {
      continue;
    }
    napi_value value;
    CHECK(napi_create_string_utf8(
      env, pair.c_str() + equals + 1, NAPI_AUTO_LENGTH, &value
    ));
    CHECK(napi_set_named_property(
      env, variables, pair.substr(0, equals).c_str(), value
    ));
  }
  CHECK(napi_set_named_property(env, object, "env", variables));
  CHECK(napi_set_named_property(env, global, "hermesHost", object));
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::fprintf(stderr, "usage: %s <script.js>\n", argv[0]);
    return 2;
  }
  std::ifstream file(argv[1], std::ios::binary);
  if (!file) {
    std::fprintf(stderr, "hermes host: cannot read %s\n", argv[1]);
    return 2;
  }
  std::stringstream source;
  source << file.rdbuf();
  std::string script = source.str();

  // The loop outlives the runtime, whose teardown runs the envs' cleanup
  // hooks, which may still reach it.
  Loop loop;
  Host self{nullptr, &loop};
  int exitCode;
  {
    auto runtime = vm::Runtime::create(
      vm::RuntimeConfig::Builder()
        .withES6BlockScoping(true)
        .withEnableAsyncGenerators(true)
        .build()
    );
    self.runtime = runtime.get();
    napi_env env = hermes_napi_create_env(runtime.get(), &loop.host);
    installGlobals(env, &self);

    napi_handle_scope scope;
    CHECK(napi_open_handle_scope(env, &scope));
    // An application runs optimized bytecode. hermes_run_script compiles
    // debug bytecode, whose closures keep every variable of their scopes
    // alive, so an object that only a dead variable names is never collected.
    hermes_compile_flags compile{};
    compile.struct_size = sizeof(compile);
    compile.optimize = true;
    uint8_t* bytecode = nullptr;
    size_t bytecodeSize = 0;
    napi_value result;
    if (
      hermes_compile_to_bytecode(
        env, reinterpret_cast<const uint8_t*>(script.data()), script.size(),
        argv[1], &compile, &bytecode, &bytecodeSize
      ) != napi_ok ||
      hermes_run_bytecode(
        env, bytecode, bytecodeSize,
        [](const uint8_t* data, size_t, void*) {
          hermes_free_bytecode(const_cast<uint8_t*>(data));
        },
        nullptr, argv[1], nullptr, &result
      ) != napi_ok
    ) {
      Loop::reportUncaught(env);
    }
    CHECK(napi_close_handle_scope(env, scope));
    Loop::checkpoint(*runtime);

    loop.run(env, *runtime);
    exitCode = self.exitCode;
    if (exitCode == 13) {
      std::fprintf(
        stderr,
        "hermes host: the event loop emptied before the script "
        "set an exit code\n"
      );
    }
  }
  std::fflush(stdout);
  return exitCode;
}
