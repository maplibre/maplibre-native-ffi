// GeoJSON data and tiling: background preparation, ordered against newer
// data and against what cancels it by parking the preparation, and
// asynchronous slices on a data's sequenced worker, ordered against data
// replacement and the synchronous tiling override by parking the worker at
// its slice.

#include <cstdio>
#include <cstring>
#include <memory>

#include <mln/map/map.hpp>
#include <mln/style/sources/geojson_source.hpp>
#include <mln/style/sources/geojson_source_impl.hpp>
#include <mln/style/style.hpp>

#include "internal/support/checks.hpp"
#include "internal/support/sync_points.hpp"
#include "map/map.hpp"
#include "map/map_internal.hpp"
#include "maplibre_native_c.h"
#include "runtime/runtime.hpp"
#include "support/style.h"
#include "support/test_support.h"

namespace {

using mln::native_tests::await;
using mln::native_tests::SyncPoint;
using mln::native_tests::SyncPointScope;

// One point at the map's center, named so a query can tell datasets apart.
#define POINT_COLLECTION(name)                                           \
  "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\"," \
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},"             \
  "\"properties\":{\"name\":\"" name "\"}}]}"

constexpr char empty_style_json[] = R"({"version":8,"sources":{},"layers":[]})";

// A map with a GeoJSON source named "points", drawn by a circle layer so
// that rendering requests its tiles.
void add_drawn_source(mln_map map) {
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(empty_style_json));
  MLN_TEST_AWAIT_OK(mln_map_add_geo_json_source(
    map, MLN_BUFFER_LITERAL("points"), nullptr, MLN_BUFFER_LITERAL(""),
    &completion.descriptor, nullptr
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map,
    MLN_BUFFER_LITERAL(R"({"id":"dots","type":"circle","source":"points"})"),
    MLN_BUFFER_LITERAL(""), &completion.descriptor, nullptr
  ));
}

void set_data(mln_map map, const char* json) {
  MLN_TEST_AWAIT_OK(mln_map_set_geo_json_source_data(
    map, MLN_BUFFER_LITERAL("points"), mln_buffer_view_from_cstr(json),
    &completion.descriptor, nullptr
  ));
}

// Submits `json` for the "points" source with `completion`.
void submit_data(
  mln_map map, const char* json, mln_test_completion& completion
) {
  MLN_TEST_OK(mln_map_set_geo_json_source_data(
    map, MLN_BUFFER_LITERAL("points"), mln_buffer_view_from_cstr(json),
    &completion.descriptor, nullptr
  ));
}

// The index that the "points" source holds, read on the runtime worker.
auto index_of(mln_map map) -> std::weak_ptr<mln::style::GeoJSONData> {
  auto index = std::weak_ptr<mln::style::GeoJSONData>{};
  MLN_TEST_AWAIT_OK(
    mln::core::submit_map_command(
      map,
      [&index](mln::core::MapObject& live) -> mln_status {
        auto* source = live.map->getStyle().getSource("points");
        index = source->as<mln::style::GeoJSONSource>()->impl().getData();
        return MLN_STATUS_OK;
      },
      &completion.descriptor
    )
  );
  return index;
}

// Holds when the source holds exactly one feature, named `name`.
auto holds_the_name(const mln_test_render_fixture& fixture, const char* name)
  -> bool {
  char property[64];
  std::snprintf(property, sizeof(property), "\"name\":\"%s\"", name);
  const auto features =
    mln_test_style_query_source(&fixture, "points", nullptr);
  return features.status == MLN_STATUS_OK && features.count == 1 &&
         std::strstr(features.features[0].feature, property) != nullptr;
}

template <typename Predicate>
auto render_until(
  const mln_test_render_fixture& fixture, Predicate ready, const char* what
) -> bool {
  return mln_test_style_render_until(
    &fixture,
    [](void* context) -> bool { return (*static_cast<Predicate*>(context))(); },
    &ready, what
  );
}

// Issue #644: an asynchronous slice holds the data it slices, so when the
// data is replaced mid-slice the slice is its last owner and destroys it on
// the data's own worker. The worker's scheduler must outlive that, or the
// destruction joins the worker from itself and aborts the process. Parking
// the worker at its slice puts the replacement there every run.
void replaced_data_that_a_slice_outlives_retires_on_its_worker() {
  SyncPointScope sync;
  const auto runtime = mln_test_create_runtime();
  const auto map = mln_test_create_map(runtime);
  add_drawn_source(map);
  set_data(map, POINT_COLLECTION("first"));
  const auto first_index = index_of(map);

  auto fixture = mln_test_render_fixture{};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  sync.hold(SyncPoint::GeoJsonTileSlice);
  TEST_ASSERT_TRUE(render_until(
    fixture, [&] { return sync.hits(SyncPoint::GeoJsonTileSlice) >= 1; },
    "a slice of the first data"
  ));

  // The parked slice becomes the first data's last owner once the source and
  // every tile have moved on. The second data's slices park on their own
  // worker meanwhile, which a frame does not wait for.
  set_data(map, POINT_COLLECTION("second"));
  TEST_ASSERT_TRUE(render_until(
    fixture, [&] { return first_index.expired(); },
    "the source and tiles to let the first data go"
  ));

  sync.release(SyncPoint::GeoJsonTileSlice);
  TEST_ASSERT_TRUE(render_until(
    fixture, [&] { return holds_the_name(fixture, "second"); },
    "the second data's tiles"
  ));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// With the override on, a source slices tiles inline during the update pass,
// so installed data reaches the source's tiles without its worker. The data's
// worker stays parked at its slice throughout, so an asynchronous slice could
// never deliver. Laying out the sliced tile still runs on a tile worker, so the
// case renders until that layout lands rather than assuming one frame.
void the_synchronous_tiling_override_slices_without_the_datas_worker() {
  SyncPointScope sync;
  const auto runtime = mln_test_create_runtime();
  const auto map = mln_test_create_map(runtime);
  add_drawn_source(map);
  set_data(map, POINT_COLLECTION("first"));

  auto fixture = mln_test_render_fixture{};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  TEST_ASSERT_TRUE(render_until(
    fixture, [&] { return holds_the_name(fixture, "first"); },
    "the first data's tiles"
  ));

  MLN_TEST_AWAIT_OK(mln_map_set_geo_json_source_synchronous_tiling(
    map, MLN_BUFFER_LITERAL("points"), true, &completion.descriptor, nullptr
  ));
  // A synchronous source prepares its data in the command, so the command
  // completes while background preparation is parked.
  const auto preparations = sync.hits(SyncPoint::GeoJsonDataPrepare);
  sync.hold(SyncPoint::GeoJsonDataPrepare);
  sync.hold(SyncPoint::GeoJsonTileSlice);
  set_data(map, POINT_COLLECTION("second"));
  TEST_ASSERT_EQUAL_INT(preparations, sync.hits(SyncPoint::GeoJsonDataPrepare));
  TEST_ASSERT_TRUE(render_until(
    fixture, [&] { return holds_the_name(fixture, "second"); },
    "the second data's tiles while its worker is parked"
  ));

  sync.release(SyncPoint::GeoJsonTileSlice);
  sync.release(SyncPoint::GeoJsonDataPrepare);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// While a source prepares data, newer data waits behind it, and only the
// newest waits: each later call replaces the data that waits.
void only_the_newest_data_waits_behind_a_preparation() {
  SyncPointScope sync;
  const auto runtime = mln_test_create_runtime();
  const auto map = mln_test_create_map(runtime);
  add_drawn_source(map);

  sync.hold(SyncPoint::GeoJsonDataPrepare);
  auto first = mln_test_completion_default(0);
  submit_data(map, POINT_COLLECTION("first"), first);
  TEST_ASSERT_TRUE(sync.wait_for_hits(SyncPoint::GeoJsonDataPrepare, 1));
  auto second = mln_test_completion_default(0);
  submit_data(map, POINT_COLLECTION("second"), second);
  auto third = mln_test_completion_default(0);
  submit_data(map, POINT_COLLECTION("third"), third);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_CANCELLED, mln_test_completion_settle(&second)
  );
  TEST_ASSERT_FALSE(mln_test_completion_poll(&first));
  TEST_ASSERT_FALSE(mln_test_completion_poll(&third));

  sync.release(SyncPoint::GeoJsonDataPrepare);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_settle(&first));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_settle(&third));
  TEST_ASSERT_EQUAL_INT(2, sync.hits(SyncPoint::GeoJsonDataPrepare));

  auto fixture = mln_test_render_fixture{};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  TEST_ASSERT_TRUE(render_until(
    fixture, [&] { return holds_the_name(fixture, "third"); },
    "the third data's tiles"
  ));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void set_the_url(mln_map map) {
  MLN_TEST_AWAIT_OK(mln_map_set_geo_json_source_url(
    map, MLN_BUFFER_LITERAL("points"),
    MLN_BUFFER_LITERAL("fixture://missing.geojson"), &completion.descriptor,
    nullptr
  ));
}
void remove_the_source(mln_map map) {
  MLN_TEST_AWAIT_OK(mln_map_remove_style_layer(
    map, MLN_BUFFER_LITERAL("dots"), &completion.descriptor, nullptr
  ));
  MLN_TEST_AWAIT_OK(mln_map_remove_style_source(
    map, MLN_BUFFER_LITERAL("points"), &completion.descriptor, nullptr
  ));
}
void load_a_style(mln_map map) {
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(empty_style_json));
}
void tile_synchronously_and_set_data(mln_map map) {
  MLN_TEST_AWAIT_OK(mln_map_set_geo_json_source_synchronous_tiling(
    map, MLN_BUFFER_LITERAL("points"), true, &completion.descriptor, nullptr
  ));
  set_data(map, POINT_COLLECTION("synchronous"));
}

struct cancellation_case {
  const char* label;
  void (*replace)(mln_map map);
};

// Data in preparation, and the data that waits behind it, complete as
// cancelled when something else replaces the source's data first.
void replacing_the_sources_data_cancels_data_in_preparation() {
  static constexpr cancellation_case cases[] = {
    {"a URL", set_the_url},
    {"the source's removal", remove_the_source},
    {"a style load", load_a_style},
    {"synchronous data", tile_synchronously_and_set_data},
  };
  for (const auto& row : cases) {
    SyncPointScope sync;
    const auto runtime = mln_test_create_runtime();
    const auto map = mln_test_create_map(runtime);
    add_drawn_source(map);
    sync.hold(SyncPoint::GeoJsonDataPrepare);
    auto preparing = mln_test_completion_default(0);
    submit_data(map, POINT_COLLECTION("preparing"), preparing);
    TEST_ASSERT_TRUE(sync.wait_for_hits(SyncPoint::GeoJsonDataPrepare, 1));
    auto waiting = mln_test_completion_default(0);
    submit_data(map, POINT_COLLECTION("waiting"), waiting);

    row.replace(map);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_CANCELLED, mln_test_completion_settle(&preparing), row.label
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_CANCELLED, mln_test_completion_settle(&waiting), row.label
    );
    sync.release(SyncPoint::GeoJsonDataPrepare);

    mln_test_destroy_map(map);
    mln_test_destroy_runtime(runtime);
  }
}

// Releasing the map cancels its data in preparation.
void releasing_the_map_cancels_data_in_preparation() {
  SyncPointScope sync;
  const auto runtime = mln_test_create_runtime();
  const auto map = mln_test_create_map(runtime);
  add_drawn_source(map);
  sync.hold(SyncPoint::GeoJsonDataPrepare);
  auto preparing = mln_test_completion_default(0);
  submit_data(map, POINT_COLLECTION("preparing"), preparing);
  TEST_ASSERT_TRUE(sync.wait_for_hits(SyncPoint::GeoJsonDataPrepare, 1));

  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_CANCELLED, mln_test_completion_settle(&preparing)
  );
  sync.release(SyncPoint::GeoJsonDataPrepare);
  mln_test_destroy_runtime(runtime);
}

// A disposed runtime refuses new submissions but finishes the commands it
// accepted, so data in preparation still reaches its source.
void a_disposed_runtime_finishes_data_in_preparation() {
  SyncPointScope sync;
  // Untracked, since the case disposes both itself.
  auto runtime_options = mln_runtime_options{};
  runtime_options.size = sizeof(mln_runtime_options);
  auto runtime = mln_runtime{MLN_HANDLE_NULL};
  MLN_TEST_OK(mln_runtime_create(&runtime_options, &runtime, nullptr));
  const auto map_options = mln_test_map_options();
  auto map = mln_map{MLN_HANDLE_NULL};
  MLN_TEST_OK(mln_test_map_create_status(runtime, &map_options, &map));
  add_drawn_source(map);
  sync.hold(SyncPoint::GeoJsonDataPrepare);
  auto preparing = mln_test_completion_default(0);
  submit_data(map, POINT_COLLECTION("preparing"), preparing);
  TEST_ASSERT_TRUE(sync.wait_for_hits(SyncPoint::GeoJsonDataPrepare, 1));

  const auto retired = std::weak_ptr{mln::core::lease_runtime(runtime)};
  MLN_TEST_OK(mln_runtime_dispose(runtime, nullptr));
  sync.release(SyncPoint::GeoJsonDataPrepare);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_settle(&preparing));
  MLN_TEST_OK(mln_map_dispose(map, nullptr));
  TEST_ASSERT_TRUE(
    await([&] { return retired.expired(); }, "the runtime's retirement")
  );
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(replaced_data_that_a_slice_outlives_retires_on_its_worker);
  RUN_TEST(the_synchronous_tiling_override_slices_without_the_datas_worker);
  RUN_TEST(only_the_newest_data_waits_behind_a_preparation);
  RUN_TEST(replacing_the_sources_data_cancels_data_in_preparation);
  RUN_TEST(releasing_the_map_cancels_data_in_preparation);
  RUN_TEST(a_disposed_runtime_finishes_data_in_preparation);
}
