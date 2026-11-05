// What the library keeps in memory once an operation has settled: `heap_mib`,
// the malloc heap in use, and `rss_mib`, the resident memory, each above what
// it was when the case started, read untimed and averaged over the
// iterations.

#include <array>
#include <cstdio>
#include <string>
#include <vector>

#include "bench.hpp"

namespace bench {
namespace {

// Adds the heap and resident memory now, above where they were at
// construction, to sums that report() averages.
struct MemorySample {
  double start_heap_kib = static_cast<double>(heap_in_use_kib());
  double start_rss_kib = static_cast<double>(resident_kib());
  double heap_kib = 0.0;
  double rss_kib = 0.0;

  void add() {
    if (start_heap_kib > 0.0) {
      heap_kib += static_cast<double>(heap_in_use_kib()) - start_heap_kib;
    }
    if (start_rss_kib > 0.0) {
      rss_kib += static_cast<double>(resident_kib()) - start_rss_kib;
    }
  }

  void report(benchmark::State& state, const std::string& prefix) const {
    const auto per_iteration = benchmark::Counter::kAvgIterations;
    if (start_heap_kib > 0.0) {
      state.counters[prefix + "heap_mib"] =
        benchmark::Counter(heap_kib / 1024.0, per_iteration);
    }
    if (start_rss_kib > 0.0) {
      state.counters[prefix + "rss_mib"] =
        benchmark::Counter(rss_kib / 1024.0, per_iteration);
    }
  }
};

void collect_sourced_layers(
  Completion& completion, const mln_completion_result& result
) {
  auto& ids = *static_cast<std::vector<std::string>*>(completion.context);
  for (size_t index = 0; index < result.value_count; ++index) {
    const auto* layer = MLN_STRIDED_AT(
      mln_style_layer_info, result.value, result.value_size, index
    );
    if (layer->source_id.size == 0) continue;
    ids.emplace_back(static_cast<const char*>(layer->id.data), layer->id.size);
  }
}

constexpr double kInRange = 15.5;
constexpr double kOutOfRange = 14.5;

// From a jump from zoom 15.5 to 14.5, out of the range of every layer that
// draws from a source, until the first complete frame there has finished on
// the GPU, on a map with its tile cache off. Untimed, the memory is read once
// the map settles there, and again, as `in_range_`, back at zoom 15.5.
void leave_zoom_range(benchmark::State& state) {
  MemorySample out_of_range;
  MemorySample in_range;
  Scene scene;
  auto options = map_options();
  options.fields |= MLN_MAP_OPTION_TILE_CACHE_ENABLED;
  options.tile_cache_enabled = false;
  scene.create_map(options);
  scene.load_style(benchmark_style());
  auto& session = scene.session();
  session.attach(scene.map());

  std::vector<std::string> layers;
  Completion completion;
  completion.hook = collect_sourced_layers;
  completion.context = &layers;
  BENCH_CHECK(
    mln_map_list_style_layers(scene.map(), completion.descriptor(), diag())
  );
  finish(completion, "the layer list", &session);
  completion.hook = nullptr;
  for (const auto& id : layers) {
    completion.reset();
    BENCH_CHECK(mln_map_set_style_layer_property(
      scene.map(), view(id), view("minzoom"), view("15"),
      completion.descriptor(), diag()
    ));
    finish(completion, "the layer's minzoom", &session);
  }
  session.present_until_full(scene.jump(scene_center(), kInRange));
  session.settle(scene.map());

  Meter meter{state};
  for (auto _ : state) {
    session.present_until_full(scene.jump(scene_center(), kOutOfRange));
    state.PauseTiming();
    meter.pause();
    session.settle(scene.map());
    out_of_range.add();
    session.present_until_full(scene.jump(scene_center(), kInRange));
    session.settle(scene.map());
    in_range.add();
    meter.resume();
    state.ResumeTiming();
  }
  meter.finish();
  out_of_range.report(state, "");
  in_range.report(state, "in_range_");
  state.counters["layers"] = static_cast<double>(layers.size());
  scene.save_frame(state);
}

constexpr size_t kMaps = 4;

// Disposes of the maps that a failure left unreleased.
struct Maps {
  Maps() = default;
  Maps(const Maps&) = delete;
  auto operator=(const Maps&) -> Maps& = delete;
  ~Maps() {
    for (auto map : handles) {
      if (map != MLN_HANDLE_NULL) mln_map_dispose(map, nullptr);
    }
  }
  std::array<mln_map, kMaps> handles{};
};

// From the first of four mln_runtime_create_map() calls on a running runtime,
// one map after another, until the fourth has shown `style` complete and its
// GPU work has finished. Untimed, the memory is read with all four loaded, and
// the maps are released.
void maps_on_one_runtime(benchmark::State& state, const std::string& style) {
  MemorySample loaded;
  Scene scene;
  const auto runtime = scene.runtime();
  const auto options = map_options();
  const auto update = jump_to(scene_center());
  std::array<Ring, kMaps> rings;
  for (auto& ring : rings) ring.create();
  Image last_frame;
  Completion completion;
  Meter meter{state};
  for (auto _ : state) {
    Maps owned;
    auto& maps = owned.handles;
    std::array<Session, kMaps> sessions;
    for (size_t index = 0; index < kMaps; ++index) {
      auto& map = maps[index];
      completion.reset();
      BENCH_CHECK(mln_runtime_create_map(
        runtime, &options, &map, completion.descriptor(), diag()
      ));
      finish(completion, "the map creation");
      BENCH_CHECK(mln_map_update_camera(map, &update, nullptr, diag()));
      completion.reset();
      BENCH_CHECK(mln_map_set_style_json(
        map, view(style), completion.descriptor(), diag()
      ));
      finish(completion, "the style load");
      const auto generation = completion.render_update_generation;
      auto& session = sessions[index];
      session.borrow_ring(rings[index]);
      session.attach(map);
      session.present_until_full(generation);
    }
    state.PauseTiming();
    meter.pause();
    for (size_t index = 0; index < kMaps; ++index) {
      sessions[index].settle(maps[index]);
    }
    loaded.add();
    if (environment().images) {
      try {
        last_frame = sessions[kMaps - 1].capture();
      } catch (const Failure& failure) {
        state.SetLabel(std::string{"no frame: "} + failure.what());
      }
    }
    for (size_t index = 0; index < kMaps; ++index) {
      sessions[index].release();
      completion.reset();
      BENCH_CHECK(
        mln_map_release(maps[index], completion.descriptor(), diag())
      );
      maps[index] = MLN_HANDLE_NULL;
      finish(completion, "the map release");
    }
    meter.resume();
    state.ResumeTiming();
  }
  meter.finish();
  loaded.report(state, "");
  if (!last_frame.rgba.empty()) keep_image(case_key(state), last_frame);
}

void four_basemaps(benchmark::State& state) {
  maps_on_one_runtime(state, benchmark_style());
}

// Appends a code point from U+0800 to U+FFFF as UTF-8.
void append_utf8(std::string& out, uint32_t code_point) {
  out += static_cast<char>(0xE0U | (code_point >> 12U));
  out += static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU));
  out += static_cast<char>(0x80U | (code_point & 0x3FU));
}

// A background and 600 point labels around the scene's center, each three Han
// characters from all of U+4E00 to U+9FFF and a kana or hangul syllable, so
// that every map loads about 130 glyph ranges.
auto cjk_style() -> const std::string& {
  static const std::string style = [] {
    const auto center = scene_center();
    std::string features;
    char point[160];
    for (uint32_t index = 0; index < 600; ++index) {
      const auto hash = [&](uint32_t salt) { return mix(index * 8U + salt); };
      std::string name;
      for (uint32_t salt = 0; salt < 3; ++salt) {
        append_utf8(name, 0x4E00U + hash(salt) % 0x5200U);
      }
      append_utf8(
        name,
        index % 2 == 0 ? 0x3041U + hash(3) % 0xBEU : 0xAC00U + hash(3) % 11172U
      );
      std::snprintf(
        point, sizeof(point),
        R"(%s{"type":"Feature","properties":{"name":"%s"},)"
        R"("geometry":{"type":"Point","coordinates":[%.6f,%.6f]}})",
        index == 0 ? "" : ",", name.c_str(),
        center.longitude + (hash(4) / 4294967296.0 - 0.5) * 0.03,
        center.latitude + (hash(5) / 4294967296.0 - 0.5) * 0.02
      );
      features += point;
    }
    return with_fixture_urls(
      R"({"version":8,"glyphs":"mln-bench-fixtures:/basemap/glyphs/{fontstack}/{range}.pbf",)"
      R"("sources":{"labels":{"type":"geojson","data":{"type":"FeatureCollection","features":[)" +
      features +
      R"(]}}},"layers":[{"id":"background","type":"background","paint":{"background-color":"#f2efe9"}},)"
      R"({"id":"labels","type":"symbol","source":"labels","layout":{"text-field":["get","name"],)"
      R"("text-font":["noto_sans_regular"],"text-size":14,"text-allow-overlap":true,)"
      R"("text-ignore-placement":true}}]})"
    );
  }();
  return style;
}

void four_cjk_maps(benchmark::State& state) {
  maps_on_one_runtime(state, cjk_style());
}

}  // namespace

void register_memory_cases() {
  add_case("Memory/LeaveZoomRange", leave_zoom_range, 10)
    ->Unit(benchmark::kMillisecond);
  add_case("Map/FourOnOneRuntime", four_basemaps, 5)
    ->Unit(benchmark::kMillisecond);
  add_case("Map/FourOnOneRuntimeCjk", four_cjk_maps, 5)
    ->Unit(benchmark::kMillisecond);
}

}  // namespace bench
