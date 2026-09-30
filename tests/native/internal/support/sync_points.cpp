#include <array>
#include <atomic>
#include <cstddef>

#include "sync_points.hpp"

#include "support/wait.h"

namespace mln::native_tests {
namespace {

constexpr auto point_count =
  static_cast<std::size_t>(SyncPoint::EmscriptenRunLoopStopSubmitted) + 1;

// Process lifetime, because a thread can still be inside the handler, or
// parked in it, after the scope that installed it ends.
struct State {
  std::array<std::atomic_int, point_count> hits{};
  std::array<std::atomic_bool, point_count> held{};
};

auto state() -> State& {
  static auto* instance = new State{};
  return *instance;
}

auto index(SyncPoint point) -> std::size_t {
  return static_cast<std::size_t>(point);
}

struct HitTarget {
  SyncPoint point;
  int count;
};

auto hits_reached(void* context) -> bool {
  const auto* target = static_cast<const HitTarget*>(context);
  return state().hits.at(index(target->point)).load() >= target->count;
}

auto released(void* context) -> bool {
  return !static_cast<const std::atomic_bool*>(context)->load();
}

void handle(SyncPoint point, void* context) noexcept {
  static_cast<void>(context);
  auto& held = state().held.at(index(point));
  state().hits.at(index(point)).fetch_add(1);
  mln_test_pulse();
  if (held.load()) {
    static_cast<void>(mln_test_await(
      released, &held, mln_test_deadline_default(),
      "the case to release a sync point"
    ));
  }
}

}  // namespace

SyncPointScope::SyncPointScope() noexcept {
  for (auto& count : state().hits) count.store(0);
  for (auto& held : state().held) held.store(false);
  mln::testing::set_sync_point_handler(handle, nullptr);
}

SyncPointScope::~SyncPointScope() {
  for (auto& held : state().held) held.store(false);
  mln_test_pulse();
  mln::testing::set_sync_point_handler(nullptr, nullptr);
}

auto SyncPointScope::hits(SyncPoint point) const noexcept -> int {
  return state().hits.at(index(point)).load();
}

auto SyncPointScope::wait_for_hits(SyncPoint point, int count) const noexcept
  -> bool {
  auto target = HitTarget{point, count};
  return mln_test_await(
    hits_reached, &target, mln_test_deadline_default(), "a sync point"
  );
}

void SyncPointScope::hold(SyncPoint point) noexcept {
  state().held.at(index(point)).store(true);
}

void SyncPointScope::release(SyncPoint point) noexcept {
  state().held.at(index(point)).store(false);
  mln_test_pulse();
}

}  // namespace mln::native_tests
