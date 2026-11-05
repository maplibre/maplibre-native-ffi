// Loading: how long a camera jump takes to show the new view complete.

#include <array>

#include "bench.hpp"

namespace bench {
namespace {

// From a jump between two views that share no tile until the first frame that
// shows the new view complete has finished on the GPU, with frames on the
// display's clock. Untimed, `idle` runs on to the map's idle event, and `cold`
// first drops the loaded tiles with mln_render_session_clear_data().
void jump(benchmark::State& state, bool cold) {
  Scene scene;
  scene.load_benchmark_scene();
  auto& session = scene.session();
  const std::array<mln_lat_lng, 2> views{south_east(), north_west()};
  // Both views load once, so a warm jump finds their tiles.
  for (const auto& view : views) {
    session.present_until_full(scene.jump(view));
    session.settle(scene.map());
  }
  Meter meter{state};
  auto& idle = meter.series("idle");
  int64_t frames = 0;
  int64_t refreshes = 0;
  size_t index = 0;
  for (auto _ : state) {
    if (cold) {
      state.PauseTiming();
      meter.pause();
      Completion cleared;
      BENCH_CHECK(mln_render_session_clear_data(
        session.handle(), cleared.descriptor(), diag()
      ));
      finish(cleared, "the clear", &session);
      meter.resume();
      state.ResumeTiming();
    }
    const auto idle_before = events().idle_count(scene.map());
    const auto start = Clock::now();
    const auto presented =
      session.present_until_full(scene.jump(views[index++ & 1U]));
    state.PauseTiming();
    meter.pause();
    frames += presented.frames;
    refreshes += presented.refreshes;
    session.follow_redraws_until(
      [&] { return events().idle_count(scene.map()) > idle_before; },
      "the idle event"
    );
    idle.add(events().last_idle(scene.map()) - start);
    meter.resume();
    state.ResumeTiming();
  }
  meter.finish();
  state.counters["frames_rendered"] = benchmark::Counter(
    static_cast<double>(frames), benchmark::Counter::kAvgIterations
  );
  state.counters["refreshes"] = benchmark::Counter(
    static_cast<double>(refreshes), benchmark::Counter::kAvgIterations
  );
  scene.save_frame(state);
}

void jump_cold(benchmark::State& state) { jump(state, true); }
void jump_warm(benchmark::State& state) { jump(state, false); }

}  // namespace

void register_load_cases() {
  add_case("Load/JumpCold", jump_cold, 10)->Unit(benchmark::kMillisecond);
  add_case("Load/JumpWarm", jump_warm, 10)->Unit(benchmark::kMillisecond);
}

}  // namespace bench
