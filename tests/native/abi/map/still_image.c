// Still images: a static or tile map renders one on request through a frame
// demand of its own on its render session, which resizes between them. The
// demand outlasts barriers and detachment until the image renders, and never
// replaces a host demand or is replaced by one. A map in another mode refuses
// the request, only one request is pending at a time, and closing the map
// cancels the pending one.

#include "support/frames.h"
#include "support/map.h"
#include "support/resources.h"
#include "support/test_support.h"

static mln_map create_map_in_mode(mln_runtime runtime, uint32_t map_mode) {
  mln_map_options options = mln_map_options_default();
  options.initial_extent =
    (mln_logical_extent){.width = 64, .height = 64, .scale_factor = 1.0};
  options.map_mode = map_mode;
  return mln_test_create_map_with_options(runtime, &options);
}

// Reads back the center pixel of the frame the session rendered last, which
// is `width` by `height` pixels.
static void read_center_pixel(
  const mln_test_render_fixture* fixture, uint32_t width, uint32_t height,
  uint8_t out_rgba[4]
) {
  mln_test_completion readback = mln_test_completion_readback();
  MLN_TEST_OK(mln_render_session_read_texture(
    fixture->session, &readback.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &readback));
  mln_texture_readback_result image = {0};
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&readback, &image, sizeof(image))
  );
  TEST_ASSERT_EQUAL_UINT32(width, image.info.width);
  TEST_ASSERT_EQUAL_UINT32(height, image.info.height);
  TEST_ASSERT_EQUAL_size_t((size_t)width * height * 4, image.data.size);
  const uint8_t* pixels = image.data.data;
  memcpy(out_rgba, pixels + (((size_t)height / 2 * width) + width / 2) * 4, 4);
  mln_test_completion_destroy(&readback);
}

// Each still image renders the map as it stands when the image is taken, so a
// second request after a style change shows the new style. The host demands
// no frame: the request's own demand renders the image, and its one result
// carries the still-image token.
static void a_static_map_renders_still_images(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_map_in_mode(runtime, MLN_MAP_MODE_STATIC);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_test_completion still = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_request_still_image(map, &still.descriptor, NULL));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &still));
  mln_test_completion_destroy(&still);
  mln_render_frame_result result = {0};
  mln_test_render_wait_for_result_list(&fixture, 1, &result);
  TEST_ASSERT_EQUAL_UINT64(MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  uint8_t rgba[4] = {0};
  read_center_pixel(&fixture, 64, 64, rgba);
  TEST_ASSERT_EQUAL_UINT8(255, rgba[0]);
  TEST_ASSERT_EQUAL_UINT8(0, rgba[1]);
  TEST_ASSERT_EQUAL_UINT8(0, rgba[2]);
  TEST_ASSERT_EQUAL_UINT8(255, rgba[3]);

  static const char green_background_style_json[] =
    "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"bg\","
    "\"type\":\"background\",\"paint\":{\"background-color\":\"#00ff00\"}}]}";
  mln_test_load_style_and_wait(
    map, MLN_BUFFER_LITERAL(green_background_style_json)
  );
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
  read_center_pixel(&fixture, 64, 64, rgba);
  TEST_ASSERT_EQUAL_UINT8(0, rgba[0]);
  TEST_ASSERT_EQUAL_UINT8(255, rgba[1]);
  TEST_ASSERT_EQUAL_UINT8(0, rgba[2]);
  TEST_ASSERT_EQUAL_UINT8(255, rgba[3]);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A static or tile map publishes a render update only for a still image, so a
// session resize applies without waiting for one. The next still image renders
// at the new extent, and detach, which runs after the resize, completes.
static void a_tile_map_session_resizes_between_still_images(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_map_in_mode(runtime, MLN_MAP_MODE_TILE);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));

  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_resize(
      fixture.session,
      (mln_logical_extent){.width = 96, .height = 48, .scale_factor = 1.0},
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
  uint8_t rgba[4] = {0};
  read_center_pixel(&fixture, 96, 48, rgba);
  TEST_ASSERT_EQUAL_UINT8(255, rgba[0]);
  TEST_ASSERT_EQUAL_UINT8(0, rgba[1]);

  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_detach(fixture.session, &completion.descriptor, NULL)
  );
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// One vector source with one tile at zoom 0, which a 64 by 64 static map
// requests for its still image. The provider holds the first request, so the
// image waits for the case to answer it, and answers every later one with an
// empty tile.
static const char tiled_style_json[] =
  "{\"version\":8,\"sources\":{\"tiles\":{\"type\":\"vector\",\"tiles\":"
  "[\"custom://tiles/{z}/{x}/{y}.pbf\"],\"minzoom\":0,\"maxzoom\":0}},"
  "\"layers\":[{\"id\":\"bg\",\"type\":\"background\",\"paint\":"
  "{\"background-color\":\"#ff0000\"}},{\"id\":\"fill\",\"type\":\"fill\","
  "\"source\":\"tiles\",\"source-layer\":\"any\"}]}";
static const char tile_url[] = "custom://tiles/0/0/0.pbf";
static const mln_test_provided_resource held_tile[] = {
  {.url = tile_url,
   .held = true,
   .has_later_response = true,
   .later_response = {.status = MLN_RESOURCE_RESPONSE_STATUS_OK}},
};

typedef struct held_tile_map {
  mln_test_provider* provider;
  mln_runtime runtime;
  mln_map map;
} held_tile_map;

static held_tile_map create_held_tile_map(void) {
  held_tile_map out = {.provider = mln_test_provider_create(held_tile, 1)};
  out.runtime = mln_test_create_runtime();
  mln_test_provider_install(out.runtime, out.provider);
  out.map = create_map_in_mode(out.runtime, MLN_MAP_MODE_STATIC);
  mln_test_load_style_and_wait(out.map, MLN_BUFFER_LITERAL(tiled_style_json));
  return out;
}

// Answers the held tile request with an empty tile. A request the renderer
// has cancelled meanwhile is released instead.
static void answer_held_tile(const held_tile_map* held) {
  const mln_test_provider_request* request =
    mln_test_provider_request_at(held->provider, tile_url, 0);
  TEST_ASSERT_NOT_NULL(request);
  const mln_resource_response empty = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
  };
  mln_test_request_answer(request->handle, &empty);
}

static void destroy_held_tile_map(held_tile_map* held) {
  mln_test_destroy_map(held->map);
  mln_test_destroy_runtime(held->runtime);
  mln_test_provider_destroy(held->provider);
}

static bool tile_requested(void* context) {
  return mln_test_provider_requests(context, tile_url) >= 1;
}

// Services `fixture` until the still image's demand has rendered once and the
// map has asked for the tile it waits on.
static void render_until_tile_requested(
  const mln_test_render_fixture* fixture, const held_tile_map* held
) {
  MLN_TEST_OK(mln_test_render_step_until(
    fixture, tile_requested, held->provider, mln_test_deadline_default(),
    "the still image's tile request"
  ));
}

// Copies the one frame result queued on `session`, which needs no service.
static mln_render_frame_result drain_one_result(mln_render_session session) {
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_drain_frame_results(session, &batch, NULL));
  const mln_render_frame_batch_view view = mln_test_render_batch_view(batch);
  TEST_ASSERT_EQUAL_size_t(1, view.result_count);
  const mln_render_frame_result result = *mln_test_render_view_result(&view, 0);
  mln_render_frame_batch_release(batch);
  return result;
}

// A request with no session attached waits for one, and the session that
// attaches renders it. When that session detaches first, its demand ends with
// MLN_RENDER_RESULT_TARGET_NOT_READY, and the next session renders the image.
static void a_still_image_waits_for_the_next_session(void) {
  held_tile_map held = create_held_tile_map();
  mln_test_completion still = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_request_still_image(held.map, &still.descriptor, NULL));

  mln_test_render_fixture first = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(held.map, &first));
  render_until_tile_requested(&first, &held);
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &first,
    mln_render_session_detach(first.session, &completion.descriptor, NULL)
  );
  const mln_render_frame_result ended = drain_one_result(first.session);
  TEST_ASSERT_EQUAL_UINT64(MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE, ended.token);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_TARGET_NOT_READY, ended.disposition
  );
  TEST_ASSERT_FALSE(mln_test_completion_poll(&still));
  mln_test_render_fixture_destroy(&first);

  // Detach cancelled the held request, so the next session's renderer asks
  // for the tile again and receives an empty one.
  mln_test_render_fixture second = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(held.map, &second));
  MLN_TEST_OK(mln_test_render_pending_still_image(&second, &still));
  mln_test_completion_destroy(&still);
  answer_held_tile(&held);
  mln_test_render_fixture_destroy(&second);
  destroy_held_tile_map(&held);
}

// A barrier ends the still image's demand like any other, with
// MLN_RENDER_RESULT_NO_UPDATE, and the request submits its next demand once
// the barrier completes. The host holds both frames of the session's ring
// meanwhile, so each demand parks, and once the host releases one the next
// demand renders the image without any map update to start it.
static void a_barrier_ends_a_still_image_demand_until_the_next(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_map_in_mode(runtime, MLN_MAP_MODE_STATIC);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  MLN_TEST_OK(
    mln_render_session_get_capabilities(fixture.session, &capabilities, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(2, capabilities.texture_ring_depth);
  mln_acquired_frame first = mln_test_render_and_acquire(&fixture, 1);
  mln_acquired_frame second = mln_test_render_and_acquire(&fixture, 2);

  mln_test_completion still = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_request_still_image(map, &still.descriptor, NULL));
  // A read behind the request completes once the request has run and
  // submitted its demand.
  mln_test_completion fence =
    mln_test_completion_default(sizeof(mln_camera_query_result));
  MLN_TEST_OK(mln_map_get_camera(map, &fence.descriptor, NULL));
  mln_camera_query_result camera = {0};
  MLN_TEST_OK(
    mln_test_completion_finish_value(&fence, &camera, sizeof(camera))
  );
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_barrier(fixture.session, &completion.descriptor, NULL)
  );
  mln_render_frame_result ended = {0};
  mln_test_render_wait_for_result_list(&fixture, 1, &ended);
  TEST_ASSERT_EQUAL_UINT64(MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE, ended.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_NO_UPDATE, ended.disposition);
  TEST_ASSERT_FALSE(mln_test_completion_poll(&still));

  mln_test_render_release_frame(&first);
  MLN_TEST_OK(mln_test_render_pending_still_image(&fixture, &still));
  mln_test_completion_destroy(&still);
  mln_test_render_release_frame(&second);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A host demand can neither carry the still-image token nor replace the
// still image's demand, even with the same flags, and the still image's
// demand does not replace the host's. Both reach a result of their own.
static void host_demands_and_a_still_image_demand_never_replace_each_other(
  void
) {
  held_tile_map held = create_held_tile_map();
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(held.map, &fixture));
  mln_test_completion still = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_request_still_image(held.map, &still.descriptor, NULL));
  render_until_tile_requested(&fixture, &held);

  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_IF_NEEDED | MLN_FRAME_DEMAND_PRESENT |
                 MLN_FRAME_DEMAND_WAIT_FOR_UPDATE;
  demand.token = MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE;
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_ARGUMENT, mln_render_session_request_frame(
                                   fixture.session, &demand, MLN_TEST_DIAGNOSTIC
                                 )
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "reserved"));
  demand.token = 7;
  MLN_TEST_OK(mln_render_session_request_frame(fixture.session, &demand, NULL));

  answer_held_tile(&held);
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &still));
  mln_test_completion_destroy(&still);
  // The host's demand waits for an update after the image's, which a static
  // map publishes only for its next still image, so a barrier ends its wait.
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_barrier(fixture.session, &completion.descriptor, NULL)
  );
  mln_render_frame_result results[2] = {0};
  mln_test_render_wait_for_result_list(&fixture, 2, results);
  bool saw_still_image = false;
  bool saw_host = false;
  for (size_t index = 0; index < 2; ++index) {
    TEST_ASSERT_NOT_EQUAL_UINT32(
      MLN_RENDER_RESULT_SUPERSEDED, results[index].disposition
    );
    saw_still_image |=
      results[index].token == MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE;
    saw_host |= results[index].token == 7;
  }
  TEST_ASSERT_TRUE(saw_still_image);
  TEST_ASSERT_TRUE(saw_host);
  mln_test_render_fixture_destroy(&fixture);
  destroy_held_tile_map(&held);
}

// A continuous map renders frames on demand and has no still image to give.
// The refused request leaves the completion with the caller.
static void a_continuous_map_refuses_a_still_image(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_map_in_mode(runtime, MLN_MAP_MODE_CONTINUOUS);
  mln_test_completion refused = mln_test_completion_default(0);
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_map_request_still_image(map, &refused.descriptor, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL(
    strstr(mln_test_last_error(), "map is not in static or tile mode")
  );
  refused.descriptor.release_user_data(refused.descriptor.user_data);
  mln_test_completion_destroy(&refused);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A second request that reaches the map worker while the first is pending
// completes MLN_STATUS_INVALID_STATE, and closing the map cancels the first.
static void map_close_cancels_the_pending_still_image(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_map_in_mode(runtime, MLN_MAP_MODE_STATIC);

  // Without a render session the first request waits for one.
  mln_test_completion pending = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_request_still_image(map, &pending.descriptor, NULL));
  mln_test_completion duplicate = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_request_still_image(map, &duplicate.descriptor, NULL));
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE, mln_test_completion_finish(&duplicate)
  );
  mln_test_completion_destroy(&duplicate);

  mln_test_destroy_map(map);
  MLN_TEST_STATUS(MLN_STATUS_CANCELLED, mln_test_completion_finish(&pending));
  mln_test_completion_destroy(&pending);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_static_map_renders_still_images);
  RUN_TEST(a_tile_map_session_resizes_between_still_images);
  RUN_TEST(a_still_image_waits_for_the_next_session);
  RUN_TEST(a_barrier_ends_a_still_image_demand_until_the_next);
  RUN_TEST(host_demands_and_a_still_image_demand_never_replace_each_other);
  RUN_TEST(a_continuous_map_refuses_a_still_image);
  RUN_TEST(map_close_cancels_the_pending_still_image);
}
