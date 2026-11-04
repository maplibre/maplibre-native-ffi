// A WebGL surface whose OffscreenCanvas the host transfers to the library,
// which the core worker then drives.

#include <emscripten.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <stdbool.h>
#include <stdint.h>

#include "support/frames.h"
#include "support/test_support.h"

static void transferred_offscreen_canvas_runs_on_core_worker(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_transferred_webgl_surface_create(map, &fixture));

  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture.session, &snapshot, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_ATTACHED, snapshot.state);

  mln_test_render_demand_forced(&fixture, 901);
  mln_render_frame_batch batch = mln_test_render_wait_for_results(&fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  TEST_ASSERT_EQUAL_UINT64(901, result.token);
  mln_render_frame_batch_release(batch);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#define PAGE_CANVAS_ID "mln-presented"

static mln_buffer_view background_style(const char* color) {
  static char json[256];
  (void)snprintf(
    json, sizeof(json),
    "{\"version\":8,\"transition\":{\"duration\":0,\"delay\":0},"
    "\"sources\":{},\"layers\":[{\"id\":\"background\","
    "\"type\":\"background\",\"paint\":{\"background-color\":\"%s\"}}]}",
    color
  );
  return mln_buffer_view_from_cstr(json);
}

EM_JS(void, add_page_canvas_here, (void), {
  const canvas = document.createElement("canvas");
  canvas.id = "mln-presented";
  canvas.width = 64;
  canvas.height = 64;
  document.body.appendChild(canvas);
});

EM_JS(void, remove_page_canvas_here, (void), {
  document.getElementById("mln-presented").remove();
});

// The RGBA of the center pixel that the page shows for its canvas, which is
// the frame the browser last presented.
EM_JS(uint32_t, page_canvas_center_here, (void), {
  const copy = new OffscreenCanvas(64, 64);
  const context = copy.getContext("2d");
  context.drawImage(document.getElementById("mln-presented"), 0, 0);
  const pixel = context.getImageData(32, 32, 1, 1).data;
  return ((pixel[0] << 24) | (pixel[1] << 16) | (pixel[2] << 8) | pixel[3]) >>>
         0;
});

static void add_page_canvas_task(void* unused) {
  (void)unused;
  add_page_canvas_here();
}

static void remove_page_canvas_task(void* unused) {
  (void)unused;
  remove_page_canvas_here();
}

static void page_canvas_center_task(void* out) {
  *(uint32_t*)out = page_canvas_center_here();
}

// The suite runs on a pthread, and the page belongs to the main thread.
static void on_main_thread(void (*task)(void*), void* arg) {
  TEST_ASSERT_EQUAL_INT(
    1, emscripten_proxy_sync(
         emscripten_proxy_get_system_queue(),
         emscripten_main_runtime_thread_id(), task, arg
       )
  );
}

static uint32_t page_canvas_center(void) {
  uint32_t center = 0;
  on_main_thread(page_canvas_center_task, &center);
  return center;
}

static bool page_shows(void* expected) {
  return page_canvas_center() == *(const uint32_t*)expected;
}

static void render_and_await_page(
  const mln_test_render_fixture* fixture, uint64_t token, uint32_t expected
) {
  mln_test_render_demand_forced(fixture, token);
  mln_render_frame_batch batch = mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_TRUE(mln_test_await(
    page_shows, &expected, mln_test_deadline_default(),
    "the page to show the rendered frame"
  ));
}

// The browser presents a worker's canvas only between the worker's event loop
// tasks, so a worker that blocked for the session's life would render frames
// that the page never shows.
static void transferred_page_canvas_presents_each_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_json(map, background_style("#ff0000")));
  MLN_TEST_OK(mln_test_map_fence(map));
  on_main_thread(add_page_canvas_task, NULL);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(
    mln_test_page_webgl_surface_create(map, PAGE_CANVAS_ID, &fixture)
  );

  render_and_await_page(&fixture, 1, 0xff0000ffU);

  // A later frame shows too, while the session stays attached.
  MLN_TEST_OK(mln_test_map_set_style_json(map, background_style("#0000ff")));
  MLN_TEST_OK(mln_test_map_fence(map));
  render_and_await_page(&fixture, 2, 0x0000ffffU);

  mln_test_render_fixture_destroy(&fixture);
  on_main_thread(remove_page_canvas_task, NULL);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(transferred_offscreen_canvas_runs_on_core_worker);
  RUN_TEST(transferred_page_canvas_presents_each_frame);
}
