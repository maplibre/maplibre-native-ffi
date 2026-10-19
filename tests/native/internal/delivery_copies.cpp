// The copies that queue deliveries make of completion values. The generated
// table synthesizes two values of each completion value type, with every
// pointer set and every array of two elements, and each copy must hold equal
// content in storage of its own.

#include <cstddef>
#include <cstdint>
#include <new>

#include "delivery/delivery_queue.hpp"
#include "internal/delivery_copy_cases_generated.inc"
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

void every_value_type_copies_into_storage_of_its_own() {
  for (const auto& entry : mln_delivery_copy_cases) {
    auto count = std::size_t{};
    const auto* source = entry.value(&count);
    auto copy = Copy{};
    copy_values(*entry.capture, source, count, copy);
    TEST_ASSERT_TRUE_MESSAGE(copy.value != source, entry.type);
    TEST_ASSERT_TRUE_MESSAGE(
      entry.matches(copy.value, source, count), entry.type
    );
    // The synthesized handles were never issued, so nothing disposes them.
  }
}

template <class T>
void copy_one(const T& value, Copy& out) {
  copy_values(mln::delivery::completion_capture<T>, &value, 1, out);
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

// A source span that a count claims but no pointer backs cannot be copied.
void a_span_without_storage_cannot_be_copied() {
  auto measure = mln::delivery::Arena<false>{nullptr, 0};
  auto threw = false;
  try {
    mln::delivery::completion_capture<mln_lat_lng>.measure(measure, nullptr, 2);
  } catch (const std::exception&) {
    threw = true;
  }
  TEST_ASSERT_TRUE(threw);
}

}  // namespace

MLN_TEST_GROUP {
  RUN_TEST(every_value_type_copies_into_storage_of_its_own);
  RUN_TEST(a_copy_clears_the_fields_its_presence_bits_mark_absent);
  RUN_TEST(a_span_without_storage_cannot_be_copied);
}
