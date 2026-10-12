#pragma once

// The internal suite's view of the library's sync points; see
// src/testing/sync_point.hpp.

#include "testing/sync_point.hpp"

namespace mln::native_tests {

using mln::testing::SyncPoint;

// Installs the suite's sync point handler for the life of the scope, with
// every count at zero and nothing held. The handler counts each point's hits,
// pulses so a waiter re-checks, and parks a thread that reaches a held point
// until the case releases it or the default deadline passes.
//
// Some points run with a library lock held; see sync_point.hpp. Hold only the
// points that document no lock.
//
// The counts are process-wide, and a thread still finishing an earlier case's
// work would add to them. Single-process runners run every case in one
// process, so each case has to end only after its runtimes' releases have
// completed. A completed release has waited for its maps' cleanup and its
// in-flight resource callbacks, so no earlier case can reach a point later.
class SyncPointScope {
 public:
  SyncPointScope() noexcept;
  ~SyncPointScope();
  SyncPointScope(const SyncPointScope&) = delete;
  auto operator=(const SyncPointScope&) -> SyncPointScope& = delete;

  [[nodiscard]] auto hits(SyncPoint point) const noexcept -> int;
  // Waits within the default deadline until `point` has `count` hits.
  [[nodiscard]] auto wait_for_hits(SyncPoint point, int count) const noexcept
    -> bool;
  // Parks every later arrival at `point` until release().
  void hold(SyncPoint point) noexcept;
  void release(SyncPoint point) noexcept;
};

}  // namespace mln::native_tests
