// Adapter completion copies. The generated table synthesizes one result for
// each copy kind, with every pointer set and every array of two elements. Each
// result passes through an adapter completion, and the delivered record must
// hold equal content in storage of its own.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "adapter_copy_cases_generated.inc"
#include "maplibre_native_c/callback_adapter.h"
#include "support/adapter.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

// Passes one result through an adapter completion and returns the record it
// delivered, or null for the failure channel.
static mln_adapter_completion_record* copy_result(
  uint32_t copy_kind, size_t element_size, const mln_completion_result* result,
  mln_test_adapter_delivery* delivery
) {
  mln_completion completion =
    mln_test_adapter_completion(copy_kind, element_size, delivery);
  completion.callback(completion.user_data, result);
  completion.release_user_data(completion.user_data);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&delivery->deliveries));
  return atomic_load(&delivery->record);
}

static void every_copy_kind_copies_its_result_into_storage_of_its_own(void) {
  static const char diagnostic[] = "copied diagnostic";
  for (size_t index = 0; index < sizeof(mln_adapter_copy_cases) /
                                   sizeof(mln_adapter_copy_cases[0]);
       ++index) {
    const mln_adapter_copy_case* entry = &mln_adapter_copy_cases[index];
    size_t count = 0;
    const void* source = entry->value(&count);
    const mln_completion_result result = {
      .size = sizeof(mln_completion_result),
      .status = MLN_STATUS_OK,
      .generation = 5,
      .diagnostic = MLN_BUFFER_LITERAL(diagnostic),
      .value = source,
      .value_count = count,
    };
    mln_test_adapter_delivery delivery = {0};
    mln_adapter_completion_record* record =
      copy_result(entry->kind, entry->element_size, &result, &delivery);

    TEST_ASSERT_NOT_NULL_MESSAGE(record, entry->type);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK, record->result.status, entry->type
    );
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(5, record->result.generation, entry->type);
    TEST_ASSERT_TRUE_MESSAGE(
      mln_adapter_copy_case_view(record->result.diagnostic, result.diagnostic),
      entry->type
    );
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      count, record->result.value_count, entry->type
    );
    TEST_ASSERT_TRUE_MESSAGE(record->result.value != source, entry->type);
    TEST_ASSERT_TRUE_MESSAGE(
      entry->matches(record->result.value, source, count), entry->type
    );
    // The synthesized handles were never issued, so the record must not
    // dispose them.
    mln_adapter_completion_record_adopt(record);
    mln_adapter_completion_record_destroy(record);
  }
}

// A flat copy takes element_size bytes per value, and cannot copy values of
// unknown size.
static void flat_copies_take_element_size_bytes_per_value(void) {
  typedef struct triple {
    uint32_t first;
    uint32_t second;
    uint32_t third;
  } triple;
  const triple values[] = {{1, 2, 3}, {4, 5, 6}};
  const mln_completion_result result = {
    .size = sizeof(mln_completion_result),
    .status = MLN_STATUS_OK,
    .value = values,
    .value_count = 2,
  };

  mln_test_adapter_delivery delivery = {0};
  mln_adapter_completion_record* record = copy_result(
    MLN_ADAPTER_COMPLETION_COPY_FLAT, sizeof(triple), &result, &delivery
  );
  TEST_ASSERT_NOT_NULL(record);
  TEST_ASSERT_EQUAL_size_t(2, record->result.value_count);
  TEST_ASSERT_TRUE(record->result.value != (const void*)values);
  TEST_ASSERT_EQUAL_MEMORY(values, record->result.value, sizeof(values));
  mln_adapter_completion_record_destroy(record);

  mln_test_adapter_delivery unsized = {0};
  TEST_ASSERT_NULL(
    copy_result(MLN_ADAPTER_COMPLETION_COPY_FLAT, 0, &result, &unsized)
  );
}

// A failed result copies its diagnostic and carries no value, whatever value
// the borrowed result pointed to.
static void a_failed_result_copies_its_diagnostic_and_no_value(void) {
  static const char diagnostic[] = "native failure";
  const mln_map map = 1;
  const mln_completion_result result = {
    .size = sizeof(mln_completion_result),
    .status = MLN_STATUS_NATIVE_ERROR,
    .diagnostic = MLN_BUFFER_LITERAL(diagnostic),
    .value = &map,
    .value_count = 1,
  };
  mln_test_adapter_delivery delivery = {0};
  mln_adapter_completion_record* record = copy_result(
    MLN_ADAPTER_COMPLETION_COPY_MAP, sizeof(mln_map), &result, &delivery
  );
  TEST_ASSERT_NOT_NULL(record);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_NATIVE_ERROR, record->result.status);
  TEST_ASSERT_TRUE(
    mln_adapter_copy_case_view(record->result.diagnostic, result.diagnostic)
  );
  TEST_ASSERT_NULL(record->result.value);
  TEST_ASSERT_EQUAL_size_t(0, record->result.value_count);
  mln_adapter_completion_record_destroy(record);
}

// Copies one value of `size` bytes and returns the record, whose value the
// caller reads before destroying it.
static mln_adapter_completion_record* copy_one(
  uint32_t copy_kind, const void* value, size_t size,
  mln_test_adapter_delivery* delivery
) {
  const mln_completion_result result = {
    .size = sizeof(mln_completion_result),
    .status = MLN_STATUS_OK,
    .value = value,
    .value_count = 1,
  };
  mln_adapter_completion_record* record =
    copy_result(copy_kind, size, &result, delivery);
  TEST_ASSERT_NOT_NULL(record);
  return record;
}

static bool all_zero(const void* bytes, size_t size) {
  const unsigned char* byte = bytes;
  for (size_t index = 0; index < size; index += 1) {
    if (byte[index] != 0) return false;
  }
  return true;
}

// A field its presence bit marks absent reaches the host cleared, whatever
// the borrowed source left in it, so no stale value looks present.
static void a_copy_clears_the_fields_its_presence_bits_mark_absent(void) {
  static const char text[] = "left behind";
  const mln_buffer_view stale = MLN_BUFFER_LITERAL(text);

  const mln_camera_options camera = {
    .size = sizeof(mln_camera_options),
    .latitude = 1.0,
    .longitude = 2.0,
    .center_altitude = 3.0,
    .padding = {1.0, 1.0, 1.0, 1.0},
    .anchor = {1.0, 1.0},
    .zoom = 4.0,
    .bearing = 5.0,
    .pitch = 6.0,
    .roll = 7.0,
    .field_of_view = 8.0,
  };
  mln_test_adapter_delivery camera_delivery = {0};
  mln_adapter_completion_record* record = copy_one(
    MLN_ADAPTER_COMPLETION_COPY_CAMERA_OPTIONS, &camera, sizeof(camera),
    &camera_delivery
  );
  const mln_camera_options* copied_camera = record->result.value;
  TEST_ASSERT_EQUAL_UINT32(0, copied_camera->fields);
  TEST_ASSERT_TRUE(all_zero(
    &copied_camera->latitude,
    sizeof(mln_camera_options) - offsetof(mln_camera_options, latitude)
  ));
  mln_adapter_completion_record_destroy(record);

  const mln_style_transition_options transition = {
    .size = sizeof(mln_style_transition_options),
    .duration_ms = 5.0,
    .delay_ms = 6.0,
  };
  mln_test_adapter_delivery transition_delivery = {0};
  record = copy_one(
    MLN_ADAPTER_COMPLETION_COPY_STYLE_TRANSITION_OPTIONS, &transition,
    sizeof(transition), &transition_delivery
  );
  const mln_style_transition_options* copied_transition = record->result.value;
  TEST_ASSERT_EQUAL_DOUBLE(0.0, copied_transition->duration_ms);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, copied_transition->delay_ms);
  mln_adapter_completion_record_destroy(record);

  const mln_queried_feature feature = {
    .size = sizeof(mln_queried_feature),
    .feature = MLN_BUFFER_LITERAL("{}"),
    .source_id = stale,
    .source_layer_id = stale,
    .state = stale,
  };
  mln_test_adapter_delivery feature_delivery = {0};
  record = copy_one(
    MLN_ADAPTER_COMPLETION_COPY_QUERIED_FEATURE, &feature, sizeof(feature),
    &feature_delivery
  );
  const mln_queried_feature* copied_feature = record->result.value;
  TEST_ASSERT_EQUAL_size_t(2, copied_feature->feature.size);
  TEST_ASSERT_EQUAL_size_t(0, copied_feature->source_id.size);
  TEST_ASSERT_EQUAL_size_t(0, copied_feature->source_layer_id.size);
  TEST_ASSERT_EQUAL_size_t(0, copied_feature->state.size);
  mln_adapter_completion_record_destroy(record);

  mln_style_image_result image = {.size = sizeof(mln_style_image_result)};
  image.info = mln_style_image_info_default();
  image.info.content = (mln_image_content){1.0f, 1.0f, 2.0f, 2.0f};
  image.info.text_fit_width = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  image.info.text_fit_height = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  mln_test_adapter_delivery image_delivery = {0};
  record = copy_one(
    MLN_ADAPTER_COMPLETION_COPY_STYLE_IMAGE_RESULT, &image, sizeof(image),
    &image_delivery
  );
  const mln_style_image_result* copied_image = record->result.value;
  TEST_ASSERT_TRUE(
    all_zero(&copied_image->info.content, sizeof(copied_image->info.content))
  );
  TEST_ASSERT_EQUAL_UINT32(0, copied_image->info.text_fit_width);
  TEST_ASSERT_EQUAL_UINT32(0, copied_image->info.text_fit_height);
  mln_adapter_completion_record_destroy(record);

  const mln_buffer_view tile_urls[] = {stale};
  const mln_style_source_result source = {
    .size = sizeof(mln_style_source_result),
    .info =
      {
        .size = sizeof(mln_style_source_info),
        .attribution_size = 3,
        .url_size = 3,
        .tile_count = 1,
        .min_zoom = 1.0,
        .max_zoom = 2.0,
        .scheme = 1,
        .bounds = {{1.0, 1.0}, {2.0, 2.0}},
        .tile_size = 512,
        .vector_encoding = 1,
        .raster_encoding = 1,
      },
    .attribution = stale,
    .url = stale,
    .tile_urls = tile_urls,
    .tile_url_count = 1,
  };
  mln_test_adapter_delivery source_delivery = {0};
  record = copy_one(
    MLN_ADAPTER_COMPLETION_COPY_STYLE_SOURCE_RESULT, &source, sizeof(source),
    &source_delivery
  );
  const mln_style_source_result* copied_source = record->result.value;
  TEST_ASSERT_EQUAL_size_t(0, copied_source->info.attribution_size);
  TEST_ASSERT_EQUAL_size_t(0, copied_source->info.url_size);
  TEST_ASSERT_EQUAL_size_t(0, copied_source->info.tile_count);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, copied_source->info.max_zoom);
  TEST_ASSERT_TRUE(
    all_zero(&copied_source->info.bounds, sizeof(copied_source->info.bounds))
  );
  TEST_ASSERT_EQUAL_UINT32(0, copied_source->info.tile_size);
  TEST_ASSERT_EQUAL_UINT32(0, copied_source->info.vector_encoding);
  TEST_ASSERT_EQUAL_UINT32(0, copied_source->info.raster_encoding);
  TEST_ASSERT_EQUAL_size_t(0, copied_source->attribution.size);
  TEST_ASSERT_EQUAL_size_t(0, copied_source->url.size);
  TEST_ASSERT_NULL(copied_source->tile_urls);
  mln_adapter_completion_record_destroy(record);
}

MLN_TEST_GROUP {
  RUN_TEST(every_copy_kind_copies_its_result_into_storage_of_its_own);
  RUN_TEST(a_copy_clears_the_fields_its_presence_bits_mark_absent);
  RUN_TEST(flat_copies_take_element_size_bytes_per_value);
  RUN_TEST(a_failed_result_copies_its_diagnostic_and_no_value);
}
