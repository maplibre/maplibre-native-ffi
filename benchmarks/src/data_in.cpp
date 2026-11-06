// Data that a caller sends in: GeoJSON, style documents, style edits, and
// images.

#include <array>
#include <cstdio>
#include <string>
#include <vector>

#include "bench.hpp"

namespace bench {
namespace {

// A feature collection of `count` points around the scene's center. The two
// variants put every point in a slightly different place.
auto points_json(int count, int variant) -> std::string {
  const auto center = scene_center();
  std::string json = R"({"type":"FeatureCollection","features":[)";
  char feature[256];
  for (int index = 0; index < count; ++index) {
    const double x =
      mix(static_cast<uint32_t>(index) * 2U + 1234567U) / 4294967296.0 - 0.5;
    const double y =
      mix(static_cast<uint32_t>(index) * 2U + 1234568U) / 4294967296.0 - 0.5;
    std::snprintf(
      feature, sizeof(feature),
      R"(%s{"type":"Feature","id":%d,"properties":{"category":%d},)"
      R"("geometry":{"type":"Point","coordinates":[%.6f,%.6f]}})",
      index == 0 ? "" : ",", index, index % 4,
      center.longitude + x * 0.03 + variant * 0.0002, center.latitude + y * 0.02
    );
    json += feature;
  }
  json += "]}";
  return json;
}

// From mln_map_set_geo_json_source_data() with 1000 points until the first
// complete frame that shows them has finished on the GPU, with frames on the
// display's clock. "call" is the time the caller's thread spends in the call.
void geojson_replace(benchmark::State& state) {
  Scene scene;
  scene.create_map();
  scene.load_style(benchmark_style());
  const std::array<std::string, 2> datasets{
    points_json(1000, 0), points_json(1000, 1)
  };
  Completion completion;
  BENCH_CHECK(mln_map_add_geo_json_source(
    scene.map(), view("points"), nullptr, {nullptr, 0}, completion.descriptor(),
    nullptr
  ));
  finish(completion, "the GeoJSON source");
  completion.reset();
  BENCH_CHECK(mln_map_set_geo_json_source_data(
    scene.map(), view("points"), view(datasets[1]), completion.descriptor(),
    nullptr
  ));
  finish(completion, "the GeoJSON data");
  completion.reset();
  BENCH_CHECK(mln_map_add_style_layer_json(
    scene.map(),
    view(
      R"({"id":"points","type":"circle","source":"points","paint":{)"
      R"("circle-radius":4,"circle-color":["match",["get","category"],0,"#e63946",)"
      R"(1,"#457b9d",2,"#2a9d8f","#f4a261"],"circle-stroke-width":1,)"
      R"("circle-stroke-color":"#ffffff"}})"
    ),
    {nullptr, 0}, completion.descriptor(), nullptr
  ));
  finish(completion, "the circle layer");
  scene.session().attach(scene.map());
  scene.session().settle(scene.map());

  Meter meter{state};
  auto& call = meter.series("call");
  auto& latency = meter.series("latency");
  size_t index = 0;
  for (auto _ : state) {
    const auto start = Clock::now();
    completion.reset();
    BENCH_CHECK(mln_map_set_geo_json_source_data(
      scene.map(), view("points"), view(datasets[index++ & 1U]),
      completion.descriptor(), nullptr
    ));
    call.add_since(start);
    finish(completion, "the GeoJSON replacement", &scene.session());
    scene.session().present_until_full(completion.render_update_generation);
    latency.add_since(start);
  }
  meter.finish();
  scene.save_frame(state);
}

// From mln_map_set_style_json() with the benchmark style until it completes.
// No session is attached, so no tile or glyph loads.
void style_load(benchmark::State& state) {
  Scene scene;
  scene.create_map();
  const auto& style = benchmark_style();
  std::string renamed = style;
  const auto brace = renamed.find('{');
  renamed.insert(brace + 1, R"("name":"benchmark variant",)");
  const std::array<const std::string*, 2> documents{&style, &renamed};
  Completion completion;
  Meter meter{state};
  size_t index = 0;
  for (auto _ : state) {
    completion.reset();
    BENCH_CHECK(mln_map_set_style_json(
      scene.map(), view(*documents[index++ & 1U]), completion.descriptor(),
      nullptr
    ));
    finish(completion, "the style load");
  }
  meter.finish();
}

// From mln_map_set_style_layer_property() on the buildings' fill color until
// the first complete frame that shows it has finished on the GPU, with frames
// on the display's clock.
void style_set_paint_property(benchmark::State& state) {
  Scene scene;
  scene.load_benchmark_scene();
  const std::array<const char*, 2> colors{R"("#c9b8a8")", R"("#b8a898")"};
  Completion completion;
  Meter meter{state};
  auto& latency = meter.series("latency");
  size_t index = 0;
  for (auto _ : state) {
    const auto start = Clock::now();
    completion.reset();
    BENCH_CHECK(mln_map_set_style_layer_property(
      scene.map(), view("buildings"), view("fill-color"),
      view(colors[index++ & 1U]), completion.descriptor(), nullptr
    ));
    finish(completion, "the paint property", &scene.session());
    scene.session().present_until_full(completion.render_update_generation);
    latency.add_since(start);
  }
  meter.finish();
  scene.save_frame(state);
}

constexpr int kLayerCount = 100;

// From the first of 100 mln_map_add_style_layer_json() calls, each a filtered
// line layer over the loaded streets, until the first complete frame after the
// 100 removals that follow, with frames on the display's clock. `added` ends
// at the first complete frame that shows all 100.
void style_add_remove_layers(benchmark::State& state) {
  Scene scene;
  scene.load_benchmark_scene();
  std::vector<std::string> layers;
  std::vector<std::string> ids;
  for (int index = 0; index < kLayerCount; ++index) {
    ids.push_back("bench-" + std::to_string(index));
    char layer[512];
    std::snprintf(
      layer, sizeof(layer),
      R"({"id":"%s","type":"line","source":"basemap","source-layer":"streets",)"
      R"("filter":["==",["get","kind"],"%s"],"paint":{"line-color":"#%06x",)"
      R"("line-width":%d}})",
      ids.back().c_str(),
      std::array{
        "primary", "secondary", "tertiary", "residential", "service"
      }[index % 5],
      static_cast<unsigned>(mix(static_cast<uint32_t>(index)) & 0xFFFFFFU),
      1 + index % 4
    );
    layers.emplace_back(layer);
  }
  Completion completion;
  auto& session = scene.session();
  Meter meter{state};
  auto& added = meter.series("added");
  for (auto _ : state) {
    const auto start = Clock::now();
    for (size_t index = 0; index < layers.size(); ++index) {
      const bool last = index + 1 == layers.size();
      if (last) completion.reset();
      BENCH_CHECK(mln_map_add_style_layer_json(
        scene.map(), view(layers[index]), {nullptr, 0},
        last ? completion.descriptor() : nullptr, nullptr
      ));
    }
    finish(completion, "the layer additions", &session);
    session.present_until_full(completion.render_update_generation);
    added.add_since(start);
    for (int index = 0; index < kLayerCount; ++index) {
      const bool last = index == kLayerCount - 1;
      if (last) completion.reset();
      BENCH_CHECK(mln_map_remove_style_layer(
        scene.map(), view(ids[static_cast<size_t>(index)]),
        last ? completion.descriptor() : nullptr, nullptr
      ));
    }
    finish(completion, "the layer removals", &session);
    session.present_until_full(completion.render_update_generation);
  }
  meter.finish();
  scene.save_frame(state);
}

// From mln_map_set_style_image() with a 256 x 256 image until it completes.
void style_image_set(benchmark::State& state) {
  Scene scene;
  scene.create_map();
  scene.load_style(benchmark_style());
  constexpr uint32_t kSide = 256;
  std::vector<uint8_t> pixels(static_cast<size_t>(kSide) * kSide * 4);
  for (size_t index = 0; index < pixels.size(); index += 4) {
    const auto value =
      static_cast<uint8_t>(mix(static_cast<uint32_t>(index)) & 0xFFU);
    pixels[index] = value;
    pixels[index + 1] = static_cast<uint8_t>(255 - value);
    pixels[index + 2] = 128;
    pixels[index + 3] = 255;
  }
  mln_premultiplied_rgba8_image image{};
  image.size = sizeof(image);
  image.width = kSide;
  image.height = kSide;
  image.stride = kSide * 4;
  image.pixels = pixels.data();
  image.byte_length = pixels.size();
  Completion completion;
  Meter meter{state};
  auto& submit = meter.series("submit");
  for (auto _ : state) {
    completion.reset();
    const auto start = Clock::now();
    BENCH_CHECK(mln_map_set_style_image(
      scene.map(), view("bench-image"), &image, nullptr,
      completion.descriptor(), nullptr
    ));
    submit.add_since(start);
    finish(completion, "the style image");
  }
  meter.finish();
  state.SetBytesProcessed(
    static_cast<int64_t>(state.iterations()) *
    static_cast<int64_t>(pixels.size())
  );
}

}  // namespace

void register_data_in_cases() {
  add_case("GeoJson/Replace", geojson_replace, 30);
  add_case("Style/Load", style_load, 30);
  add_case("Style/SetPaintProperty", style_set_paint_property, 50);
  add_case("Style/AddRemoveLayers", style_add_remove_layers, 10);
  add_case("StyleImage/Set", style_image_set, 500);
}

}  // namespace bench
