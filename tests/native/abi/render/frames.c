// Rendered frames and the texture ring that holds them.

#include <math.h>

#include "support/frames.h"

#include "support/style.h"
#include "support/test_support.h"

#if defined(MLN_FFI_TEST_BACKEND_VULKAN) || defined(MLN_FFI_TEST_BACKEND_METAL)
#include "support/host_graphics.h"
#endif

static void attach(
  mln_runtime* runtime, mln_map* map, mln_test_render_fixture* fixture,
  mln_buffer_view style
) {
  *runtime = mln_test_create_runtime();
  *map = mln_test_create_map(*runtime);
  mln_test_load_style_and_wait(*map, style);
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_borrowed(*map, fixture));
}

static void detach(
  mln_runtime runtime, mln_map map, mln_test_render_fixture* fixture
) {
  mln_test_render_fixture_destroy(fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// detach() for a session that keeps its texture ring until the process exits,
// as detach and abandon do once the host's GPU may still read a slot. A kept
// Vulkan ring is a child of the host's device, so the device stays too.
static void detach_keeping_the_ring(
  mln_runtime runtime, mln_map map, mln_test_render_fixture* fixture
) {
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_test_render_fixture_keep_graphics_until_exit(fixture);
#endif
  detach(runtime, map, fixture);
}

static void texture_readback_copies_the_latest_rendered_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  MLN_TEST_OK(mln_test_render_session_failure(fixture.session, NULL, 0));
  mln_test_completion readback = mln_test_completion_readback();
  MLN_TEST_OK(
    mln_render_session_read_texture(fixture.session, &readback.descriptor, NULL)
  );
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_test_render_fixture_finish_operation(&fixture, &readback)
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_completion_diagnostic(&readback), "no rendered frame"),
    mln_test_completion_diagnostic(&readback)
  );
  mln_test_completion_destroy(&readback);
  MLN_TEST_OK(mln_test_render_session_failure(fixture.session, NULL, 0));

  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  mln_test_render_demand_forced(&fixture, 260);
  mln_render_frame_batch_release(mln_test_render_wait_for_results(&fixture));
  mln_texture_image_info info = {0};
  uint8_t pixel[4] = {0};
  MLN_TEST_OK(mln_test_render_read_back(&fixture, &info, pixel, 4));
  TEST_ASSERT_EQUAL_UINT32(64, info.width);
  TEST_ASSERT_EQUAL_UINT32(64, info.height);
  const uint8_t red[4] = {255, 0, 0, 255};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(red, pixel, 4);
  detach(runtime, map, &fixture);
}

enum frame_backend { METAL, VULKAN, OPENGL, WEBGPU };

#if defined(MLN_FFI_TEST_BACKEND_METAL)
#define PRESET_FRAME_BACKEND METAL
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
#define PRESET_FRAME_BACKEND VULKAN
#elif defined(MLN_FFI_TEST_BACKEND_OPENGL)
#define PRESET_FRAME_BACKEND OPENGL
#else
#define PRESET_FRAME_BACKEND WEBGPU
#endif

// Every backend's frame record, so one call site can take any getter.
typedef union frame_record {
  uint32_t size;
  mln_metal_texture_frame metal;
  mln_vulkan_texture_frame vulkan;
  mln_open_gl_texture_frame opengl;
  mln_web_gpu_texture_frame webgpu;
  mln_render_frame_result result;
  mln_gpu_sync sync;
} frame_record;

static mln_status get_metal(
  mln_acquired_frame frame, frame_record* out, mln_diagnostic* diagnostic
) {
  return mln_acquired_frame_get_metal_texture(
    frame, out == NULL ? NULL : &out->metal, diagnostic
  );
}
static mln_status get_vulkan(
  mln_acquired_frame frame, frame_record* out, mln_diagnostic* diagnostic
) {
  return mln_acquired_frame_get_vulkan_texture(
    frame, out == NULL ? NULL : &out->vulkan, diagnostic
  );
}
static mln_status get_opengl(
  mln_acquired_frame frame, frame_record* out, mln_diagnostic* diagnostic
) {
  return mln_acquired_frame_get_open_gl_texture(
    frame, out == NULL ? NULL : &out->opengl, diagnostic
  );
}
static mln_status get_webgpu(
  mln_acquired_frame frame, frame_record* out, mln_diagnostic* diagnostic
) {
  return mln_acquired_frame_get_web_gpu_texture(
    frame, out == NULL ? NULL : &out->webgpu, diagnostic
  );
}
static mln_status get_result(
  mln_acquired_frame frame, frame_record* out, mln_diagnostic* diagnostic
) {
  return mln_acquired_frame_get_result(
    frame, out == NULL ? NULL : &out->result, diagnostic
  );
}
static mln_status get_producer_sync(
  mln_acquired_frame frame, frame_record* out, mln_diagnostic* diagnostic
) {
  return mln_acquired_frame_get_producer_sync(
    frame, out == NULL ? NULL : &out->sync, diagnostic
  );
}

typedef struct accessor_entry {
  const char* name;
  mln_status (*call)(mln_acquired_frame, frame_record*, mln_diagnostic*);
  uint32_t record_size;
  // Whether a frame of this preset's backend answers it.
  bool answers;
} accessor_entry;

static const accessor_entry accessors[] = {
  [METAL] =
    {"metal texture", get_metal, sizeof(mln_metal_texture_frame),
     PRESET_FRAME_BACKEND == METAL},
  [VULKAN] =
    {"vulkan texture", get_vulkan, sizeof(mln_vulkan_texture_frame),
     PRESET_FRAME_BACKEND == VULKAN},
  [OPENGL] =
    {"opengl texture", get_opengl, sizeof(mln_open_gl_texture_frame),
     PRESET_FRAME_BACKEND == OPENGL},
  [WEBGPU] =
    {"webgpu texture", get_webgpu, sizeof(mln_web_gpu_texture_frame),
     PRESET_FRAME_BACKEND == WEBGPU},
  {"result", get_result, sizeof(mln_render_frame_result), true},
  {"producer sync", get_producer_sync, sizeof(mln_gpu_sync), true},
};
enum { accessor_count = sizeof(accessors) / sizeof(accessors[0]) };

static mln_status call_with_record(
  const accessor_entry* accessor, mln_acquired_frame frame,
  frame_record* record, mln_diagnostic* diagnostic
) {
  memset(record, 0, sizeof(*record));
  record->size = accessor->record_size;
  return accessor->call(frame, record, diagnostic);
}

// The preset's texture record describes the frame the session rendered, at the
// session's extent.
static void expect_preset_texture(const frame_record* record) {
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  const mln_metal_texture_frame* frame = &record->metal;
  TEST_ASSERT_NOT_NULL(frame->texture);
  TEST_ASSERT_NOT_NULL(frame->device);
  TEST_ASSERT_NOT_EQUAL_UINT64(0, frame->pixel_format);
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  const mln_vulkan_texture_frame* frame = &record->vulkan;
  TEST_ASSERT_NOT_EQUAL_UINT64(0, frame->image);
  TEST_ASSERT_NOT_EQUAL_UINT64(0, frame->image_view);
  TEST_ASSERT_NOT_NULL(frame->device);
  TEST_ASSERT_NOT_EQUAL_UINT32(0, frame->format);
#elif defined(MLN_FFI_TEST_BACKEND_OPENGL)
  const mln_open_gl_texture_frame* frame = &record->opengl;
  TEST_ASSERT_NOT_EQUAL_UINT32(0, frame->texture);
  // GL_TEXTURE_2D.
  TEST_ASSERT_EQUAL_HEX32(0x0DE1, frame->target);
#else
  const mln_web_gpu_texture_frame* frame = &record->webgpu;
  TEST_ASSERT_NOT_EQUAL_UINT32(0, frame->texture);
  TEST_ASSERT_NOT_EQUAL_UINT32(0, frame->device);
  TEST_ASSERT_NOT_EQUAL_size_t(0, frame->format.size);
#endif
  TEST_ASSERT_EQUAL_UINT32(64, frame->width);
  TEST_ASSERT_EQUAL_UINT32(64, frame->height);
  TEST_ASSERT_EQUAL_DOUBLE(1.0, frame->scale_factor);
}

static void only_the_sessions_backend_describes_its_frame_texture(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture, mln_test_background_style_json);
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 1);
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture.session, &snapshot, NULL)
  );

  for (size_t index = 0; index < accessor_count; index += 1) {
    const accessor_entry* accessor = &accessors[index];
    frame_record record;
    const mln_status status =
      call_with_record(accessor, frame, &record, MLN_TEST_DIAGNOSTIC);
    if (!accessor->answers) {
      TEST_ASSERT_EQUAL_INT_MESSAGE(
        MLN_STATUS_UNSUPPORTED, status, accessor->name
      );
      TEST_ASSERT_NOT_NULL_MESSAGE(
        strstr(mln_test_last_error(), "different render backend"),
        mln_test_last_error()
      );
      continue;
    }
    MLN_TEST_OK_MESSAGE(status, accessor->name);
    if (accessor->call == get_result) {
      TEST_ASSERT_EQUAL_UINT32(
        MLN_RENDER_RESULT_RENDERED, record.result.disposition
      );
      TEST_ASSERT_EQUAL_UINT64(snapshot.generation, record.result.generation);
      TEST_ASSERT_EQUAL_UINT64(
        snapshot.frame_generation, record.result.frame_generation
      );
    } else if (accessor->call != get_producer_sync) {
      expect_preset_texture(&record);
    }
  }
  mln_test_render_release_frame(&frame);

  // The next frame reports its own result.
  frame = mln_test_render_and_acquire(&fixture, 2);
  mln_render_frame_result next = {.size = sizeof(mln_render_frame_result)};
  MLN_TEST_OK(mln_acquired_frame_get_result(frame, &next, NULL));
  TEST_ASSERT_EQUAL_UINT64(2, next.token);
  TEST_ASSERT_GREATER_THAN_UINT64(
    snapshot.frame_generation, next.frame_generation
  );
  mln_test_render_release_frame(&frame);
  detach(runtime, map, &fixture);
}

// One accessor call with its frame or output broken.
typedef struct accessor_call {
  mln_acquired_frame frame;
  uint32_t record_size;
  bool null_output;
} accessor_call;

static mln_acquired_frame released_frame = MLN_HANDLE_NULL;

static void null_frame(void* call) {
  ((accessor_call*)call)->frame = MLN_HANDLE_NULL;
}
static void a_released_frame(void* call) {
  ((accessor_call*)call)->frame = released_frame;
}
static void null_record(void* call) {
  ((accessor_call*)call)->null_output = true;
}
static void undersized_record(void* call) {
  ((accessor_call*)call)->record_size -= 1;
}

static mln_status call_accessor(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  const accessor_entry* accessor = context;
  const accessor_call* call = descriptor;
  frame_record record;
  memset(&record, 0, sizeof(record));
  record.size = call->record_size;
  return accessor->call(
    call->frame, call->null_output ? NULL : &record, diagnostic
  );
}

// Every accessor checks its own output and then leases the frame through one
// shared check, so each runs both steps, and one runs the shared check's every
// rule.
static void accessors_and_release_reject_a_broken_frame_or_record(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture, mln_test_background_style_json);
  released_frame = mln_test_render_and_acquire(&fixture, 1);
  const mln_acquired_frame stale = released_frame;
  mln_test_render_release_frame(&released_frame);
  released_frame = stale;
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 2);

  static const struct {
    const char* label;
    void (*mutate)(void* call);
    mln_status expected;
    // Whether every accessor runs the row, or only the first.
    bool every_accessor;
  } breakages[] = {
    {"released frame", a_released_frame, MLN_STATUS_INVALID_STATE, true},
    {"undersized output", undersized_record, MLN_STATUS_INVALID_ARGUMENT, true},
    {"null output", null_record, MLN_STATUS_INVALID_ARGUMENT, true},
    {"null frame", null_frame, MLN_STATUS_INVALID_ARGUMENT, false},
  };
  enum { breakage_count = sizeof(breakages) / sizeof(breakages[0]) };
  for (size_t index = 0; index < accessor_count; index += 1) {
    char labels[breakage_count][64];
    mln_test_validation_case cases[breakage_count];
    size_t case_count = 0;
    for (size_t row = 0; row < breakage_count; row += 1) {
      if (index != 0 && !breakages[row].every_accessor) continue;
      (void)snprintf(
        labels[case_count], sizeof(labels[case_count]), "%s: %s",
        accessors[index].name, breakages[row].label
      );
      cases[case_count] = (mln_test_validation_case){
        labels[case_count], breakages[row].mutate, breakages[row].expected, NULL
      };
      case_count += 1;
    }
    const accessor_call defaults = {
      .frame = frame, .record_size = accessors[index].record_size
    };
    mln_test_run_validation_table(
      cases, case_count, &defaults, sizeof(defaults), call_accessor,
      (void*)&accessors[index]
    );
  }

  const mln_gpu_sync sync = {
    .size = sizeof(mln_gpu_sync), .kind = MLN_GPU_SYNC_CPU_COMPLETE
  };
  MLN_TEST_INVALID(mln_acquired_frame_release(MLN_HANDLE_NULL, &sync, NULL));
  mln_acquired_frame stale_copy = stale;
  MLN_TEST_INVALID_STATE(mln_acquired_frame_release(stale_copy, &sync, NULL));
  mln_gpu_sync refused = sync;
  refused.size = sizeof(mln_gpu_sync) - 1;
  MLN_TEST_INVALID(mln_acquired_frame_release(frame, &refused, NULL));
  MLN_TEST_INVALID(mln_acquired_frame_release(frame, NULL, NULL));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);
  refused = sync;
  refused.kind = 999;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_acquired_frame_release(frame, &refused, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "gpu sync kind"));
  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  MLN_TEST_OK(
    mln_render_session_get_capabilities(fixture.session, &capabilities, NULL)
  );
  TEST_ASSERT_BITS_HIGH(
    MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE, capabilities.consumer_sync_kinds
  );
  refused.kind =
    (capabilities.consumer_sync_kinds & MLN_GPU_SYNC_KIND_FLAG_QUEUE_ORDER) == 0
      ? MLN_GPU_SYNC_QUEUE_ORDER
      : MLN_GPU_SYNC_OPEN_GL_FENCE;
  refused.handle = 1;
  TEST_ASSERT_BITS_LOW(1U << refused.kind, capabilities.consumer_sync_kinds);
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_acquired_frame_release(frame, &refused, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);
  mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
  MLN_TEST_OK(mln_acquired_frame_get_result(frame, &result, NULL));
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_test_render_release_frame(&frame);

  released_frame = MLN_HANDLE_NULL;
  detach(runtime, map, &fixture);
}

// Abandon right after the host acquires a frame succeeds even though a core
// worker may still be inside the call that rendered it. The ring's textures
// are the host's, so abandon destroys a Vulkan or Metal backend and renderer,
// whose objects the host never reads, and keeps an OpenGL or WebGPU one,
// which only its graphics thread may destroy.
static void an_acquired_frame_still_releases_after_abandon(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture, mln_test_empty_style_json);
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 250);

  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  MLN_TEST_OK(mln_render_session_abandon(fixture.session, &abandoned, NULL));
#if defined(MLN_FFI_TEST_BACKEND_VULKAN) || defined(MLN_FFI_TEST_BACKEND_METAL)
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_ABANDON_DISPOSITION_CLEAN, abandoned.disposition
  );
  TEST_ASSERT_EQUAL_UINT32(0, abandoned.quarantined_resource_count);
#else
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED, abandoned.disposition
  );
  TEST_ASSERT_EQUAL_UINT32(2, abandoned.quarantined_resource_count);
#endif
  // The frame keeps its own result, but the session no longer backs the
  // objects that it lends.
  mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
  MLN_TEST_OK(mln_acquired_frame_get_result(frame, &result, NULL));
  mln_gpu_sync sync = {.size = sizeof(mln_gpu_sync)};
  MLN_TEST_STATUS(
    MLN_STATUS_TARGET_LOST,
    mln_acquired_frame_get_producer_sync(frame, &sync, NULL)
  );
  mln_test_render_release_frame(&frame);
  detach(runtime, map, &fixture);
}

// A release while detach is in flight finds the frame already released
// before it looks at the sync kind, which would read the session's backend
// that the detach may be destroying.
static void a_stale_release_during_detach_is_refused_before_its_sync(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture, mln_test_empty_style_json);
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 170);
  const mln_acquired_frame stale = frame;
  mln_test_render_release_frame(&frame);

  mln_test_completion detached = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln_render_session_detach(fixture.session, &detached.descriptor, NULL)
  );
  mln_gpu_sync unsupported = {.size = sizeof(mln_gpu_sync)};
  unsupported.kind = 999;
  MLN_TEST_INVALID_STATE(mln_acquired_frame_release(stale, &unsupported, NULL));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &detached));
  mln_test_completion_destroy(&detached);
  detach(runtime, map, &fixture);
}

static void borrowed_views_hold_a_frame_until_every_view_ends(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture, mln_test_empty_style_json);
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 1);

  MLN_TEST_INVALID(
    mln_acquired_frame_view_begin(frame, NULL, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "out_scope must not be null"),
    mln_test_last_error()
  );
  void* scopes[2] = {NULL, NULL};
  for (size_t index = 0; index < 2; index += 1) {
    MLN_TEST_OK(mln_acquired_frame_view_begin(frame, &scopes[index], NULL));
    TEST_ASSERT_NOT_NULL(scopes[index]);
  }
  const mln_gpu_sync sync = {
    .size = sizeof(mln_gpu_sync), .kind = MLN_GPU_SYNC_CPU_COMPLETE
  };
  for (size_t index = 0; index < 2; index += 1) {
    const mln_acquired_frame held = frame;
    MLN_TEST_STATUS(
      MLN_STATUS_BUSY, mln_acquired_frame_release(frame, &sync, NULL)
    );
    TEST_ASSERT_EQUAL_UINT64(held, frame);
    mln_acquired_frame_view_end(scopes[index]);
  }
  mln_acquired_frame_view_end(NULL);

  const mln_acquired_frame released = frame;
  mln_test_render_release_frame(&frame);
  void* stale = NULL;
  MLN_TEST_INVALID_STATE(mln_acquired_frame_view_begin(released, &stale, NULL));
  TEST_ASSERT_NULL(stale);
  detach(runtime, map, &fixture);
}

// A view keeps its frame's texture and sync objects in place, which an abandon
// would let go of.
static void an_open_view_holds_an_abandon_back(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture, mln_test_empty_style_json);
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 1);
  void* scope = NULL;
  MLN_TEST_OK(mln_acquired_frame_view_begin(frame, &scope, NULL));

  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  MLN_TEST_STATUS(
    MLN_STATUS_BUSY,
    mln_render_session_abandon(fixture.session, &abandoned, NULL)
  );
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture.session, &snapshot, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_ATTACHED, snapshot.state);
  mln_gpu_sync sync = {.size = sizeof(mln_gpu_sync)};
  MLN_TEST_OK(mln_acquired_frame_get_producer_sync(frame, &sync, NULL));

  mln_acquired_frame_view_end(scope);
  MLN_TEST_OK(mln_render_session_abandon(fixture.session, &abandoned, NULL));
  mln_test_render_release_frame(&frame);
  detach(runtime, map, &fixture);
}

// A frame wake that runs on the session's thread, as one without a delivery
// queue does, opens a view while another thread abandons the session.
typedef struct view_during_abandon {
  mln_render_session session;
  mln_acquired_frame frame;
  atomic_bool armed;
  atomic_bool entered;
  atomic_bool done;
  mln_status view_status;
} view_during_abandon;

static bool session_abandoned(void* context) {
  const view_during_abandon* probe = context;
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  return mln_render_session_get_snapshot(probe->session, &snapshot, NULL) ==
           MLN_STATUS_OK &&
         snapshot.state == MLN_RENDER_SESSION_STATE_ABANDONED;
}

static void open_a_view_once_abandoned(void* user_data) {
  view_during_abandon* probe = user_data;
  mln_test_pulse();
  bool expected = true;
  if (!atomic_compare_exchange_strong(&probe->armed, &expected, false)) return;
  mln_test_flag_set(&probe->entered);
  // The abandon has begun once the session reports it, and waits for this
  // thread to let go of the session's graphics objects.
  if (
    mln_test_await(
      session_abandoned, probe, mln_test_deadline_default(),
      "an abandon from another thread"
    )
  ) {
    void* scope = NULL;
    probe->view_status =
      mln_acquired_frame_view_begin(probe->frame, &scope, NULL);
    mln_acquired_frame_view_end(scope);
  }
  mln_test_flag_set(&probe->done);
}

static void a_view_opened_during_an_abandon_finds_the_session_gone(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  view_during_abandon probe = {.view_status = MLN_STATUS_OK};
  atomic_init(&probe.armed, false);
  atomic_init(&probe.entered, false);
  atomic_init(&probe.done, false);
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.frame_wake =
    (mln_wake){.callback = open_a_view_once_abandoned, .user_data = &probe};
  options.driver_work_wake = mln_test_pulse_wake();
  mln_test_render_fixture fixture = {0};
  mln_test_completion attached = mln_test_completion_default(0);
  MLN_TEST_OK(mln_test_render_fixture_start_attach(
    map, &options, &attached.descriptor, &fixture
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &attached));
  mln_test_completion_destroy(&attached);
  probe.session = fixture.session;
  probe.frame = mln_test_render_and_acquire(&fixture, 1);
  // A caller driver runs the wake on this thread.
  if (fixture.driver != MLN_RENDER_DRIVER_CORE_WORKER) {
    mln_test_render_release_frame(&probe.frame);
    detach(runtime, map, &fixture);
    return;
  }

  atomic_store(&probe.armed, true);
  mln_test_render_demand_forced(&fixture, 2);
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.entered));
  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  MLN_TEST_OK(mln_render_session_abandon(fixture.session, &abandoned, NULL));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.done));
  MLN_TEST_STATUS(MLN_STATUS_TARGET_LOST, probe.view_status);

  mln_test_render_release_frame(&probe.frame);
  detach(runtime, map, &fixture);
}

// Waits for earlier driver work and verifies that parked demands have no
// result.
static void expect_parked_demand(const mln_test_render_fixture* fixture) {
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_reduce_memory_use(
      fixture->session, &completion.descriptor, NULL
    )
  );
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_drain_frame_results(fixture->session, &batch, NULL));
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, batch);
}

// A binding's finalizer disposes a frame in place of a synchronized release.
static void disposing_a_frame_quarantines_only_its_slot(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture, mln_test_empty_style_json);
  mln_acquired_frame kept = mln_test_render_and_acquire(&fixture, 1);
  const mln_acquired_frame disposed = mln_test_render_and_acquire(&fixture, 2);
  void* open = NULL;
  MLN_TEST_OK(mln_acquired_frame_view_begin(disposed, &open, NULL));

  MLN_TEST_OK(mln_acquired_frame_discard(disposed, MLN_TEST_DIAGNOSTIC));
  MLN_TEST_INVALID_STATE(mln_acquired_frame_discard(disposed, NULL));
  void* stale = NULL;
  MLN_TEST_INVALID_STATE(mln_acquired_frame_view_begin(disposed, &stale, NULL));
  TEST_ASSERT_NULL(stale);
  void* sibling = NULL;
  MLN_TEST_OK(mln_acquired_frame_view_begin(kept, &sibling, NULL));
  mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
  MLN_TEST_OK(mln_acquired_frame_get_result(kept, &result, NULL));
  TEST_ASSERT_EQUAL_UINT64(1, result.token);
  mln_acquired_frame_view_end(sibling);
  mln_acquired_frame_view_end(open);

  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture.session, &snapshot, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_ATTACHED, snapshot.state);
  TEST_ASSERT_EQUAL_UINT32(1, snapshot.acquired_frame_count);
  mln_test_render_release_frame(&kept);

  // Holding the frame in the one usable slot leaves the next demand nowhere
  // to render, even though the quarantined slot holds no frame.
  mln_acquired_frame held = mln_test_render_and_acquire(&fixture, 3);
  mln_test_render_demand_forced(&fixture, 4);
  expect_parked_demand(&fixture);
  mln_test_render_release_frame(&held);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(&fixture);
  result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(4, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  detach_keeping_the_ring(runtime, map, &fixture);
}

static void a_fully_quarantined_ring_takes_no_more_demands(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture, mln_test_empty_style_json);
  const mln_acquired_frame first = mln_test_render_and_acquire(&fixture, 1);
  const mln_acquired_frame second = mln_test_render_and_acquire(&fixture, 2);
  // Forced demands remain independent, so both park.
  mln_test_render_demand_forced(&fixture, 3);
  mln_test_render_demand_forced(&fixture, 4);
  expect_parked_demand(&fixture);

  MLN_TEST_OK(mln_acquired_frame_discard(first, NULL));
  MLN_TEST_OK(mln_acquired_frame_discard(second, NULL));
  mln_render_frame_result results[2];
  mln_test_render_wait_for_result_list(&fixture, 2, results);
  for (size_t index = 0; index < 2; index += 1) {
    TEST_ASSERT_EQUAL_UINT64(3 + index, results[index].token);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_TARGET_NOT_READY, results[index].disposition
    );
  }

  mln_frame_demand demand = {.size = sizeof(mln_frame_demand)};
  demand.token = 5;
  MLN_TEST_INVALID_STATE(mln_render_session_demand_frame(
    fixture.session, &demand, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "quarantined"), mln_test_last_error()
  );
  detach_keeping_the_ring(runtime, map, &fixture);
}

static void render_forced(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_test_render_demand_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT64(token, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_render_frame_batch_release(batch);
}

// Only an offscreen session resizes its texture; a borrowed ring takes new
// textures instead.
static void a_resize_renders_and_reads_back_at_the_new_size(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  render_forced(&fixture, 301);

  const mln_logical_size resized = {.width = 48, .height = 24};
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_resize(
      fixture.session, mln_test_session_extent(fixture.session, resized),
      &completion.descriptor, NULL
    )
  );
  render_forced(&fixture, 302);
  mln_texture_image_info info = {0};
  MLN_TEST_OK(mln_test_render_read_back(&fixture, &info, NULL, 0));
  TEST_ASSERT_EQUAL_UINT32(48, info.width);
  TEST_ASSERT_EQUAL_UINT32(24, info.height);
  detach(runtime, map, &fixture);
}

typedef struct physical_row {
  const char* label;
  mln_logical_extent extent;
  mln_status expected;
  uint32_t width;
  uint32_t height;
} physical_row;

#define EXTENT(w, h, scale) \
  ((mln_logical_extent){.width = (w), .height = (h), .scale_factor = (scale)})

static void physical_sizes_round_up_and_reject_overflow(void) {
  const physical_row rows[] = {
    {"a fractional product rounds up", EXTENT(3, 5, 1.5), MLN_STATUS_OK, 5, 8},
    {"a scale below one keeps a pixel", EXTENT(1, 3, 0.25), MLN_STATUS_OK, 1,
     1},
    {"the largest dimension fits", EXTENT(UINT32_MAX, 1, 1.0), MLN_STATUS_OK,
     UINT32_MAX, 1},
    {"a product past the limit overflows", EXTENT(UINT32_C(0x80000000), 1, 2.0),
     MLN_STATUS_INVALID_ARGUMENT, 0, 0},
    {"a scale that is not a number", EXTENT(1, 1, (double)NAN),
     MLN_STATUS_INVALID_ARGUMENT, 0, 0},
  };
  for (size_t index = 0; index < sizeof(rows) / sizeof(rows[0]); index += 1) {
    const physical_row* row = &rows[index];
    uint32_t width = 7;
    uint32_t height = 7;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      row->expected,
      mln_logical_extent_physical_size(row->extent, &width, &height, NULL),
      row->label
    );
    // A failed call leaves the outputs alone.
    const bool ok = row->expected == MLN_STATUS_OK;
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(ok ? row->width : 7, width, row->label);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(ok ? row->height : 7, height, row->label);
  }
  uint32_t height = 0;
  MLN_TEST_INVALID(
    mln_logical_extent_physical_size(EXTENT(1, 1, 1.0), NULL, &height, NULL)
  );
}

// Renders one forced frame and returns its result, releasing any frame the
// ring holds so it never fills.
static mln_render_frame_result render_one(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_test_render_demand_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  while (mln_render_session_acquire_frame(fixture->session, &frame, NULL) ==
           MLN_STATUS_OK &&
         frame != MLN_HANDLE_NULL) {
    mln_test_render_release_frame(&frame);
  }
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  return result;
}

static void set_zoom(mln_map map, uint32_t mode, double zoom) {
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.mode = mode;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = zoom;
  // Far longer than any run, so an ease is still going at every frame below
  // until the jump ends it.
  update.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  update.animation.duration_ms = 3600000;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

// Renders one forced frame and waits until the map has handled it. The map
// queues the frame-finished event while it handles the frame on the runtime
// worker, so a map command submitted once the event arrives runs after that
// handling, and after any update the handling published. A fence submitted
// before the event would not do: it waits only for earlier submissions, not
// for the worker.
static mln_render_frame_result render_and_settle(
  const mln_test_render_fixture* fixture, mln_runtime runtime, mln_map map,
  uint64_t token
) {
  const mln_render_frame_result result = render_one(fixture, token);
  mln_runtime_event event = {0};
  TEST_ASSERT_TRUE(mln_test_await_event(
    runtime, MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED, map, &event, NULL, 0
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_event_mask(
    map, MLN_RUNTIME_EVENT_FLAG_ALL, &completion.descriptor, NULL
  ));
  return result;
}

// While a camera transition runs, the map publishes a new update after every
// frame, which is how a render-if-needed host learns to render again. The
// camera alone leaves needs_repaint unset: it reports the renderer's own
// transitions, which this style makes instant, placement fades included. A
// jump ends the camera transition, and the map stops publishing.
static void a_camera_transition_publishes_an_update_after_every_frame(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(
    &runtime, &map, &fixture,
    MLN_BUFFER_LITERAL(
      "{\"version\":8,\"sources\":{},\"layers\":[],"
      "\"transition\":{\"duration\":0,\"delay\":0}}"
    )
  );
  set_zoom(map, MLN_CAMERA_UPDATE_MODE_EASE, 8.0);
  mln_render_frame_result previous =
    render_and_settle(&fixture, runtime, map, 1);
  for (uint64_t token = 2; token <= 4; token += 1) {
    const mln_render_frame_result result =
      render_and_settle(&fixture, runtime, map, token);
    TEST_ASSERT_GREATER_THAN_UINT64(
      previous.map_update_generation, result.map_update_generation
    );
    TEST_ASSERT_FALSE(result.needs_repaint);
    previous = result;
  }

  set_zoom(map, MLN_CAMERA_UPDATE_MODE_JUMP, 2.0);
  const mln_render_frame_result settled =
    render_and_settle(&fixture, runtime, map, 5);
  TEST_ASSERT_FALSE(settled.needs_repaint);
  TEST_ASSERT_EQUAL_UINT64(
    settled.map_update_generation, render_one(&fixture, 6).map_update_generation
  );
  detach(runtime, map, &fixture);
}

MLN_TEST_GROUP {
  RUN_TEST(texture_readback_copies_the_latest_rendered_frame);
  RUN_TEST(only_the_sessions_backend_describes_its_frame_texture);
  RUN_TEST(accessors_and_release_reject_a_broken_frame_or_record);
  RUN_TEST(an_acquired_frame_still_releases_after_abandon);
  RUN_TEST(a_stale_release_during_detach_is_refused_before_its_sync);
  RUN_TEST(borrowed_views_hold_a_frame_until_every_view_ends);
  RUN_TEST(an_open_view_holds_an_abandon_back);
  RUN_TEST(a_view_opened_during_an_abandon_finds_the_session_gone);
  RUN_TEST(disposing_a_frame_quarantines_only_its_slot);
  RUN_TEST(a_fully_quarantined_ring_takes_no_more_demands);
  RUN_TEST(a_resize_renders_and_reads_back_at_the_new_size);
  RUN_TEST(physical_sizes_round_up_and_reject_overflow);
  RUN_TEST(a_camera_transition_publishes_an_update_after_every_frame);
}
