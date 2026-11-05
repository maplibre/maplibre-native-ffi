// The frame loop: frames as fast as the host takes them, frames on a display's
// clock while the camera flies into tiles that haven't loaded, and a settled
// map while nothing happens.

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

#include "bench.hpp"

namespace bench {
namespace {

constexpr size_t kLap = 240;
constexpr double kRadiusPixels = 200.0;

// Camera jumps around a circle of kRadiusPixels around the scene's center, one
// lap in kLap frames, which stays inside the fixture's tiles.
auto circle() -> std::array<mln_camera_update, kLap> {
  const auto center = scene_center();
  // Pixels per degree at the scene's zoom, for 512-pixel tiles.
  const double world = 512.0 * std::exp2(kZoom);
  const double per_degree_x = world / 360.0;
  const double per_degree_y =
    per_degree_x / std::cos(center.latitude * std::numbers::pi / 180.0);
  std::array<mln_camera_update, kLap> jumps{};
  for (size_t index = 0; index < kLap; ++index) {
    const double angle = 2.0 * std::numbers::pi * static_cast<double>(index) /
                         static_cast<double>(kLap);
    jumps[index] = jump_to({
      center.latitude + kRadiusPixels * std::sin(angle) / per_degree_y,
      center.longitude + kRadiusPixels * std::cos(angle) / per_degree_x,
    });
  }
  return jumps;
}

void report_engine_frames(
  benchmark::State& state, const std::vector<FrameEvent>& frames
) {
  Series encoding;
  Series rendering;
  for (const auto& frame : frames) {
    encoding.add_ms(frame.encoding_ms);
    rendering.add_ms(frame.rendering_ms);
  }
  encoding.report_distribution(state, "engine_encoding");
  rendering.report_distribution(state, "engine_rendering");
}

// From a camera jump until the host has drained, acquired, waited for, and
// released the frame that shows it, around a circle whose tiles an untimed lap
// loaded.
void frame_throughput(benchmark::State& state) {
  Scene scene;
  scene.load_benchmark_scene();
  auto& session = scene.session();
  const auto jumps = circle();
  for (const auto& jump : jumps) {
    BENCH_CHECK(mln_map_update_camera(scene.map(), &jump, nullptr, diag()));
    session.render(MLN_FRAME_DEMAND_WAIT_FOR_UPDATE);
  }
  session.settle(scene.map());

  events().collect_frames(scene.map());
  Meter meter{state};
  auto& latency = meter.series("latency");
  auto& gpu_wait = meter.series("gpu_wait");
  size_t index = 0;
  bool gpu_waited = true;
  for (auto _ : state) {
    BENCH_CHECK(mln_map_update_camera(
      scene.map(), &jumps[index++ % kLap], nullptr, diag()
    ));
    const auto frame = session.render(MLN_FRAME_DEMAND_WAIT_FOR_UPDATE);
    if (frame.result.disposition != MLN_RENDER_RESULT_RENDERED) {
      fail(
        "a moving camera's frame finished with disposition %u",
        frame.result.disposition
      );
    }
    latency.add(frame.drained - frame.demanded);
    gpu_wait.add(frame.completed - frame.drained);
    gpu_waited = gpu_waited && frame.gpu_waited;
  }
  meter.finish();
  report_engine_frames(state, events().take_frames());
  if (!gpu_waited) state.counters.erase("gpu_wait_p50_us");
  if (!gpu_waited) state.counters.erase("gpu_wait_p95_us");
  state.SetItemsProcessed(state.iterations());
  scene.save_frame(state);
}

constexpr auto kAnimation = std::chrono::milliseconds{2000};
constexpr double kFlyZoom = 1.5;

// The camera at `progress` from 0 to 1 of the fly: from the block's
// north-west corner at zoom 14 to its south-east corner at zoom 15.5, eased in
// and out. The source's tiles stop at zoom 14, so from zoom 15 on, halfway,
// the map lays the same data out again as zoom 15 tiles, as it does past a
// basemap's deepest zoom.
auto fly(double progress) -> mln_camera_update {
  const double eased = progress * progress * (3.0 - 2.0 * progress);
  const auto from = north_west();
  const auto to = south_east();
  return jump_to(
    {from.latitude + (to.latitude - from.latitude) * eased,
     from.longitude + (to.longitude - from.longitude) * eased},
    kZoom + kFlyZoom * eased
  );
}

// What one frame of the flight drew.
struct FlightFrame {
  uint64_t shown_generation;
  bool partial;
  size_t camera;
};

// From the start of a two-second camera flight, with only the first view's
// tiles loaded, until the final view renders complete. At each refresh the
// host starts a frame when none is in flight, with the camera where the
// animation will be at the next refresh, and shows it at the first refresh
// after its GPU work finishes.
void frame_paced(benchmark::State& state) {
  const auto period = refresh_period();
  const auto ticks = static_cast<int64_t>(std::ceil(
    std::chrono::duration<double>(kAnimation).count() * environment().hz
  ));
  Scene scene;
  scene.load_benchmark_scene();
  auto& session = scene.session();

  // One completion per camera update, so each frame's camera generation is
  // known.
  std::vector<Completion> cameras(static_cast<size_t>(ticks + 1));
  std::vector<FlightFrame> flight;
  flight.reserve(cameras.size());
  Series frame_time;
  Series interval;
  std::vector<double> complete_after;
  complete_after.reserve(static_cast<size_t>(state.max_iterations));
  int64_t frames = 0;
  int64_t missed = 0;
  int64_t over_twice = 0;
  int64_t dropped = 0;
  int64_t partial = 0;
  int64_t stale = 0;
  Clock::duration longest{};
  std::vector<FrameEvent> engine_frames;
  Meter meter{state};
  for (auto _ : state) {
    state.PauseTiming();
    meter.pause();
    Completion cleared;
    BENCH_CHECK(mln_render_session_clear_data(
      session.handle(), cleared.descriptor(), diag()
    ));
    finish(cleared, "the clear", &session);
    const auto start = fly(0.0);
    BENCH_CHECK(mln_map_update_camera(scene.map(), &start, nullptr, diag()));
    session.settle(scene.map());
    events().collect_frames(scene.map());
    for (auto& camera : cameras) camera.reset();
    flight.clear();
    meter.resume();
    state.ResumeTiming();

    const auto t0 = Clock::now();
    Frame in_flight;
    bool flying = false;
    int64_t demanded_tick = 0;
    size_t sent = 0;
    Clock::time_point last_shown = t0;
    for (int64_t tick = 0; tick <= ticks || flying; ++tick) {
      const auto refresh = t0 + period * tick;
      // Waits for the refresh, handling results as they come.
      for (;;) {
        const auto seen = pump().generation();
        session.service();
        if (flying && session.poll(in_flight)) {
          flying = false;
          const auto& result = in_flight.result;
          if (result.disposition != MLN_RENDER_RESULT_RENDERED) {
            ++dropped;
            continue;
          }
          ++frames;
          flight.push_back(
            {result.map_update_generation, result.mode != MLN_RENDER_MODE_FULL,
             static_cast<size_t>(demanded_tick)}
          );
          const auto took = in_flight.completed - in_flight.demanded;
          frame_time.add(took);
          if (in_flight.completed > t0 + period * (demanded_tick + 1)) ++missed;
          if (took > period * 2) ++over_twice;
          // Shown at the first refresh after its GPU work.
          const auto since = in_flight.completed - t0;
          const auto shown =
            t0 + period * ((since + period - Clock::duration{1}) / period);
          const auto gap = shown - last_shown;
          interval.add(gap);
          longest = std::max(longest, gap);
          last_shown = shown;
          continue;
        }
        if (Clock::now() >= refresh) break;
        if (Clock::now() - t0 > kAnimation + kWaitLimit) {
          fail("a paced frame never finished");
        }
        pump().wait(seen, refresh);
      }
      if (tick > ticks || flying) continue;
      // Waiting for a frame's GPU work can pass several refreshes; like a host
      // on a display clock, start the next frame at the current one.
      tick = std::min(
        ticks,
        std::max(tick, static_cast<int64_t>((Clock::now() - t0) / period))
      );
      // Each frame moves the camera, so each demand has an update to wait
      // for.
      const double progress =
        static_cast<double>(tick + 1) / static_cast<double>(ticks + 1);
      const auto camera = fly(progress);
      auto& completion = cameras[static_cast<size_t>(tick)];
      BENCH_CHECK(mln_map_update_camera(
        scene.map(), &camera, completion.descriptor(), diag()
      ));
      sent = static_cast<size_t>(tick) + 1;
      in_flight = Frame{};
      in_flight.result.token = session.next_token();
      in_flight.demanded = Clock::now();
      demanded_tick = tick;
      session.demand(MLN_FRAME_DEMAND_WAIT_FOR_UPDATE, in_flight.result.token);
      flying = true;
    }
    // A frame still in flight at the last refresh skipped the final camera, so
    // every side still ends on the same view.
    if (sent != cameras.size()) {
      const auto camera = fly(1.0);
      sent = cameras.size();
      BENCH_CHECK(mln_map_update_camera(
        scene.map(), &camera, cameras[sent - 1].descriptor(), diag()
      ));
    }
    // The flight ends at its last refresh; the final camera's view should be
    // complete by then.
    const auto flight_end = t0 + period * ticks;
    auto& last_camera = cameras[sent - 1];
    finish(last_camera, "the last camera update", &session);
    Clock::duration completed_after{};
    if (
      !flight.empty() && !flight.back().partial &&
      flight.back().camera + 1 == sent &&
      flight.back().shown_generation >= last_camera.render_update_generation
    ) {
      completed_after = std::max(Clock::duration{}, last_shown - flight_end);
    } else {
      const auto presented =
        session.present_until_full(last_camera.render_update_generation);
      completed_after = presented.frame.completed - flight_end;
    }
    complete_after.push_back(
      std::chrono::duration<double, std::milli>(completed_after).count()
    );
    const auto flown = events().take_frames();
    engine_frames.insert(engine_frames.end(), flown.begin(), flown.end());
    for (const auto& frame : flight) {
      auto& camera = cameras[frame.camera];
      finish(camera, "a camera update", &session);
      if (frame.partial) ++partial;
      if (frame.shown_generation < camera.render_update_generation) ++stale;
    }
  }
  meter.finish();
  report_engine_frames(state, engine_frames);
  frame_time.report_distribution(state, "frame");
  interval.report_distribution(state, "interval");
  if (!complete_after.empty()) {
    std::sort(complete_after.begin(), complete_after.end());
    state.counters["complete_after_ms"] =
      complete_after[complete_after.size() / 2];
  }
  const auto shown = static_cast<double>(std::max<int64_t>(frames, 1));
  state.counters["frames"] = benchmark::Counter(
    static_cast<double>(frames), benchmark::Counter::kAvgIterations
  );
  state.counters["missed_deadline_pct"] =
    100.0 * static_cast<double>(missed) / shown;
  state.counters["over_2x_budget"] = benchmark::Counter(
    static_cast<double>(over_twice), benchmark::Counter::kAvgIterations
  );
  state.counters["partial_frame_pct"] =
    100.0 * static_cast<double>(partial) / shown;
  state.counters["stale_frame_pct"] =
    100.0 * static_cast<double>(stale) / shown;
  state.counters["longest_stall_ms"] =
    std::chrono::duration<double, std::milli>(longest).count();
  if (dropped > 0) state.counters["dropped"] = static_cast<double>(dropped);
  // State::name() leaves out the case's argument.
  scene.save_frame(state, "Frame/Paced/" + std::to_string(state.range(0)));
}

constexpr auto kIdleSlice = std::chrono::milliseconds{100};

// 100 ms of a settled map with an attached session, where the host waits for
// wakes as an idle app does. The wait is fixed because idleness is the absence
// of an event.
void frame_idle(benchmark::State& state) {
  Scene scene;
  scene.load_benchmark_scene();
  auto& session = scene.session();
  const auto count_wakes = [] {
    auto& counts = wakes();
    return counts.frame.load() + counts.redraw.load() + counts.driver.load();
  };
  Meter meter{state};
  const auto before = count_wakes();
  for (auto _ : state) {
    const auto until = Clock::now() + kIdleSlice;
    while (Clock::now() < until) {
      const auto seen = pump().generation();
      session.service();
      pump().wait(seen, until);
    }
  }
  meter.finish();
  const auto seconds = static_cast<double>(state.iterations()) *
                       std::chrono::duration<double>(kIdleSlice).count();
  state.counters["wakes_per_s"] =
    static_cast<double>(count_wakes() - before) / seconds;
}

}  // namespace

void register_frame_cases() {
  add_case("Frame/Throughput", frame_throughput, 300);
  add_case("Frame/Paced", frame_paced, 3)
    ->Arg(static_cast<int64_t>(environment().hz))
    ->Unit(benchmark::kMillisecond);
  add_case("Frame/Idle", frame_idle, 10);
}

}  // namespace bench
