// GeoJSON tiling: asynchronous slices on a data's sequenced worker, ordered
// against data replacement and the synchronous tiling override by parking the
// worker at its slice.

#include <cstdio>
#include <cstring>
#include <memory>

#include "geojson/geojson_source_data.hpp"
#include "internal/support/checks.hpp"
#include "internal/support/sync_points.hpp"
#include "maplibre_native_c.h"
#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

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

auto prepare(const char* json, const mln_geojson_source_options* options)
  -> mln_geojson_source_data {
  auto data = mln_geojson_source_data{MLN_HANDLE_NULL};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     mln_test_view_of(json), options, &data, MLN_TEST_DIAGNOSTIC
                   )
  );
  return data;
}

// The index behind a prepared handle, which the handle, the sources it is
// installed on, and their tiles share.
auto index_of(mln_geojson_source_data data)
  -> std::weak_ptr<mln::style::GeoJSONData> {
  const auto object = mln::core::geojson_source_data_table().lease(data);
  TEST_ASSERT_NOT_NULL(object.get());
  return object->data;
}

// A map with a GeoJSON source named "points" holding `data`, drawn by a
// circle layer so that rendering requests its tiles.
void add_drawn_source(
  mln_runtime runtime, mln_map map, mln_geojson_source_data data
) {
  mln_test_load_style_and_wait(
    runtime, map, MLN_BUFFER_LITERAL(empty_style_json)
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(
      map, MLN_BUFFER_LITERAL("points"), data, &completion.descriptor, nullptr
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_style_layer_json(
      map,
      MLN_BUFFER_LITERAL(R"({"id":"dots","type":"circle","source":"points"})"),
      MLN_BUFFER_LITERAL(""), &completion.descriptor, nullptr
    )
  );
}

void set_data(mln_map map, mln_geojson_source_data data) {
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_geojson_source_data(
      map, MLN_BUFFER_LITERAL("points"), data, &completion.descriptor, nullptr
    )
  );
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
  auto first = prepare(POINT_COLLECTION("first"), nullptr);
  const auto first_index = index_of(first);
  add_drawn_source(runtime, map, first);

  auto fixture = mln_test_render_fixture{};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  sync.hold(SyncPoint::GeoJsonTileSlice);
  TEST_ASSERT_TRUE(render_until(
    fixture, [&] { return sync.hits(SyncPoint::GeoJsonTileSlice) >= 1; },
    "a slice of the first data"
  ));

  // The parked slice becomes the first data's last owner once the handle and
  // every tile have moved on. The second data's slices park on their own
  // worker meanwhile, which a frame does not wait for.
  const auto second = prepare(POINT_COLLECTION("second"), nullptr);
  set_data(map, second);
  mln_geojson_source_data_destroy(first);
  first = MLN_HANDLE_NULL;
  TEST_ASSERT_TRUE(render_until(
    fixture, [&] { return first_index.expired(); },
    "the first data's handle and tiles to let it go"
  ));

  sync.release(SyncPoint::GeoJsonTileSlice);
  TEST_ASSERT_TRUE(render_until(
    fixture, [&] { return holds_the_name(fixture, "second"); },
    "the second data's tiles"
  ));

  mln_geojson_source_data_destroy(second);
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
  const auto first = prepare(POINT_COLLECTION("first"), nullptr);
  add_drawn_source(runtime, map, first);
  mln_geojson_source_data_destroy(first);

  auto fixture = mln_test_render_fixture{};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  TEST_ASSERT_TRUE(render_until(
    fixture, [&] { return holds_the_name(fixture, "first"); },
    "the first data's tiles"
  ));

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_geojson_source_synchronous_tiling(
      map, MLN_BUFFER_LITERAL("points"), true, &completion.descriptor, nullptr
    )
  );
  sync.hold(SyncPoint::GeoJsonTileSlice);
  const auto second = prepare(POINT_COLLECTION("second"), nullptr);
  set_data(map, second);
  mln_geojson_source_data_destroy(second);
  TEST_ASSERT_TRUE(render_until(
    fixture, [&] { return holds_the_name(fixture, "second"); },
    "the second data's tiles while its worker is parked"
  ));

  sync.release(SyncPoint::GeoJsonTileSlice);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(replaced_data_that_a_slice_outlives_retires_on_its_worker);
  RUN_TEST(the_synchronous_tiling_override_slices_without_the_datas_worker);
}
