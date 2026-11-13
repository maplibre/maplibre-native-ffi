// Where a frame drew the map, which its result reports, and what a device and
// an attach say about keeping a frame's depth.

#include <math.h>

#include "support/frames.h"
#include "support/style.h"
#include "support/test_support.h"

enum { side = 64 };

// One red circle on black, in a source tiled to its highest zoom so that tile
// quantization stays far below a pixel, and tiled inline so that every frame
// draws it.
static const char circle_style[] =
  "{\"version\":8,\"sources\":{\"dot\":{\"type\":\"geojson\","
  "\"maxzoom\":24,\"data\":{\"type\":\"Point\",\"coordinates\":[20,10]}}},"
  "\"layers\":[{\"id\":\"base\",\"type\":\"background\",\"paint\":"
  "{\"background-color\":\"#000000\"}},{\"id\":\"dot\",\"type\":\"circle\","
  "\"source\":\"dot\",\"paint\":{\"circle-color\":\"#ff0000\","
  "\"circle-radius\":3}}],\"transition\":{\"duration\":0,\"delay\":0}}";

static const mln_lat_lng dot = {.latitude = 10.0, .longitude = 20.0};

// The centroid of the red pixels of the fixture's latest frame, in pixels from
// the top-left corner, measured at pixel centers.
static mln_screen_point drawn_dot(const mln_test_render_fixture* fixture) {
  static uint8_t pixels[side * side * 4];
  mln_texture_image_info info = {0};
  MLN_TEST_OK(
    mln_test_render_read_back(fixture, &info, pixels, sizeof(pixels))
  );
  double x = 0.0;
  double y = 0.0;
  size_t count = 0;
  for (uint32_t row = 0; row < info.height; row += 1) {
    for (uint32_t column = 0; column < info.width; column += 1) {
      const uint8_t* pixel = pixels + (size_t)row * info.stride + column * 4U;
      if (pixel[0] >= 128 && pixel[1] < 64 && pixel[2] < 64) {
        x += column + 0.5;
        y += row + 0.5;
        count += 1;
      }
    }
  }
  TEST_ASSERT_GREATER_THAN_size_t(0, count);
  return (mln_screen_point){.x = x / (double)count, .y = y / (double)count};
}

// `coordinate` through the projection's matrix: its pixel from the top-left
// corner of a frame of `side` pixels.
static mln_screen_point projected(
  const mln_frame_projection* projection, mln_lat_lng coordinate
) {
  const double pi = 3.14159265358979323846;
  const double latitude = coordinate.latitude * pi / 180.0;
  const double world[4] = {
    (coordinate.longitude + 180.0) / 360.0,
    (1.0 - log(tan(pi / 4.0 + latitude / 2.0)) / pi) / 2.0, 0.0, 1.0
  };
  const mln_mat4* matrix = &projection->matrix;
  const mln_vec4* columns[4] = {&matrix->x, &matrix->y, &matrix->z, &matrix->w};
  double clip[4] = {0};
  for (int column = 0; column < 4; column += 1) {
    clip[0] += columns[column]->x * world[column];
    clip[1] += columns[column]->y * world[column];
    clip[2] += columns[column]->z * world[column];
    clip[3] += columns[column]->w * world[column];
  }
  return (mln_screen_point){
    .x = (clip[0] / clip[3] + 1.0) / 2.0 * side,
    .y = (1.0 - clip[1] / clip[3]) / 2.0 * side,
  };
}

static void a_frame_projection_places_points_where_the_frame_drew_them(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, MLN_BUFFER_LITERAL(circle_style));
  MLN_TEST_AWAIT_OK(mln_map_set_geo_json_source_synchronous_tiling(
    map, MLN_BUFFER_LITERAL("dot"), true, &completion.descriptor, NULL
  ));
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                         MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH |
                         MLN_CAMERA_OPTION_ROLL | MLN_CAMERA_OPTION_PADDING;
  update.camera.center = (mln_lat_lng){
    .latitude = dot.latitude + 0.01, .longitude = dot.longitude - 0.015
  };
  update.camera.zoom = 9.0;
  update.camera.bearing = 30.0;
  update.camera.pitch = 40.0;
  update.camera.roll = 12.0;
  update.camera.padding = (mln_edge_insets){.top = 14.0, .left = 10.0};
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  mln_test_render_until_idle(runtime, map, &fixture);

  mln_test_render_demand_forced(&fixture, 1);
  mln_render_frame_batch batch = mln_test_render_wait_for_results(&fixture);
  const mln_render_frame_result frame = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, frame.disposition);
  const mln_frame_projection* projection = &frame.frame_projection;
  // A session without a device renders with the platform's own backend,
  // whatever the build tests, so the projection names its convention.
  TEST_ASSERT_TRUE(
    projection->clip_depth_range == MLN_CLIP_DEPTH_RANGE_ZERO_TO_ONE ||
    projection->clip_depth_range == MLN_CLIP_DEPTH_RANGE_NEGATIVE_ONE_TO_ONE
  );
  TEST_ASSERT_TRUE(projection->far_depth > 0.9 && projection->far_depth < 1.0);
  TEST_ASSERT_TRUE(projection->near_distance > 0.0);
  TEST_ASSERT_TRUE(projection->near_distance < projection->far_distance);
  TEST_ASSERT_FALSE(projection->globe);
  const mln_screen_point drawn = drawn_dot(&fixture);
  const mln_screen_point point = projected(projection, dot);
  TEST_ASSERT_DOUBLE_WITHIN(1.0, drawn.x, point.x);
  TEST_ASSERT_DOUBLE_WITHIN(1.0, drawn.y, point.y);

  // A frame that draws nothing has no projection. The forced frame may have
  // left an update to draw, such as a placement's, so demand until one doesn't
  // render.
  const mln_test_deadline deadline = mln_test_deadline_default();
  mln_render_frame_result unchanged = {0};
  do {
    TEST_ASSERT_FALSE_MESSAGE(
      mln_test_deadline_passed(deadline), "the map kept publishing updates"
    );
    mln_frame_demand demand = {.size = sizeof(mln_frame_demand)};
    MLN_TEST_OK(
      mln_render_session_demand_frame(fixture.session, &demand, NULL)
    );
    batch = mln_test_render_wait_for_results(&fixture);
    unchanged = mln_test_render_batch_result(batch, 0);
    mln_render_frame_batch_release(batch);
  } while (unchanged.disposition == MLN_RENDER_RESULT_RENDERED);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_NO_UPDATE, unchanged.disposition);
  TEST_ASSERT_EQUAL_UINT32(0, unchanged.frame_projection.clip_depth_range);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, unchanged.frame_projection.matrix.w.w);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#if !defined(MLN_FFI_TEST_BACKEND_WEBGPU)
static void a_device_reports_the_depth_that_its_rings_keep(void) {
  mln_gpu_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_gpu_device_create(NULL, &device, MLN_TEST_DIAGNOSTIC));
  mln_gpu_device_capabilities capabilities = {.size = sizeof(capabilities)};
  MLN_TEST_OK(mln_gpu_device_get_capabilities(device, &capabilities, NULL));
  const mln_render_target_kind_flags borrowed =
    MLN_RENDER_TARGET_KIND_FLAG_METAL_BORROWED_TEXTURES |
    MLN_RENDER_TARGET_KIND_FLAG_VULKAN_BORROWED_TEXTURES |
    MLN_RENDER_TARGET_KIND_FLAG_OPEN_GL_BORROWED_TEXTURES |
    MLN_RENDER_TARGET_KIND_FLAG_WEB_GPU_BORROWED_TEXTURES;
  const mln_depth_stencil_format_flags formats =
    MLN_DEPTH_STENCIL_FORMAT_FLAG_DEPTH24_STENCIL8 |
    MLN_DEPTH_STENCIL_FORMAT_FLAG_DEPTH32_FLOAT_STENCIL8;
  TEST_ASSERT_EQUAL_UINT32(0, capabilities.depth_stencil_formats & ~formats);
  if (capabilities.target_kinds & borrowed) {
    TEST_ASSERT_NOT_EQUAL_UINT32(0, capabilities.depth_stencil_formats);
  }
  // Only an opaque file descriptor holds depth, and only where the device
  // exports one.
  TEST_ASSERT_EQUAL_UINT32(
    0, capabilities.depth_export_kinds & ~MLN_GPU_EXPORT_KIND_FLAG_OPAQUE_FD
  );
  TEST_ASSERT_EQUAL_UINT32(
    0, capabilities.depth_export_kinds & ~capabilities.export_kinds
  );
  mln_gpu_device_release(device);
}
#endif

static void only_an_exported_ring_keeps_depth(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_render_target target = {
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_OFFSCREEN,
    .data = {.offscreen = {.logical_size = {.width = side, .height = side}}},
  };
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options),
    .driver = MLN_RENDER_DRIVER_CORE_WORKER,
    .keep_depth = true,
  };
  const mln_completion discard = mln_test_discard_completion();
  mln_render_session session = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT32(
    MLN_STATUS_UNSUPPORTED, mln_map_attach(
                              map, &target, MLN_HANDLE_NULL, &options, &session,
                              &discard, MLN_TEST_DIAGNOSTIC
                            )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_frame_projection_places_points_where_the_frame_drew_them);
#if !defined(MLN_FFI_TEST_BACKEND_WEBGPU)
  RUN_TEST(a_device_reports_the_depth_that_its_rings_keep);
#endif
  RUN_TEST(only_an_exported_ring_keeps_depth);
}
