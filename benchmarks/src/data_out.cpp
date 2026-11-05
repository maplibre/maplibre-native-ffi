// Data that a caller reads out: rendered features, pixels, and projections.

#include <array>
#include <optional>

#include "bench.hpp"

namespace bench {
namespace {

// From mln_render_session_query_rendered_features() over a 128 x 128 box in
// the middle of the settled scene, as for a tap, until it completes.
void query_rendered_features(benchmark::State& state) {
  Scene scene;
  scene.load_benchmark_scene();
  mln_rendered_query_geometry geometry{};
  geometry.size = sizeof(geometry);
  geometry.type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX;
  constexpr double kHalf = 64.0;
  const double x = kWidth / 2.0;
  const double y = kHeight / 2.0;
  geometry.data.box = {{x - kHalf, y - kHalf}, {x + kHalf, y + kHalf}};
  Completion completion;
  Meter meter{state};
  auto& latency = meter.series("latency");
  size_t features = 0;
  for (auto _ : state) {
    completion.reset();
    const auto start = Clock::now();
    BENCH_CHECK(mln_render_session_query_rendered_features(
      scene.session().handle(), &geometry, nullptr, completion.descriptor(),
      nullptr
    ));
    finish(completion, "the rendered feature query", &scene.session());
    latency.add(completion.at - start);
    features = completion.value_count;
  }
  meter.finish();
  state.counters["features"] = static_cast<double>(features);
}

void on_queue_wake(void*) { pump().notify(); }

// A delivery queue that the benchmark thread dispatches.
struct Queue {
  Queue() {
    mln_delivery_queue_options options{};
    options.size = sizeof(options);
    options.wake.callback = on_queue_wake;
    BENCH_CHECK(mln_delivery_queue_create(&options, &handle, diag()));
  }
  Queue(const Queue&) = delete;
  auto operator=(const Queue&) -> Queue& = delete;
  ~Queue() { mln_delivery_queue_release(handle); }
  mln_delivery_queue handle = MLN_HANDLE_NULL;
};

// From mln_render_session_read_texture() on an offscreen session of the
// settled scene until the completion has copied the pixels. With `queued`, the
// benchmark thread dispatches the completion from a delivery queue.
void readback(benchmark::State& state, bool queued) {
  Scene scene;
  scene.create_map();
  scene.load_style(benchmark_style());
  scene.session().attach_offscreen(scene.map());
  scene.session().settle(scene.map());
  Image pixels;
  pixels.rgba.reserve(static_cast<size_t>(kWidth) * kHeight * 4);
  Completion completion;
  completion.hook = copy_readback;
  completion.context = &pixels;
  std::optional<Queue> queue;
  if (queued) {
    queue.emplace();
    completion.queue = queue->handle;
  }
  Meter meter{state};
  auto& latency = meter.series("latency");
  for (auto _ : state) {
    completion.reset();
    const auto start = Clock::now();
    BENCH_CHECK(mln_render_session_read_texture(
      scene.session().handle(), completion.descriptor(), nullptr
    ));
    if (queue) {
      pump_until(
        [&] {
          BENCH_CHECK(mln_delivery_queue_dispatch(queue->handle, diag()));
          return completion.done.load(std::memory_order_acquire);
        },
        "the queued readback", &scene.session()
      );
    }
    finish(completion, "the readback", &scene.session());
    latency.add_since(start);
  }
  meter.finish();
  state.SetBytesProcessed(
    static_cast<int64_t>(state.iterations()) *
    static_cast<int64_t>(pixels.rgba.size())
  );
  scene.save_frame(state);
}

void readback_offscreen(benchmark::State& state) { readback(state, false); }
void readback_queued(benchmark::State& state) { readback(state, true); }

// One mln_map_projection_pixel_for_lat_lng() call, the overhead of a cheap
// synchronous call.
void projection_pixel_for_lat_lng(benchmark::State& state) {
  Scene scene;
  scene.create_map();
  Completion completion;
  BENCH_CHECK(
    mln_map_get_camera(scene.map(), completion.descriptor(), nullptr)
  );
  finish(completion, "the camera query");
  mln_map_snapshot snapshot{};
  snapshot.size = sizeof(snapshot);
  BENCH_CHECK(mln_map_get_snapshot(scene.map(), &snapshot, nullptr));
  mln_map_projection projection = MLN_HANDLE_NULL;
  BENCH_CHECK(mln_map_projection_create(&snapshot.view, &projection, nullptr));
  const auto center = scene_center();
  const std::array<mln_lat_lng, 2> coordinates{
    center, mln_lat_lng{center.latitude + 0.001, center.longitude - 0.001}
  };
  Meter meter{state};
  size_t index = 0;
  for (auto _ : state) {
    mln_screen_point point{};
    BENCH_CHECK(mln_map_projection_pixel_for_lat_lng(
      projection, coordinates[index++ & 1U], &point, nullptr
    ));
    benchmark::DoNotOptimize(point);
  }
  meter.finish();
  mln_map_projection_release(projection);
}

}  // namespace

void register_data_out_cases() {
  add_case("Query/RenderedFeatures", query_rendered_features, 50);
  add_case("Readback/Offscreen", readback_offscreen, 50);
  add_case("Readback/Queued", readback_queued, 50);
  add_case("Projection/PixelForLatLng", projection_pixel_for_lat_lng, 0);
}

}  // namespace bench
