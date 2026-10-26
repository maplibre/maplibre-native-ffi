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
  source.fields = MLN_STYLE_SOURCE_INFO_TILE_JSON;
  source.tile_json.tile_urls = &url;
  source.tile_json.tile_url_count = 1;
  const mln_style_source_info sources[] = {source, source};
  auto copy = Copy{};
  copy_values(
    mln::delivery::completion_capture<mln_style_source_info>, sources, 2, copy
  );
  const auto* copied = static_cast<const mln_style_source_info*>(copy.value);
  text[0] = 'X';
  for (std::size_t index = 0; index < 2; ++index) {
    TEST_ASSERT_EQUAL_MEMORY("tile", copied[index].id.data, 4);
    TEST_ASSERT_EQUAL_size_t(1, copied[index].tile_json.tile_url_count);
    TEST_ASSERT_TRUE(copied[index].tile_json.tile_urls != &url);
    TEST_ASSERT_EQUAL_MEMORY(
      "tile", copied[index].tile_json.tile_urls[0].data, 4
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

  // A clear TILE_JSON bit leaves the embedded record unread, so its tile URL
  // pointer and count are never followed.
  auto source = mln_style_source_info{};
  source.tile_json.tile_urls = &stale;
  source.tile_json.tile_url_count = SIZE_MAX;
  source.tile_json.min_zoom = 1.0;
  source.tile_size = 512;
  auto source_copy = Copy{};
  copy_one(source, source_copy);
  const auto* copied_source =
    static_cast<const mln_style_source_info*>(source_copy.value);
  TEST_ASSERT_TRUE(mln_test_tile_info_is_zero(&copied_source->tile_json));
  TEST_ASSERT_EQUAL_UINT32(0, copied_source->tile_size);
}

// Text that native lends is NUL-terminated, and a copy cannot tell text from
// bytes, so every copied view gets a terminator, even an empty one whose
// source had no data.
void a_copy_terminates_every_view() {
  // Each view stops short of its source's last byte, so a copy that kept the
  // source's bytes and nothing more would end in a letter.
  static const char text[] = "lent textX";
  const auto lent = mln_buffer_view{text, sizeof(text) - 2};
  const auto empty = mln_buffer_view{nullptr, 0};

  auto buffer_copy = Copy{};
  copy_one(lent, buffer_copy);
  const auto* copied_buffer =
    static_cast<const mln_buffer_view*>(buffer_copy.value);
  TEST_ASSERT_EQUAL_size_t(lent.size, copied_buffer->size);
  TEST_ASSERT_TRUE(mln_test_lent_text_is_terminated(*copied_buffer));

  const auto feature = mln_rendered_feature{
    .feature = lent,
    .layer_id = lent,
    .source_id = empty,
    .source_layer_id = lent,
    .state = empty,
  };
  auto feature_copy = Copy{};
  copy_one(feature, feature_copy);
  const auto* copied_feature =
    static_cast<const mln_rendered_feature*>(feature_copy.value);
  for (const auto& view :
       {copied_feature->feature, copied_feature->layer_id,
        copied_feature->source_id, copied_feature->source_layer_id,
        copied_feature->state}) {
    TEST_ASSERT_TRUE(mln_test_lent_text_is_terminated(view));
  }
  TEST_ASSERT_EQUAL_size_t(0, copied_feature->source_id.size);

  const auto source_feature =
    mln_source_feature{.feature = lent, .source_layer_id = empty};
  auto source_feature_copy = Copy{};
  copy_one(source_feature, source_feature_copy);
  const auto* copied_source_feature =
    static_cast<const mln_source_feature*>(source_feature_copy.value);
  TEST_ASSERT_TRUE(
    mln_test_lent_text_is_terminated(copied_source_feature->feature)
  );
  TEST_ASSERT_TRUE(
    mln_test_lent_text_is_terminated(copied_source_feature->source_layer_id)
  );

  auto source = mln_style_source_info{};
  source.id = lent;
  source.attribution = empty;
  source.url = lent;
  source.fields = MLN_STYLE_SOURCE_INFO_TILE_JSON;
  source.tile_json.tile_urls = &lent;
  source.tile_json.tile_url_count = 1;
  auto source_copy = Copy{};
  copy_one(source, source_copy);
  const auto* copied_source =
    static_cast<const mln_style_source_info*>(source_copy.value);
  for (const auto& view :
       {copied_source->id, copied_source->attribution, copied_source->url,
        copied_source->tile_json.tile_urls[0]}) {
    TEST_ASSERT_TRUE(mln_test_lent_text_is_terminated(view));
  }

  auto layer = mln_style_layer_info{};
  layer.id = lent;
  layer.type = lent;
  layer.source_id = empty;
  layer.source_layer = empty;
  auto layer_copy = Copy{};
  copy_one(layer, layer_copy);
  const auto* copied_layer =
    static_cast<const mln_style_layer_info*>(layer_copy.value);
  for (const auto& view :
       {copied_layer->id, copied_layer->type, copied_layer->source_id,
        copied_layer->source_layer}) {
    TEST_ASSERT_TRUE(mln_test_lent_text_is_terminated(view));
  }
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(a_copy_owns_arrays_and_their_nested_buffers);
  RUN_TEST(a_copy_preserves_the_selected_union_variant);
  RUN_TEST(a_copy_clears_the_fields_its_presence_bits_mark_absent);
  RUN_TEST(a_copy_terminates_every_view);
}
