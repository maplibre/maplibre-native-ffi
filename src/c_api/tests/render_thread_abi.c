// Raw C ABI coverage: a render session's owner thread is the thread that
// attached it, which need not be the map's owner thread.
//
// These live below the bindings because they need a second native thread that
// attaches and drives a session against a map owned by the thread that created
// it, which is not a shape a single-threaded binding test can build.

#include <stdatomic.h>
#include <stddef.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

// A background layer needs no network, so these tests are deterministic and
// produce a known readback color.
static const char background_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":"
  "[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#ff0000\"}}]}";

// Renders until the map publishes an update for the session's extent, which
// takes at least one pump on the map owner thread. Returns the last status.
static mln_status render_until_frame(
  mln_render_session session, bool* out_rendered
) {
  mln_status status = MLN_STATUS_OK;
  *out_rendered = false;
  for (unsigned int attempt = 0; attempt < 500 && !*out_rendered;
       attempt += 1) {
    status = mln_render_session_render_update(session, out_rendered);
    if (status != MLN_STATUS_OK) {
      return status;
    }
    if (!*out_rendered) {
      mln_test_sleep_millisecond();
    }
  }
  return status;
}

typedef struct render_probe {
  mln_map map;
  atomic_bool finished;
  bool attached;
  bool rendered;
  mln_status render_status;
  mln_status readback_status;
  uint32_t width;
  uint32_t height;
  uint8_t pixel[4];
} render_probe;

static void attach_render_readback(void* argument) {
  render_probe* probe = (render_probe*)argument;
  mln_test_render_fixture fixture = {0};
  probe->attached = mln_test_render_fixture_create(probe->map, &fixture);
  if (!probe->attached) {
    atomic_store(&probe->finished, true);
    return;
  }

  probe->render_status = render_until_frame(fixture.session, &probe->rendered);

  static uint8_t pixels[64 * 64 * 4];
  mln_texture_image_info info = {.size = sizeof(mln_texture_image_info)};
  probe->readback_status = mln_texture_read_premultiplied_rgba8(
    fixture.session, pixels, sizeof(pixels), &info
  );
  probe->width = info.width;
  probe->height = info.height;
  for (size_t channel = 0; channel < 4; channel += 1) {
    probe->pixel[channel] = pixels[channel];
  }

  mln_test_render_fixture_destroy(&fixture);
  atomic_store(&probe->finished, true);
}

// The end-to-end proof: a thread that does not own the map attaches a session,
// renders a real frame, and reads it back, while the map is pumped where it was
// created. This covers the scheduler mbgl reaches for during render, the
// observer hop back to the map's run loop, the graphics context priming at
// attach, and the size gate together.
static void a_second_thread_attaches_and_renders(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, background_style_json)
  );

  render_probe probe = {.map = map};
  atomic_init(&probe.finished, false);

  mln_test_thread* thread =
    mln_test_thread_start(attach_render_readback, &probe);
  TEST_ASSERT_TRUE(mln_test_pump_until(runtime, &probe.finished));
  mln_test_thread_join(thread);

  TEST_ASSERT_TRUE(probe.attached);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.render_status);
  TEST_ASSERT_TRUE(probe.rendered);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.readback_status);
  TEST_ASSERT_EQUAL_UINT32(64, probe.width);
  TEST_ASSERT_EQUAL_UINT32(64, probe.height);
  // The style paints an opaque red background.
  TEST_ASSERT_EQUAL_UINT8(255, probe.pixel[0]);
  TEST_ASSERT_EQUAL_UINT8(0, probe.pixel[1]);
  TEST_ASSERT_EQUAL_UINT8(0, probe.pixel[2]);
  TEST_ASSERT_EQUAL_UINT8(255, probe.pixel[3]);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct foreign_call_probe {
  mln_render_session session;
  atomic_bool finished;
  mln_status render_status;
  mln_status resize_status;
  mln_status detach_status;
  mln_status destroy_status;
  mln_status reduce_memory_status;
  mln_status clear_data_status;
  mln_status readback_status;
  mln_status surface_set_target_status[3];
  mln_status texture_set_target_status[3];
} foreign_call_probe;

static void call_session_from_a_foreign_thread(void* argument) {
  foreign_call_probe* probe = (foreign_call_probe*)argument;
  bool rendered = false;
  probe->render_status =
    mln_render_session_render_update(probe->session, &rendered);
  probe->resize_status = mln_render_session_resize(probe->session, 32, 32, 1.0);
  probe->detach_status = mln_render_session_detach(probe->session);
  probe->destroy_status = mln_render_session_destroy(probe->session);
  probe->reduce_memory_status =
    mln_render_session_reduce_memory_use(probe->session);
  probe->clear_data_status = mln_render_session_clear_data(probe->session);
  mln_texture_image_info info = {.size = sizeof(mln_texture_image_info)};
  probe->readback_status =
    mln_texture_read_premultiplied_rgba8(probe->session, NULL, 0, &info);

  // Thread affinity is checked before the target kind, so these report the
  // foreign thread rather than that this session owns its texture.
  const mln_metal_surface_descriptor metal_surface =
    mln_metal_surface_descriptor_default();
  const mln_vulkan_surface_descriptor vulkan_surface =
    mln_vulkan_surface_descriptor_default();
  const mln_opengl_surface_descriptor opengl_surface =
    mln_opengl_surface_descriptor_default();
  probe->surface_set_target_status[0] =
    mln_metal_surface_set_target(probe->session, &metal_surface);
  probe->surface_set_target_status[1] =
    mln_vulkan_surface_set_target(probe->session, &vulkan_surface);
  probe->surface_set_target_status[2] =
    mln_opengl_surface_set_target(probe->session, &opengl_surface);

  const mln_metal_borrowed_texture_descriptor metal_texture =
    mln_metal_borrowed_texture_descriptor_default();
  const mln_vulkan_borrowed_texture_descriptor vulkan_texture =
    mln_vulkan_borrowed_texture_descriptor_default();
  const mln_opengl_borrowed_texture_descriptor opengl_texture =
    mln_opengl_borrowed_texture_descriptor_default();
  probe->texture_set_target_status[0] =
    mln_metal_borrowed_texture_set_target(probe->session, &metal_texture);
  probe->texture_set_target_status[1] =
    mln_vulkan_borrowed_texture_set_target(probe->session, &vulkan_texture);
  probe->texture_set_target_status[2] =
    mln_opengl_borrowed_texture_set_target(probe->session, &opengl_texture);
  atomic_store(&probe->finished, true);
}

// Attaching binds the session to the attaching thread, so every entry point
// rejects any other thread. This pins the whole affinity contract in one place,
// including that destroy cannot be used to escape it.
static void session_entry_points_reject_a_foreign_thread(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  foreign_call_probe probe = {.session = fixture.session};
  atomic_init(&probe.finished, false);

  mln_test_thread* thread =
    mln_test_thread_start(call_session_from_a_foreign_thread, &probe);
  TEST_ASSERT_TRUE(mln_test_pump_until(runtime, &probe.finished));
  mln_test_thread_join(thread);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_WRONG_THREAD, probe.render_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_WRONG_THREAD, probe.resize_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_WRONG_THREAD, probe.detach_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_WRONG_THREAD, probe.destroy_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_WRONG_THREAD, probe.reduce_memory_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_WRONG_THREAD, probe.clear_data_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_WRONG_THREAD, probe.readback_status);
  for (size_t index = 0; index < 3; index += 1) {
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_WRONG_THREAD, probe.surface_set_target_status[index]
    );
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_WRONG_THREAD, probe.texture_set_target_status[index]
    );
  }

  // The session survived every rejected call and is still usable here.
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct hold_session_probe {
  mln_map map;
  atomic_bool attached;
  atomic_bool start_destroy;
  atomic_bool destroyed;
  bool attach_succeeded;
  mln_status destroy_status;
  mln_test_render_fixture fixture;
} hold_session_probe;

static void attach_hold_destroy(void* argument) {
  hold_session_probe* probe = (hold_session_probe*)argument;
  probe->attach_succeeded =
    mln_test_render_fixture_create(probe->map, &probe->fixture);
  atomic_store(&probe->attached, true);
  while (!atomic_load(&probe->start_destroy)) {
    mln_test_sleep_millisecond();
  }
  if (probe->attach_succeeded) {
    probe->destroy_status = mln_render_session_destroy(probe->fixture.session);
    probe->fixture.session = MLN_HANDLE_NULL;
    mln_test_render_fixture_destroy(&probe->fixture);
  }
  atomic_store(&probe->destroyed, true);
}

// The map cannot be destroyed while a session is attached, and that has to hold
// when the session belongs to a thread the destroying thread never observes.
// The map registry mutex is what makes the check race-free rather than the
// owner-thread check that used to imply it.
static void map_destroy_rejects_a_session_owned_by_another_thread(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  hold_session_probe probe = {.map = map};
  atomic_init(&probe.attached, false);
  atomic_init(&probe.start_destroy, false);
  atomic_init(&probe.destroyed, false);

  mln_test_thread* thread = mln_test_thread_start(attach_hold_destroy, &probe);
  TEST_ASSERT_TRUE(mln_test_pump_until(runtime, &probe.attached));
  TEST_ASSERT_TRUE(probe.attach_succeeded);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_STATE, mln_map_destroy(map));

  atomic_store(&probe.start_destroy, true);
  TEST_ASSERT_TRUE(mln_test_pump_until(runtime, &probe.destroyed));
  mln_test_thread_join(thread);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.destroy_status);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct resize_probe {
  mln_map map;
  atomic_bool ready_to_resize;
  atomic_bool start_resize;
  atomic_bool immediate_render_checked;
  atomic_bool finished;
  bool attached;
  bool observed_no_update;
  bool rendered;
  mln_status resize_status;
} resize_probe;

static void attach_resize_render(void* argument) {
  resize_probe* probe = (resize_probe*)argument;
  mln_test_render_fixture fixture = {0};
  probe->attached = mln_test_render_fixture_create(probe->map, &fixture);
  if (!probe->attached) {
    atomic_store(&probe->ready_to_resize, true);
    atomic_store(&probe->immediate_render_checked, true);
    atomic_store(&probe->finished, true);
    return;
  }

  atomic_store(&probe->ready_to_resize, true);
  while (!atomic_load(&probe->start_resize)) {
    mln_test_sleep_millisecond();
  }

  probe->resize_status =
    mln_render_session_resize(fixture.session, 96, 48, 1.0);

  // Immediately after the resize the map still holds an update built for the
  // previous extent, so rendering must report no frame rather than project it
  // into the new target.
  bool rendered = true;
  if (
    mln_render_session_render_update(fixture.session, &rendered) ==
      MLN_STATUS_OK &&
    !rendered
  ) {
    probe->observed_no_update = true;
  }
  atomic_store(&probe->immediate_render_checked, true);

  if (!rendered) {
    (void)render_until_frame(fixture.session, &rendered);
  }
  probe->rendered = rendered;

  mln_test_render_fixture_destroy(&fixture);
  atomic_store(&probe->finished, true);
}

// A resize from the render thread reaches the map through the map's own owner
// thread. Rendering has to wait for the map to catch up, or the frame would
// take its projection from the old logical size and its viewport from the new
// physical one.
static void resize_from_the_render_thread_lands_on_the_map_thread(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, background_style_json)
  );

  resize_probe probe = {.map = map};
  atomic_init(&probe.ready_to_resize, false);
  atomic_init(&probe.start_resize, false);
  atomic_init(&probe.immediate_render_checked, false);
  atomic_init(&probe.finished, false);

  mln_test_thread* thread = mln_test_thread_start(attach_resize_render, &probe);
  TEST_ASSERT_TRUE(mln_test_pump_until(runtime, &probe.ready_to_resize));
  atomic_store(&probe.start_resize, true);

  // Keep the map owner thread from processing the queued resize until the
  // render thread has checked the update for the previous extent.
  for (unsigned int attempt = 0;
       attempt < 500 && !atomic_load(&probe.immediate_render_checked);
       attempt += 1) {
    mln_test_sleep_millisecond();
  }
  TEST_ASSERT_TRUE(atomic_load(&probe.immediate_render_checked));

  TEST_ASSERT_TRUE(mln_test_pump_until(runtime, &probe.finished));
  mln_test_thread_join(thread);

  TEST_ASSERT_TRUE(probe.attached);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.resize_status);
  TEST_ASSERT_TRUE(probe.observed_no_update);
  TEST_ASSERT_TRUE(probe.rendered);

  uint32_t width = 0;
  uint32_t height = 0;
  double scale_factor = 0.0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_size(map, &width, &height, &scale_factor)
  );
  TEST_ASSERT_EQUAL_UINT32(96, width);
  TEST_ASSERT_EQUAL_UINT32(48, height);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct runtime_after_render_probe {
  mln_map map;
  atomic_bool finished;
  mln_status create_status;
} runtime_after_render_probe;

static void render_then_create_runtime(void* argument) {
  runtime_after_render_probe* probe = (runtime_after_render_probe*)argument;
  mln_test_render_fixture fixture = {0};
  if (mln_test_render_fixture_create(probe->map, &fixture)) {
    bool rendered = false;
    (void)render_until_frame(fixture.session, &rendered);
    mln_test_render_fixture_destroy(&fixture);
  }

  // Rendering installs a scheduler as this thread's current one. If it were
  // left installed, or if looking it up had lazily created a run loop, the
  // thread could never host a runtime.
  mln_runtime runtime = MLN_HANDLE_NULL;
  const mln_runtime_options options = mln_runtime_options_default();
  probe->create_status = mln_runtime_create(&options, &runtime);
  if (probe->create_status == MLN_STATUS_OK) {
    (void)mln_runtime_destroy(runtime);
  }
  atomic_store(&probe->finished, true);
}

// Attaching and rendering must leave the thread's MapLibre scheduler state
// exactly as they found it. Creating a runtime is the cheapest observable
// proof, because runtime creation refuses a thread that already has one.
static void render_thread_is_not_poisoned_for_runtime_creation(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, background_style_json)
  );

  runtime_after_render_probe probe = {.map = map};
  atomic_init(&probe.finished, false);
  mln_test_thread* thread =
    mln_test_thread_start(render_then_create_runtime, &probe);
  TEST_ASSERT_TRUE(mln_test_pump_until(runtime, &probe.finished));
  mln_test_thread_join(thread);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.create_status);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct token_probe {
  atomic_bool finished;
  uint64_t token;
} token_probe;

static void read_thread_token(void* argument) {
  token_probe* probe = (token_probe*)argument;
  probe->token = mln_thread_token();
  atomic_store(&probe->finished, true);
}

// Hosts whose unit of execution is not pinned to a native thread compare this
// token to detect that they moved, so it has to be stable within a thread and
// distinct between live ones.
static void thread_tokens_are_stable_and_distinct(void) {
  const uint64_t first = mln_thread_token();
  TEST_ASSERT_NOT_EQUAL_UINT64(0, first);
  TEST_ASSERT_EQUAL_UINT64(first, mln_thread_token());

  token_probe probe = {0};
  atomic_init(&probe.finished, false);
  mln_test_thread* thread = mln_test_thread_start(read_thread_token, &probe);
  mln_test_thread_join(thread);

  TEST_ASSERT_NOT_EQUAL_UINT64(0, probe.token);
  TEST_ASSERT_NOT_EQUAL_UINT64(first, probe.token);
  TEST_ASSERT_EQUAL_UINT64(first, mln_thread_token());
}

void run_render_thread_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(thread_tokens_are_stable_and_distinct);
  RUN_TEST(a_second_thread_attaches_and_renders);
  RUN_TEST(session_entry_points_reject_a_foreign_thread);
  RUN_TEST(map_destroy_rejects_a_session_owned_by_another_thread);
  RUN_TEST(resize_from_the_render_thread_lands_on_the_map_thread);
  RUN_TEST(render_thread_is_not_poisoned_for_runtime_creation);
}
