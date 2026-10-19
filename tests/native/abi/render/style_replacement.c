#include "support/frames.h"
#include "support/host_graphics.h"
#include "support/resources.h"
#include "support/test_support.h"

#if !defined(__EMSCRIPTEN__)

typedef struct replacement_wait {
  mln_runtime runtime;
  const mln_test_render_fixture* fixture;
  uint64_t token;
  uint64_t generation;
  uint64_t retained_frame;
  unsigned int observed_frame_wakes;
  bool host_requested;
  bool pending;
  bool idle;
  bool full;
} replacement_wait;

static mln_render_session_snapshot session_snapshot(
  const replacement_wait* wait
) {
  mln_render_session_snapshot snapshot = {.size = sizeof(snapshot)};
  MLN_TEST_OK(
    mln_render_session_get_snapshot(wait->fixture->session, &snapshot, NULL)
  );
  return snapshot;
}

static bool frame_finished_while_retained(void* context) {
  replacement_wait* wait = context;
  const mln_render_session_snapshot snapshot = session_snapshot(wait);
  return snapshot.frame_generation > wait->retained_frame &&
         snapshot.pending_demand_count == 0;
}

static bool repaint_waits_for_a_demand(void* context) {
  const mln_render_session_snapshot snapshot = session_snapshot(context);
  return snapshot.pending_changes && snapshot.pending_demand_count == 0;
}

static void request_next(replacement_wait* wait) {
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_IF_NEEDED | MLN_FRAME_DEMAND_WAIT_FOR_UPDATE |
                 MLN_FRAME_DEMAND_PRESENT;
  demand.token = ++wait->token;
  MLN_TEST_OK(
    mln_render_session_request_frame(wait->fixture->session, &demand, NULL)
  );
  wait->pending = true;
}

static bool note_settlement(
  const mln_runtime_event* event, const char* messages, void* context
) {
  (void)messages;
  replacement_wait* wait = context;
  if (event->generation < wait->generation) return false;
  TEST_ASSERT_NOT_EQUAL_UINT32(
    MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, event->type
  );
  if (event->type == MLN_RUNTIME_EVENT_MAP_IDLE) wait->idle = true;
  if (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED) {
    wait->full = event->payload.render_frame.mode == MLN_RENDER_MODE_FULL &&
                 !event->payload.render_frame.needs_repaint;
  }
  return false;
}

static void drain_requested_frame(replacement_wait* wait) {
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  const mln_status status = mln_render_session_drain_frame_results(
    wait->fixture->session, &batch, NULL
  );
  if (status == MLN_STATUS_OK) {
    const mln_render_frame_batch_view view = mln_test_render_batch_view(batch);
    TEST_ASSERT_EQUAL_size_t(1, view.result_count);
    const mln_render_frame_result result =
      *mln_test_render_view_result(&view, 0);
    mln_render_frame_batch_release(batch);
    TEST_ASSERT_EQUAL_UINT64(wait->token, result.token);
    TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
    wait->pending = false;
    mln_acquired_frame frame = MLN_HANDLE_NULL;
    MLN_TEST_OK(
      mln_render_session_acquire_frame(wait->fixture->session, &frame, NULL)
    );
    TEST_ASSERT_NOT_EQUAL(MLN_HANDLE_NULL, frame);
    mln_test_render_release_frame(&frame);
  } else {
    MLN_TEST_STATUS(MLN_STATUS_NOT_READY, status);
  }
  if (!wait->pending) request_next(wait);
}

static bool replacement_settled(void* context) {
  replacement_wait* wait = context;
  const unsigned int wakes = atomic_load(&wait->fixture->frame_wakes);
  if (wakes != wait->observed_frame_wakes) {
    wait->observed_frame_wakes = wakes;
    wait->host_requested = true;
  }
  if (wait->host_requested) {
    wait->host_requested = false;
    drain_requested_frame(wait);
  }
  (void)mln_test_drain_counting_matching(wait->runtime, note_settlement, wait);
  return wait->idle && wait->full;
}

static void await_replacement(replacement_wait* wait) {
  MLN_TEST_OK(mln_test_render_step_until(
    wait->fixture, replacement_settled, wait, mln_test_deadline_default(),
    "a full frame and idle after style replacement"
  ));
}

static void waiting_demands_settle_replaced_styles_with_revalidated_tiles(
  void
) {
  static const char tile_url[] = "custom://replacement/0/0/0.pbf";
  // An empty vector tile with one layer named "any".
  static const uint8_t tile[] = {0x1a, 0x0a, 0x0a, 0x03, 'a',  'n',
                                 'y',  0x28, 0x80, 0x20, 0x78, 0x02};
  static const mln_test_provided_resource resources[] = {
    {.url = tile_url,
     .response =
       {
         .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
         .bytes = tile,
         .byte_count = sizeof(tile),
         .fields = MLN_RESOURCE_RESPONSE_EXPIRES,
         .expires_unix_ms = 1,
         .etag = "replacement-tile",
         .must_revalidate = true,
       },
     .has_later_response = true,
     .later_response = {
       .status = MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED,
       .fields = MLN_RESOURCE_RESPONSE_EXPIRES,
       .expires_unix_ms = 1,
       .etag = "replacement-tile",
       .must_revalidate = true,
     }},
  };
  mln_test_provider* provider = mln_test_provider_create(resources, 1);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);
  mln_map_options options = mln_map_options_default();
  options.map_mode = MLN_MAP_MODE_CONTINUOUS;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_borrowed_ring(
    map, &fixture, 1, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
  ));
  replacement_wait wait = {.runtime = runtime, .fixture = &fixture};
  request_next(&wait);
  await_replacement(&wait);

  for (unsigned int revision = 0; revision < 6; revision += 1) {
    wait.retained_frame = session_snapshot(&wait).frame_generation;
    char style[768];
    const int size = snprintf(
      style, sizeof(style),
      "{\"version\":8,\"transition\":{\"duration\":0,\"delay\":0},"
      "\"sources\":{\"tiles-%u\":{\"type\":\"vector\","
      "\"tiles\":[\"custom://replacement/{z}/{x}/{y}.pbf\"],"
      "\"minzoom\":0,\"maxzoom\":0}},\"layers\":[{\"id\":\"fill\","
      "\"type\":\"fill\",\"source\":\"tiles-%u\",\"source-layer\":\"any\"}]}",
      revision, revision
    );
    TEST_ASSERT_GREATER_THAN_INT(0, size);
    TEST_ASSERT_LESS_THAN_INT(sizeof(style), size);
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_OK(mln_map_set_style_json(
      map, mln_test_buffer_view(style, (size_t)size), &completion.descriptor,
      NULL
    ));
    MLN_TEST_OK(mln_test_completion_finish(&completion));
    wait.generation = mln_test_completion_generation(&completion);
    mln_test_completion_destroy(&completion);
    wait.idle = false;
    wait.full = false;

    // A host retaining its previous presentation can service the driver and
    // consume the frame wake before it resumes draining results.
    MLN_TEST_OK(mln_test_render_step_until(
      &fixture, frame_finished_while_retained, &wait,
      mln_test_deadline_default(), "a frame during retained presentation"
    ));
    const unsigned int retained_wakes = atomic_load(&fixture.frame_wakes);
    wait.observed_frame_wakes = retained_wakes;
    (void)mln_test_drain_all(runtime);
    MLN_TEST_OK(mln_test_map_request_repaint(map));
    MLN_TEST_OK(mln_test_render_step_until(
      &fixture, repaint_waits_for_a_demand, &wait, mln_test_deadline_default(),
      "a repaint without a pending demand"
    ));
    TEST_ASSERT_EQUAL_UINT32(retained_wakes, atomic_load(&fixture.frame_wakes));

    // Resuming the host loop drains the completed demand and arms its next
    // one. A map repaint alone cannot replace this host-side wake.
    wait.host_requested = true;
    await_replacement(&wait);
  }
  TEST_ASSERT_GREATER_OR_EQUAL_INT(
    6, mln_test_provider_requests(provider, tile_url)
  );
  const mln_test_provider_request* revalidated =
    mln_test_provider_request_at(provider, tile_url, 1);
  TEST_ASSERT_NOT_NULL(revalidated);
  TEST_ASSERT_EQUAL_size_t(sizeof(tile), revalidated->prior_data_size);

  (void)mln_test_drain_all(runtime);
  wait.idle = false;
  wait.full = false;
  MLN_TEST_OK(mln_test_map_request_repaint(map));
  await_replacement(&wait);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

#endif

MLN_TEST_GROUP {
#if !defined(__EMSCRIPTEN__)
  RUN_TEST(waiting_demands_settle_replaced_styles_with_revalidated_tiles);
#endif
}
