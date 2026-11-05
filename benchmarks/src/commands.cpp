// The command path: camera jumps, the command a host sends most, on a settled
// map whose session renders no frames meanwhile.

#include <array>

#include "bench.hpp"

namespace bench {
namespace {

auto two_jumps() -> std::array<mln_camera_update, 2> {
  const auto center = scene_center();
  return {
    jump_to(center),
    jump_to({center.latitude + 0.0005, center.longitude + 0.0005})
  };
}

// From the mln_map_update_camera() call until its completion arrives.
void command_round_trip(benchmark::State& state) {
  Scene scene;
  scene.load_benchmark_scene();
  const auto jumps = two_jumps();
  Completion completion;
  Meter meter{state};
  auto& submit = meter.series("submit");
  auto& latency = meter.series("latency");
  size_t index = 0;
  for (auto _ : state) {
    completion.reset();
    const auto start = Clock::now();
    BENCH_CHECK(mln_map_update_camera(
      scene.map(), &jumps[index++ & 1U], completion.descriptor(), nullptr
    ));
    submit.add_since(start);
    finish(completion, "the camera command", &scene.session());
    latency.add(completion.at - start);
  }
  meter.finish();
}

constexpr int kBurst = 1000;

// From the first of 1000 camera commands until a camera query sent after them
// completes.
void command_burst(benchmark::State& state) {
  Scene scene;
  scene.load_benchmark_scene();
  const auto jumps = two_jumps();
  Completion completion;
  Meter meter{state};
  Clock::duration submitting{};
  for (auto _ : state) {
    const auto start = Clock::now();
    for (int index = 0; index < kBurst; ++index) {
      BENCH_CHECK(mln_map_update_camera(
        scene.map(), &jumps[static_cast<size_t>(index) & 1U], nullptr, nullptr
      ));
    }
    submitting += Clock::now() - start;
    completion.reset();
    BENCH_CHECK(
      mln_map_get_camera(scene.map(), completion.descriptor(), nullptr)
    );
    finish(completion, "the camera query", &scene.session());
  }
  meter.finish();
  const auto commands = static_cast<double>(state.iterations()) * kBurst;
  state.SetItemsProcessed(static_cast<int64_t>(commands));
  state.counters["submit_ns"] =
    std::chrono::duration<double, std::nano>(submitting).count() / commands;
}

}  // namespace

void register_command_cases() {
  add_case("Command/RoundTrip", command_round_trip, 2000);
  add_case("Command/Burst", command_burst, 20);
}

}  // namespace bench
