// Offscreen sessions that take turns rendering in a single-context EGL device.

#include "support/frames.h"
#include "support/test_support.h"

enum { extent = 64, pixel_bytes = extent * extent * 4 };

// Two styles that draw with different programs, so a session that rendered
// with the program another session left bound would draw the wrong layer.
static const char circle_style_json[] =
  "{\"version\":8,\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]},\"properties\":{}}}},"
  "\"layers\":[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#203040\"}},"
  "{\"id\":\"dot\",\"type\":\"circle\",\"source\":\"points\","
  "\"paint\":{\"circle-radius\":12,\"circle-color\":\"#ff8000\"}}]}";
static const char fill_style_json[] =
  "{\"version\":8,\"sources\":{\"square\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Polygon\","
  "\"coordinates\":[[[-10,-10],[10,-10],[10,10],[-10,10],[-10,-10]]]},"
  "\"properties\":{}}}},"
  "\"layers\":[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#ffffff\"}},"
  "{\"id\":\"square\",\"type\":\"fill\",\"source\":\"square\","
  "\"paint\":{\"fill-color\":\"#0080ff\"}}]}";

static mln_gpu_device create_device(bool single_context) {
  mln_gpu_device_options options = {.size = sizeof(mln_gpu_device_options)};
  options.fields = MLN_GPU_DEVICE_OPTION_BACKEND |
                   MLN_GPU_DEVICE_OPTION_OPEN_GL_SINGLE_CONTEXT;
  options.backend = MLN_RENDER_BACKEND_OPEN_GL;
  options.open_gl_single_context = single_context;
  mln_gpu_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_gpu_device_create(&options, &device, MLN_TEST_DIAGNOSTIC));
  return device;
}

static mln_test_render_fixture attach_offscreen_session(
  mln_map map, mln_gpu_device device
) {
  mln_test_render_fixture fixture = {.driver = MLN_RENDER_DRIVER_CORE_WORKER};
  mln_render_target descriptor = {
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_OFFSCREEN,
  };
  descriptor.data.offscreen.logical_size =
    (mln_logical_size){.width = extent, .height = extent};
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.driver = fixture.driver;
  mln_test_completion attach = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_attach(
    map, &descriptor, device, &options, &fixture.session, &attach.descriptor,
    MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &attach));
  mln_test_completion_destroy(&attach);
  return fixture;
}

static void detach_offscreen_session(mln_test_render_fixture* fixture) {
  mln_test_completion detach = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_detach(
    fixture->session, &detach.descriptor, MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &detach));
  mln_test_completion_destroy(&detach);
  MLN_TEST_OK(mln_test_render_session_release(fixture->session));
  *fixture = (mln_test_render_fixture){0};
}

static void read_pixels(
  const mln_test_render_fixture* fixture, uint8_t* pixels
) {
  mln_texture_image_info info = {0};
  MLN_TEST_OK(mln_test_render_read_back(fixture, &info, pixels, pixel_bytes));
  TEST_ASSERT_EQUAL_UINT32(extent, info.width);
  TEST_ASSERT_EQUAL_UINT32(extent, info.height);
  TEST_ASSERT_EQUAL_size_t(pixel_bytes, info.byte_length);
}

static void render_style(
  mln_runtime runtime, mln_map map, const mln_test_render_fixture* fixture,
  mln_buffer_view style, uint8_t* pixels
) {
  MLN_TEST_OK(mln_test_map_set_style_json(map, style));
  mln_test_render_until_idle(runtime, map, fixture);
  read_pixels(fixture, pixels);
}

static void render_again(
  const mln_test_render_fixture* fixture, uint64_t token, uint8_t* pixels
) {
  mln_test_render_demand_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT64(token, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_render_frame_batch_release(batch);
  read_pixels(fixture, pixels);
}

static void render_alone(
  mln_runtime runtime, mln_buffer_view style, uint8_t* pixels
) {
  mln_gpu_device device = create_device(false);
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = attach_offscreen_session(map, device);
  render_style(runtime, map, &fixture, style, pixels);
  detach_offscreen_session(&fixture);
  mln_test_destroy_map(map);
  mln_gpu_device_release(device);
}

static void sessions_that_take_turns_render_as_alone(void) {
  static uint8_t expected[2][pixel_bytes];
  static uint8_t actual[pixel_bytes];
  const mln_buffer_view styles[2] = {
    MLN_BUFFER_LITERAL(circle_style_json),
    MLN_BUFFER_LITERAL(fill_style_json),
  };
  mln_runtime runtime = mln_test_create_runtime();
  for (size_t index = 0; index < 2; index += 1) {
    render_alone(runtime, styles[index], expected[index]);
  }
  TEST_ASSERT_NOT_EQUAL_INT(0, memcmp(expected[0], expected[1], pixel_bytes));

  mln_gpu_device device = create_device(true);
  mln_map maps[2];
  mln_test_render_fixture fixtures[2];
  for (size_t index = 0; index < 2; index += 1) {
    maps[index] = mln_test_create_map(runtime);
    fixtures[index] = attach_offscreen_session(maps[index], device);
  }

  for (size_t index = 0; index < 2; index += 1) {
    render_style(runtime, maps[index], &fixtures[index], styles[index], actual);
    TEST_ASSERT_EQUAL_MEMORY(expected[index], actual, pixel_bytes);
  }
  uint64_t token = 1;
  for (size_t turn = 0; turn < 3; turn += 1) {
    for (size_t index = 0; index < 2; index += 1) {
      render_again(&fixtures[index], token++, actual);
      TEST_ASSERT_EQUAL_MEMORY(expected[index], actual, pixel_bytes);
    }
  }

  for (size_t index = 0; index < 2; index += 1) {
    detach_offscreen_session(&fixtures[index]);
    mln_test_destroy_map(maps[index]);
  }
  mln_gpu_device_release(device);
  mln_test_destroy_runtime(runtime);
}

static void sessions_detach_while_others_render(void) {
  static uint8_t circle[pixel_bytes];
  static uint8_t fill[pixel_bytes];
  static uint8_t actual[pixel_bytes];
  mln_runtime runtime = mln_test_create_runtime();
  mln_gpu_device device = create_device(true);
  mln_map first_map = mln_test_create_map(runtime);
  mln_map second_map = mln_test_create_map(runtime);
  mln_test_render_fixture first = attach_offscreen_session(first_map, device);
  mln_test_render_fixture second = attach_offscreen_session(second_map, device);
  render_style(
    runtime, first_map, &first, MLN_BUFFER_LITERAL(circle_style_json), circle
  );
  render_style(
    runtime, second_map, &second, MLN_BUFFER_LITERAL(fill_style_json), fill
  );

  detach_offscreen_session(&first);
  mln_test_destroy_map(first_map);
  render_again(&second, 31, actual);
  TEST_ASSERT_EQUAL_MEMORY(fill, actual, pixel_bytes);

  mln_map third_map = mln_test_create_map(runtime);
  mln_test_render_fixture third = attach_offscreen_session(third_map, device);
  mln_gpu_device_release(device);
  render_style(
    runtime, third_map, &third, MLN_BUFFER_LITERAL(circle_style_json), actual
  );
  TEST_ASSERT_EQUAL_MEMORY(circle, actual, pixel_bytes);
  render_again(&second, 32, actual);
  TEST_ASSERT_EQUAL_MEMORY(fill, actual, pixel_bytes);

  detach_offscreen_session(&second);
  detach_offscreen_session(&third);
  mln_test_destroy_map(third_map);
  mln_test_destroy_map(second_map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(sessions_that_take_turns_render_as_alone);
  RUN_TEST(sessions_detach_while_others_render);
}
