#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <string>

#include <hermes/hermes.h>

#include "Probe.h"
#include "log.h"
#include "module.h"
#include "smoke.h"
#include "suite.h"

#ifdef __ANDROID__
#include <fbjni/fbjni.h>
#include <maplibre_native_c/android.h>
#endif

namespace jsi = facebook::jsi;
using Clock = std::chrono::steady_clock;
using Task = std::function<void()>;

struct Loop {
  std::mutex mutex;
  std::condition_variable ready;
  std::deque<Task> tasks;
  std::map<std::pair<Clock::time_point, unsigned>, Task> timers;
  unsigned nextTimer = 0;
  bool done = false;
  bool reload = false;
  std::string error;

  void post(Task task) {
    std::lock_guard lock(mutex);
    tasks.push_back(std::move(task));
    ready.notify_one();
  }

  Task next(Clock::time_point deadline) {
    std::unique_lock lock(mutex);
    for (;;) {
      if (!tasks.empty()) {
        auto task = std::move(tasks.front());
        tasks.pop_front();
        return task;
      }
      auto now = Clock::now();
      if (now >= deadline) throw std::runtime_error("Hermes suite timed out");
      if (!timers.empty() && timers.begin()->first.first <= now) {
        auto entry = timers.extract(timers.begin());
        return std::move(entry.mapped());
      }
      ready.wait_until(
        lock, timers.empty() ? deadline
                             : std::min(deadline, timers.begin()->first.first)
      );
    }
  }

  void clear() {
    std::deque<Task> discarded;
    {
      std::lock_guard lock(mutex);
      discarded.swap(tasks);
    }
    timers.clear();
  }
};

template <typename F>
static void function(
  jsi::Runtime& runtime, const char* name, unsigned argc, F body
) {
  runtime.global().setProperty(
    runtime, name,
    jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forAscii(runtime, name), argc, std::move(body)
    )
  );
}

static void installHost(jsi::Runtime& runtime, Loop& loop) {
  function(
    runtime, "__mlnPrint", 1,
    [](jsi::Runtime& rt, const jsi::Value&, const jsi::Value* args, size_t) {
      testLog(args[0].asString(rt).utf8(rt).c_str());
      return jsi::Value::undefined();
    }
  );
  function(
    runtime, "__mlnUtf8", 1,
    [](jsi::Runtime& rt, const jsi::Value&, const jsi::Value* args, size_t) {
      auto text = args[0].asString(rt).utf8(rt);
      jsi::Array bytes(rt, text.size());
      for (size_t i = 0; i < text.size(); ++i)
        bytes.setValueAtIndex(rt, i, int(static_cast<unsigned char>(text[i])));
      return jsi::Value(std::move(bytes));
    }
  );
  function(
    runtime, "__mlnTimeout", 2,
    [&loop](
      jsi::Runtime& rt, const jsi::Value&, const jsi::Value* args, size_t
    ) {
      auto id = ++loop.nextTimer;
      auto callback =
        std::make_shared<jsi::Function>(args[0].asObject(rt).asFunction(rt));
      auto due = Clock::now() +
                 std::chrono::milliseconds(
                   static_cast<long long>(std::max(0.0, args[1].asNumber()))
                 );
      loop.timers.emplace(std::make_pair(due, id), [&rt, callback] {
        callback->call(rt);
      });
      return jsi::Value(double(id));
    }
  );
  function(
    runtime, "__mlnCancelTimeout", 1,
    [&loop](jsi::Runtime&, const jsi::Value&, const jsi::Value* args, size_t) {
      auto id = static_cast<unsigned>(args[0].asNumber());
      std::erase_if(loop.timers, [id](const auto& entry) {
        return entry.first.second == id;
      });
      return jsi::Value::undefined();
    }
  );
  function(
    runtime, "__mlnReload", 0,
    [&loop](jsi::Runtime&, const jsi::Value&, const jsi::Value*, size_t) {
      loop.reload = true;
      loop.done = true;
      return jsi::Value::undefined();
    }
  );
  function(
    runtime, "__mlnDone", 1,
    [&loop](
      jsi::Runtime& rt, const jsi::Value&, const jsi::Value* args, size_t count
    ) {
      if (count && !args[0].isUndefined())
        loop.error = args[0].asString(rt).utf8(rt);
      loop.done = true;
      return jsi::Value::undefined();
    }
  );
}

static void runRealm(unsigned generation) {
  Loop loop;
  auto runtime = facebook::hermes::makeHermesRuntime(
    ::hermes::vm::RuntimeConfig::Builder().withMicrotaskQueue(true).build()
  );
  auto schedule = [&loop](Task task) { loop.post(std::move(task)); };
  std::unique_ptr<mln::hermes::Module> module;
  std::unique_ptr<mln::test::Probe> probe;
  std::string failure;
  try {
    module = std::make_unique<mln::hermes::Module>(*runtime, schedule);
    probe = std::make_unique<mln::test::Probe>(*runtime, schedule);
    installHost(*runtime, loop);
    runtime->evaluateJavaScript(
      std::make_shared<jsi::StringBuffer>(std::string(
        reinterpret_cast<const char*>(generation == 1 ? suite : smoke),
        generation == 1 ? sizeof(suite) : sizeof(smoke)
      )),
      "hermes-test.js"
    );
    auto deadline = Clock::now() + std::chrono::seconds(180);
    runtime->drainMicrotasks();
    while (!loop.done) {
      loop.next(deadline)();
      runtime->drainMicrotasks();
    }
    if (!loop.error.empty()) throw std::runtime_error(loop.error);
    if (loop.reload != (generation == 1))
      throw std::runtime_error("unexpected runtime reload");
  } catch (const std::exception& error) {
    // JSError owns a VM value; release the exception before destroying the VM.
    failure = error.what();
  }
  probe.reset();
  module.reset();
  // Drop queued pumps and timer roots before the VM, then discard late pumps
  // posted by the VM's finalizer thread after the environments have closed.
  loop.clear();
  runtime.reset();
  loop.clear();
  if (!failure.empty()) throw std::runtime_error(failure);
  if (mln::test::Probe::released() != generation)
    throw std::runtime_error("the old realm still owns native state");
}

static int runTests() {
  try {
    runRealm(1);
    runRealm(2);
    testLog("Hermes suite, readback and runtime reload passed");
    return 0;
  } catch (const std::exception& error) {
    testLog(error.what());
    return 1;
  }
}

#ifdef __ANDROID__
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void*) {
  return facebook::jni::initialize(vm, [] {});
}
extern "C" JNIEXPORT jint JNICALL
Java_org_maplibre_nativeffi_hermestest_MainActivity_run(
  JNIEnv* env, jclass, jobject context
) {
  mln_diagnostic diagnostic = {.size = sizeof diagnostic};
  if (mln_android_init(env, context, &diagnostic) != MLN_STATUS_OK) {
    testLog(diagnostic.message);
    return 1;
  }
  return runTests();
}
#else
int main() { return runTests(); }
#endif
