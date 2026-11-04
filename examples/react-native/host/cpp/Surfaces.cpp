#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Surfaces.h"

namespace jsi = facebook::jsi;

namespace mln::example::surfaces {

namespace {

struct Entry {
  std::shared_ptr<Surface> surface;
  bool acquired = false;
  bool withdrawn = false;
};

std::mutex mutex;
std::unordered_map<int32_t, Entry> entries;
int32_t lastId = 0;

// Takes an entry that both sides are done with, so the caller frees its surface
// outside the lock.
std::shared_ptr<Surface> eraseIfDone(int32_t id) {
  auto it = entries.find(id);
  if (it == entries.end() || it->second.acquired || !it->second.withdrawn) {
    return nullptr;
  }
  auto surface = std::move(it->second.surface);
  entries.erase(it);
  return surface;
}

}  // namespace

int32_t add(std::shared_ptr<Surface> surface) {
  std::lock_guard lock(mutex);
  auto id = ++lastId;
  entries.emplace(id, Entry{.surface = std::move(surface)});
  return id;
}

void withdraw(int32_t id) {
  std::shared_ptr<Surface> done;
  {
    std::lock_guard lock(mutex);
    auto it = entries.find(id);
    if (it == entries.end()) return;
    it->second.withdrawn = true;
    done = eraseIfDone(id);
  }
}

void install(jsi::Runtime& runtime) {
  auto object = jsi::Object(runtime);
  object.setProperty(
    runtime, "acquire",
    jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forAscii(runtime, "acquire"), 1,
      [](
        jsi::Runtime& rt, const jsi::Value&, const jsi::Value* args,
        size_t count
      ) -> jsi::Value {
        if (count < 1 || !args[0].isNumber()) {
          throw jsi::JSError(rt, "acquire takes a surface ID");
        }
        auto id = static_cast<int32_t>(args[0].getNumber());
        std::shared_ptr<Surface> surface;
        {
          std::lock_guard lock(mutex);
          auto it = entries.find(id);
          // The view may have withdrawn the surface before JavaScript heard
          // about it.
          if (it == entries.end() || it->second.withdrawn) {
            return jsi::Value::undefined();
          }
          it->second.acquired = true;
          surface = it->second.surface;
        }
        return surface->describe(rt);
      }
    )
  );
  object.setProperty(
    runtime, "release",
    jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forAscii(runtime, "release"), 1,
      [](
        jsi::Runtime& rt, const jsi::Value&, const jsi::Value* args,
        size_t count
      ) -> jsi::Value {
        if (count < 1 || !args[0].isNumber()) {
          throw jsi::JSError(rt, "release takes a surface ID");
        }
        auto id = static_cast<int32_t>(args[0].getNumber());
        std::shared_ptr<Surface> done;
        {
          std::lock_guard lock(mutex);
          auto it = entries.find(id);
          if (it == entries.end()) return jsi::Value::undefined();
          it->second.acquired = false;
          done = eraseIfDone(id);
        }
        return jsi::Value::undefined();
      }
    )
  );
  runtime.global().setProperty(runtime, "__mlnExampleSurfaces", object);
}

void releaseAll() {
  std::vector<std::shared_ptr<Surface>> done;
  std::lock_guard lock(mutex);
  for (auto& [id, entry] : entries) entry.acquired = false;
  for (auto it = entries.begin(); it != entries.end();) {
    if (it->second.withdrawn) {
      done.push_back(std::move(it->second.surface));
      it = entries.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace mln::example::surfaces
