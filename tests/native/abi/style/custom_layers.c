// Custom layers: their place in the style, the order of their callbacks across
// a frame, and the release of their user data.

#include <stdatomic.h>
#include <string.h>

#include "support/style.h"
#include "support/test_support.h"

enum { call_capacity = 64 };

// The callbacks of every host in a case, in the order they ran.
typedef struct call_log {
  atomic_int length;
  char calls[call_capacity][3];
} call_log;

// A host that records its calls. The callbacks run on the session's thread,
// and the case reads what they wrote once a frame result or completion that
// follows them arrives.
typedef struct recording_host {
  char name;
  call_log* log;
  atomic_int prerenders;
  atomic_int renders;
  atomic_int deinitializations;
  atomic_int context_losses;
  atomic_int releases;
  mln_custom_layer_render_parameters render;
} recording_host;

static void record(recording_host* host, char kind) {
  if (host->log == NULL) return;
  const int index = atomic_fetch_add(&host->log->length, 1);
  if (index < call_capacity) {
    host->log->calls[index][0] = kind;
    host->log->calls[index][1] = host->name;
    host->log->calls[index][2] = '\0';
  }
}

static void on_prerender(
  void* user_data, const mln_custom_layer_prerender_parameters* parameters
) {
  recording_host* host = user_data;
  TEST_ASSERT_NOT_NULL(parameters);
  record(host, 'P');
  atomic_fetch_add(&host->prerenders, 1);
}

static void on_render(
  void* user_data, const mln_custom_layer_render_parameters* parameters
) {
  recording_host* host = user_data;
  host->render = *parameters;
  record(host, 'R');
  atomic_fetch_add(&host->renders, 1);
}

static void on_deinitialize(void* user_data) {
  recording_host* host = user_data;
  record(host, 'D');
  atomic_fetch_add(&host->deinitializations, 1);
}

static void on_context_lost(void* user_data) {
  recording_host* host = user_data;
  record(host, 'L');
  atomic_fetch_add(&host->context_losses, 1);
}

static void on_release(void* user_data) {
  recording_host* host = user_data;
  atomic_fetch_add(&host->releases, 1);
}

static mln_custom_layer_host host_of(recording_host* host) {
  return (mln_custom_layer_host){
    .size = sizeof(mln_custom_layer_host),
    .prerender = on_prerender,
    .render = on_render,
    .deinitialize = on_deinitialize,
    .context_lost = on_context_lost,
    .user_data = host,
    .release_user_data = on_release,
  };
}

static void add_background(mln_map map, const char* id, const char* paint) {
  char json[256];
  snprintf(
    json, sizeof(json), "{\"id\":\"%s\",\"type\":\"background\",\"paint\":%s}",
    id, paint
  );
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map, mln_buffer_view_from_cstr(json), (mln_buffer_view){0},
    &completion.descriptor, NULL
  ));
}

static void add_custom_layer(
  mln_map map, const char* id, const char* before, recording_host* host
) {
  const mln_custom_layer_host registration = host_of(host);
  MLN_TEST_AWAIT_OK(mln_map_add_custom_layer(
    map, mln_buffer_view_from_cstr(id),
    before == NULL ? (mln_buffer_view){0} : mln_buffer_view_from_cstr(before),
    &registration, &completion.descriptor, NULL
  ));
}

static void remove_layer(mln_map map, const char* id) {
  MLN_TEST_AWAIT_OK(mln_map_remove_style_layer(
    map, mln_buffer_view_from_cstr(id), &completion.descriptor, NULL
  ));
}

static void expect_layers(mln_map map, const char* const* ids, size_t count) {
  const mln_test_style_list list = mln_test_style_list_layers(map);
  MLN_TEST_OK(list.status);
  TEST_ASSERT_EQUAL_size_t(count, list.count);
  for (size_t index = 0; index < count; index += 1) {
    TEST_ASSERT_EQUAL_STRING(ids[index], list.entries[index].id);
  }
}

static void expect_calls(const call_log* log, const char* const* calls) {
  size_t count = 0;
  while (calls[count] != NULL) count += 1;
  TEST_ASSERT_EQUAL_INT((int)count, atomic_load(&log->length));
  for (size_t index = 0; index < count; index += 1) {
    TEST_ASSERT_EQUAL_STRING(calls[index], log->calls[index]);
  }
}

static void a_custom_layer_goes_below_the_layer_that_it_names(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_styled_map(runtime);
  recording_host below = {.name = 'a'};
  recording_host last = {.name = 'b'};
  add_background(map, "bottom", "{}");
  add_background(map, "top", "{}");

  add_custom_layer(map, "below-top", "top", &below);
  add_custom_layer(map, "last", NULL, &last);

  const char* const ids[] = {"bottom", "below-top", "top", "last"};
  expect_layers(map, ids, 4);
  const mln_test_style_list list = mln_test_style_list_layers(map);
  TEST_ASSERT_EQUAL_STRING("custom", list.entries[1].type);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_LAYER_VISIBILITY_VISIBLE, list.entries[1].visibility
  );

  remove_layer(map, "below-top");
  const char* const remaining[] = {"bottom", "top", "last"};
  expect_layers(map, remaining, 3);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&below.releases));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&last.releases));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&below.prerenders));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&below.deinitializations));
}

// A command that fails still takes the user data, and releases it once.
static void a_failed_add_releases_its_user_data_once(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_styled_map(runtime);
  recording_host first = {.name = 'a'};
  recording_host duplicate = {.name = 'b'};
  recording_host misplaced = {.name = 'c'};
  add_custom_layer(map, "custom", NULL, &first);

  const mln_custom_layer_host duplicate_host = host_of(&duplicate);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_ALREADY_EXISTS, "custom",
    mln_map_add_custom_layer(
      map, mln_buffer_view_from_cstr("custom"), (mln_buffer_view){0},
      &duplicate_host, &completion.descriptor, NULL
    )
  );
  const mln_custom_layer_host misplaced_host = host_of(&misplaced);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "missing",
    mln_map_add_custom_layer(
      map, mln_buffer_view_from_cstr("other"),
      mln_buffer_view_from_cstr("missing"), &misplaced_host,
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&duplicate.releases));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&misplaced.releases));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&first.releases));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&first.releases));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&duplicate.releases));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&misplaced.releases));
}

// A call that returns an error leaves the user data with the caller.
static void a_rejected_add_keeps_its_user_data(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_styled_map(runtime);
  recording_host host = {.name = 'a'};
  mln_custom_layer_host registration = host_of(&host);
  registration.size = 0;

  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "size", mln_map_add_custom_layer(
              map, mln_buffer_view_from_cstr("custom"), (mln_buffer_view){0},
              &registration, &completion.descriptor, MLN_TEST_DIAGNOSTIC
            )
  );
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "host", mln_map_add_custom_layer(
              map, mln_buffer_view_from_cstr("custom"), (mln_buffer_view){0},
              NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
            )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&host.releases));
}

static void attach(mln_map map, mln_test_render_fixture* fixture) {
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, fixture));
}

static void render_frame(const mln_test_render_fixture* fixture) {
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, mln_test_style_render_frame(fixture)
  );
}

// Each frame prerenders every custom layer, bottom to top, before it renders
// any of them, in the same order.
static void a_frame_prerenders_every_layer_before_it_renders_one(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_styled_map(runtime);
  call_log log = {0};
  recording_host below = {.name = 'a', .log = &log};
  recording_host above = {.name = 'b', .log = &log};
  add_background(map, "background", "{\"background-color\":\"#ff0000\"}");
  add_custom_layer(map, "above", NULL, &above);
  add_custom_layer(map, "below", "above", &below);
  mln_test_render_fixture fixture = {0};
  attach(map, &fixture);

  render_frame(&fixture);
  render_frame(&fixture);

  const char* const calls[] = {"Pa", "Pb", "Ra", "Rb", "Pa",
                               "Pb", "Ra", "Rb", NULL};
  expect_calls(&log, calls);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Render gets the frame's size, view, and projection, and the depth of the
// layer's place, which is lower for a higher layer.
static void render_gets_the_frame_and_the_layer_depth(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_styled_map(runtime);
  recording_host below = {.name = 'a'};
  recording_host above = {.name = 'b'};
  add_background(map, "background", "{\"background-color\":\"#ff0000\"}");
  add_custom_layer(map, "below", NULL, &below);
  add_custom_layer(map, "above", NULL, &above);
  mln_test_render_fixture fixture = {0};
  attach(map, &fixture);

  render_frame(&fixture);

  const mln_custom_layer_render_parameters* render = &below.render;
  TEST_ASSERT_GREATER_OR_EQUAL_UINT32(
    sizeof(mln_custom_layer_render_parameters), render->size
  );
  TEST_ASSERT_TRUE(
    render->backend >= MLN_RENDER_BACKEND_METAL &&
    render->backend <= MLN_RENDER_BACKEND_WEB_GPU
  );
  TEST_ASSERT_EQUAL_UINT32(64, render->frame.width);
  TEST_ASSERT_EQUAL_UINT32(64, render->frame.height);
  TEST_ASSERT_EQUAL_UINT32(64, render->frame.view.extent.width);
  TEST_ASSERT_EQUAL_UINT32(64, render->frame.view.extent.height);
  TEST_ASSERT_EQUAL_DOUBLE(1.0, render->frame.view.extent.scale_factor);
  TEST_ASSERT_FALSE(render->frame.frame_projection.globe);
  const double far_depth = render->frame.frame_projection.far_depth;
  TEST_ASSERT_TRUE(far_depth > 0.0 && far_depth < 1.0);
  TEST_ASSERT_TRUE(render->layer_depth > far_depth);
  TEST_ASSERT_TRUE(render->layer_depth <= 1.0);
  TEST_ASSERT_TRUE(above.render.layer_depth < render->layer_depth);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A session that stops drawing a layer deinitializes it once: when the layer
// leaves the style, or when the session detaches. The user data goes once the
// map no longer has the layer.
static void a_session_deinitializes_each_layer_that_it_stops_drawing(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_styled_map(runtime);
  call_log log = {0};
  recording_host removed = {.name = 'a', .log = &log};
  recording_host kept = {.name = 'b', .log = &log};
  add_background(map, "background", "{\"background-color\":\"#ff0000\"}");
  add_custom_layer(map, "removed", NULL, &removed);
  add_custom_layer(map, "kept", NULL, &kept);
  mln_test_render_fixture fixture = {0};
  attach(map, &fixture);
  render_frame(&fixture);

  remove_layer(map, "removed");
  render_frame(&fixture);
  mln_test_render_fixture_destroy(&fixture);

  const char* const calls[] = {"Pa", "Pb", "Ra", "Rb", "Da",
                               "Pb", "Rb", "Db", NULL};
  expect_calls(&log, calls);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&removed.context_losses));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&kept.context_losses));
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&removed.releases));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&kept.releases));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&removed.releases));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&kept.releases));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&kept.deinitializations));
}

// An abandoned session's graphics objects may be gone, so its layers lose
// their context instead of deinitializing.
static void an_abandoned_session_loses_its_layers_context(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_styled_map(runtime);
  recording_host host = {.name = 'a'};
  add_background(map, "background", "{\"background-color\":\"#ff0000\"}");
  add_custom_layer(map, "custom", NULL, &host);
  mln_test_render_fixture fixture = {0};
  attach(map, &fixture);
  render_frame(&fixture);

  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  MLN_TEST_OK(mln_render_session_abandon(fixture.session, &abandoned, NULL));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&host.context_losses));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&host.deinitializations));
  mln_test_render_fixture_destroy(&fixture);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&host.releases));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&host.context_losses));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&host.deinitializations));
}

MLN_TEST_GROUP {
  RUN_TEST(a_custom_layer_goes_below_the_layer_that_it_names);
  RUN_TEST(a_failed_add_releases_its_user_data_once);
  RUN_TEST(a_rejected_add_keeps_its_user_data);
  RUN_TEST(a_frame_prerenders_every_layer_before_it_renders_one);
  RUN_TEST(render_gets_the_frame_and_the_layer_depth);
  RUN_TEST(a_session_deinitializes_each_layer_that_it_stops_drawing);
  RUN_TEST(an_abandoned_session_loses_its_layers_context);
}
