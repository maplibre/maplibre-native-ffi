// Queue deliveries own the nested storage of completion values.

#include <cstddef>
#include <cstdint>
#include <new>

#include "delivery/delivery_queue.hpp"
#include "maplibre_native_c.h"
#include "support/env.h"
#include "support/harness.h"
#include "support/status.h"
#include "unity.h"

namespace {

// One copy and the storage it owns.
struct Copy {
  void* storage = nullptr;
  const void* value = nullptr;

  Copy() = default;
  Copy(const Copy&) = delete;
  auto operator=(const Copy&) -> Copy& = delete;
  ~Copy() { ::operator delete(storage); }
};

void copy_values(
  const mln::delivery::CompletionCapture& capture, const void* values,
  std::size_t count, Copy& out
) {
  auto measure = mln::delivery::Arena<false>{nullptr, 0};
  capture.measure(measure, values, count);
  out.storage = ::operator new(measure.offset == 0 ? 1 : measure.offset);
  auto write = mln::delivery::Arena<true>{
    static_cast<std::byte*>(out.storage), 0, measure.offset
  };
  out.value = capture.write(write, values, count);
}

template <class T>
void copy_one(const T& value, Copy& out) {
  copy_values(mln::delivery::completion_capture<T>, &value, 1, out);
}

void a_copy_owns_arrays_and_their_nested_buffers() {
  char text[] = "tile";
  const auto url = mln_buffer_view{text, sizeof(text) - 1};
  auto source = mln_style_source_info{};
  source.id = url;
  source.fields = MLN_STYLE_SOURCE_INFO_TILEJSON;
  source.tilejson.tile_urls = &url;
  source.tilejson.tile_url_count = 1;
  const mln_style_source_info sources[] = {source, source};
  auto copy = Copy{};
  copy_values(
    mln::delivery::completion_capture<mln_style_source_info>, sources, 2, copy
  );
  const auto* copied = static_cast<const mln_style_source_info*>(copy.value);
  text[0] = 'X';
  for (std::size_t index = 0; index < 2; ++index) {
    TEST_ASSERT_EQUAL_MEMORY("tile", copied[index].id.data, 4);
    TEST_ASSERT_EQUAL_size_t(1, copied[index].tilejson.tile_url_count);
    TEST_ASSERT_TRUE(copied[index].tilejson.tile_urls != &url);
    TEST_ASSERT_EQUAL_MEMORY(
      "tile", copied[index].tilejson.tile_urls[0].data, 4
    );
  }
}

void a_copy_preserves_the_selected_union_variant() {
  char geometry[] = "{}";
  auto region = mln_offline_region_info{};
  region.definition.type = MLN_OFFLINE_REGION_DEFINITION_GEOMETRY;
  region.definition.data.geometry.geometry = {geometry, 2};
  region.definition.data.geometry.pixel_ratio = 2.0F;
  auto copy = Copy{};
  copy_one(region, copy);
  geometry[0] = 'X';
  const auto* copied = static_cast<const mln_offline_region_info*>(copy.value);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_OFFLINE_REGION_DEFINITION_GEOMETRY, copied->definition.type
  );
  TEST_ASSERT_EQUAL_FLOAT(2.0F, copied->definition.data.geometry.pixel_ratio);
  TEST_ASSERT_EQUAL_MEMORY(
    "{}", copied->definition.data.geometry.geometry.data, 2
  );
}

// A field its presence bit marks absent reaches the host cleared, whatever
// the source left in it, so no stale value looks present.
void a_copy_clears_the_fields_its_presence_bits_mark_absent() {
  static const char text[] = "left behind";
  const auto stale = mln_buffer_view{text, sizeof(text) - 1};

  const auto camera = mln_camera_options{
    .size = sizeof(mln_camera_options),
    .fields = 0,
    .center = {.latitude = 1.0, .longitude = 2.0},
    .center_altitude = 3.0,
    .padding = {1.0, 1.0, 1.0, 1.0},
    .anchor = {1.0, 1.0},
    .zoom = 4.0,
    .bearing = 5.0,
    .pitch = 6.0,
    .roll = 7.0,
    .field_of_view = 8.0,
  };
  auto camera_copy = Copy{};
  copy_one(camera, camera_copy);
  const auto* copied_camera =
    static_cast<const mln_camera_options*>(camera_copy.value);
  TEST_ASSERT_EQUAL_UINT32(0, copied_camera->fields);
  TEST_ASSERT_TRUE(mln_test_all_zero(
    &copied_camera->center,
    sizeof(mln_camera_options) - offsetof(mln_camera_options, center)
  ));

  auto feature = mln_queried_feature{};
  feature.feature = mln_buffer_view{"{}", 2};
  feature.source_id = stale;
  feature.source_layer_id = stale;
  feature.state = stale;
  auto feature_copy = Copy{};
  copy_one(feature, feature_copy);
  const auto* copied_feature =
    static_cast<const mln_queried_feature*>(feature_copy.value);
  TEST_ASSERT_EQUAL_size_t(2, copied_feature->feature.size);
  TEST_ASSERT_EQUAL_size_t(0, copied_feature->source_id.size);
  TEST_ASSERT_EQUAL_size_t(0, copied_feature->source_layer_id.size);
  TEST_ASSERT_EQUAL_size_t(0, copied_feature->state.size);

  // A clear TILEJSON bit leaves the embedded record unread, so its tile URL
  // pointer and count are never followed.
  auto source = mln_style_source_info{};
  source.attribution = stale;
  source.url = stale;
  source.tilejson.tile_urls = &stale;
  source.tilejson.tile_url_count = SIZE_MAX;
  source.tilejson.min_zoom = 1.0;
  source.tile_size = 512;
  auto source_copy = Copy{};
  copy_one(source, source_copy);
  const auto* copied_source =
    static_cast<const mln_style_source_info*>(source_copy.value);
  TEST_ASSERT_TRUE(mln_test_all_zero(
    &copied_source->attribution, sizeof(copied_source->attribution)
  ));
  TEST_ASSERT_TRUE(mln_test_tile_info_is_zero(&copied_source->tilejson));
  TEST_ASSERT_EQUAL_UINT32(0, copied_source->tile_size);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(a_copy_owns_arrays_and_their_nested_buffers);
  RUN_TEST(a_copy_preserves_the_selected_union_variant);
  RUN_TEST(a_copy_clears_the_fields_its_presence_bits_mark_absent);
}
