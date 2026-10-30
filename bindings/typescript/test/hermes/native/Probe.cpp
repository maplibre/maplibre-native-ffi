#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include <jsi/instrumentation.h>

#include "Probe.h"

#include "log.h"
#include "shim.h"

extern "C" napi_value mln_hermes_test_init(napi_env, napi_value);
extern "C" napi_value mln_hermes_test_graphics_init(napi_env, napi_value);

namespace mln::test {
namespace jsi = facebook::jsi;

static std::atomic<unsigned> generation;
static std::atomic<unsigned> releasedRealms;

static void event(unsigned id, const char* name) {
  char message[128];
  std::snprintf(message, sizeof message, "MlnLifecycle %u %s", id, name);
  testLog(message);
}

static void require(bool condition) {
  if (!condition) std::abort();
}
static void check(napi_status status) { require(status == napi_ok); }

struct Probe::State : std::enable_shared_from_this<State> {
  unsigned id = ++generation;
  std::thread::id jsThread = std::this_thread::get_id();
  napi_threadsafe_function function = nullptr;
  bool hook = false;
  bool functionFinalized = false;
  std::atomic<bool> instanceFinalized = false;
  unsigned drained = 0;
  std::atomic<unsigned> finalized = 0;
  static thread_local State* installing;

  ~State() {
    require(
      hook && instanceFinalized && functionFinalized && drained == 2 &&
      finalized == 2
    );
    event(id, "realm-released");
    ++releasedRealms;
  }

  static napi_value install(napi_env env, napi_value exports) {
    auto state = installing;
    auto instance = new std::shared_ptr<State>(state->shared_from_this());
    check(napi_set_instance_data(
      env, instance,
      [](napi_env, void* data, void*) {
        auto holder = static_cast<std::shared_ptr<State>*>(data);
        auto state = *holder;
        require(state->jsThread == std::this_thread::get_id());
        require(state->hook && state->functionFinalized && state->drained == 2);
        state->instanceFinalized = true;
        delete holder;
      },
      nullptr
    ));
    check(napi_create_threadsafe_function(
      env, nullptr, nullptr, nullptr, 0, 1, state,
      [](napi_env, void* data, void*) {
        auto state = static_cast<State*>(data);
        require(state->drained == 2 && !state->instanceFinalized);
        state->functionFinalized = true;
      },
      state,
      [](napi_env env, napi_value, void* context, void* data) {
        auto state = static_cast<State*>(context);
        require(env == nullptr && data != nullptr);
        require(state->jsThread == std::this_thread::get_id());
        require(state->hook && !state->instanceFinalized);
        ++state->drained;
      },
      &state->function
    ));
    check(napi_add_env_cleanup_hook(
      env,
      [](void* data) {
        auto state = static_cast<State*>(data);
        require(state->jsThread == std::this_thread::get_id());
        state->hook = true;
        // A real host pump posted here may be silently dropped. Cleanup
        // must drain both messages synchronously even when it never runs.
        check(napi_call_threadsafe_function(
          state->function, state, napi_tsfn_nonblocking
        ));
        check(napi_call_threadsafe_function(
          state->function, state, napi_tsfn_nonblocking
        ));
      },
      state
    ));
    for (const char* name : {"early", "late"}) {
      napi_value token;
      check(napi_create_external(
        env, new std::shared_ptr<State>(state->shared_from_this()),
        [](napi_env env, void* data, void*) {
          auto holder = static_cast<std::shared_ptr<State>*>(data);
          auto state = *holder;
          if (env) {
            require(state->jsThread == std::this_thread::get_id());
            require(!state->instanceFinalized);
          } else {
            require(state->instanceFinalized);
          }
          ++state->finalized;
          delete holder;
        },
        nullptr, &token
      ));
      check(napi_set_named_property(env, exports, name, token));
    }
    napi_value graphics;
    check(napi_create_object(env, &graphics));
    check(napi_set_named_property(
      env, exports, "graphics", mln_hermes_test_graphics_init(env, graphics)
    ));
    return mln_hermes_test_init(env, exports);
  }
};
thread_local Probe::State* Probe::State::installing;

Probe::Probe(jsi::Runtime& runtime, mln::hermes::Schedule schedule)
    : state_(std::make_shared<State>()),
      env_(
        std::make_unique<hermes::Environment>(runtime, std::move(schedule))
      ) {
  State::installing = state_.get();
  auto exports = env_->install(State::install);
  State::installing = nullptr;
  auto early = exports.getPropertyAsObject(runtime, "early");
  auto external = early.getNativeState(runtime);
  early.setNativeState(runtime, std::make_shared<jsi::NativeState>());
  runtime.global().setProperty(
    runtime, "__mlnReleaseEarly",
    jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forAscii(runtime, "releaseEarly"), 0,
      [external = std::move(external)](
        jsi::Runtime&, const jsi::Value&, const jsi::Value*, size_t
      ) mutable {
        // Wait until module installation has returned and host can pump.
        std::thread([external = std::move(external)]() mutable {
          external.reset();
        }).join();
        return jsi::Value::undefined();
      }
    )
  );
  runtime.global().setProperty(runtime, "__mlnShimTest", exports);
  runtime.global().setProperty(
    runtime, "__mlnTestGraphics", exports.getProperty(runtime, "graphics")
  );
  runtime.global().setProperty(
    runtime, "__mlnCollect",
    jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forAscii(runtime, "collect"), 0,
      [](jsi::Runtime& runtime, const jsi::Value&, const jsi::Value*, size_t) {
        runtime.instrumentation().collectGarbage("MapLibre binding suite");
        return jsi::Value::undefined();
      }
    )
  );
  runtime.global().setProperty(runtime, "__mlnForeignState", early);
  runtime.global().setProperty(
    runtime, "__mlnLifecycleGeneration", double(state_->id)
  );
}

void Probe::cleanup() { env_->cleanup(); }
Probe::~Probe() { cleanup(); }
unsigned Probe::released() { return releasedRealms; }

}  // namespace mln::test
