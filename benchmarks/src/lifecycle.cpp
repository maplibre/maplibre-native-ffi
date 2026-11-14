// What a caller pays to start and stop the library's objects.

#include "bench.hpp"

namespace bench {
namespace {

constexpr const char* kBackgroundStyle =
  R"({"version":8,"sources":{},"layers":[{"id":"background","type":"background",)"
  R"("paint":{"background-color":"#d8e2dc"}}]})";

// From mln_runtime_create() until the runtime's release completes. `ready`
// ends when its first command, the event handler, completes.
void runtime_lifecycle(benchmark::State& state) {
  Meter meter{state};
  auto& ready = meter.series("ready");
  auto& release_return = meter.series("release_return");
  auto& release_done = meter.series("release_done");
  Completion completion;
  for (auto _ : state) {
    const auto start = Clock::now();
    mln_runtime runtime = MLN_HANDLE_NULL;
    BENCH_CHECK(mln_runtime_create(nullptr, &runtime, diag()));
    completion.reset();
    start_event_handler(runtime, completion);
    finish(completion, "the event handler");
    ready.add_since(start);

    completion.reset();
    const auto release = Clock::now();
    BENCH_CHECK(mln_runtime_release(runtime, completion.descriptor(), diag()));
    release_return.add_since(release);
    finish(completion, "the runtime release");
    release_done.add_since(release);
  }
  meter.finish();
}

// From mln_runtime_create_map() on a running runtime, through the attach and
// the first frame's GPU work, until the map's release completes. The style is
// one background layer, so nothing loads, and the ring is created untimed.
void map_lifecycle(benchmark::State& state) {
  Scene scene;
  const auto runtime = scene.runtime();
  const auto options = map_options();
  Ring ring;
  ring.create();
  Meter meter{state};
  auto& create = meter.series("create");
  auto& attach = meter.series("attach");
  auto& first_frame = meter.series("first_frame");
  auto& teardown = meter.series("teardown");
  Completion completion;
  for (auto _ : state) {
    auto start = Clock::now();
    mln_map map = MLN_HANDLE_NULL;
    completion.reset();
    BENCH_CHECK(mln_runtime_create_map(
      runtime, &options, &map, completion.descriptor(), nullptr
    ));
    BENCH_CHECK(
      mln_map_set_style_json(map, view(kBackgroundStyle), nullptr, nullptr)
    );
    finish(completion, "the map creation");
    create.add_since(start);

    Session session;
    session.borrow_ring(ring);
    // Unlike the old C API's attach, this one first wraps the host's device.
    start = Clock::now();
    completion.reset();
    session.start_attach(map, completion);
    finish(completion, "the attach", &session);
    session.attached();
    attach.add_since(start);

    start = Clock::now();
    const auto frame = session.render(MLN_FRAME_DEMAND_FORCE);
    if (frame.result.disposition != MLN_RENDER_RESULT_RENDERED) {
      fail(
        "the first frame finished with disposition %u", frame.result.disposition
      );
    }
    first_frame.add_since(start);

    start = Clock::now();
    session.release();
    completion.reset();
    BENCH_CHECK(mln_map_release(map, completion.descriptor(), nullptr));
    finish(completion, "the map release");
    teardown.add_since(start);
  }
  meter.finish();
}

}  // namespace

void register_lifecycle_cases() {
  add_case("Runtime/Lifecycle", runtime_lifecycle, 20);
  add_case("Map/Lifecycle", map_lifecycle, 10);
}

}  // namespace bench
