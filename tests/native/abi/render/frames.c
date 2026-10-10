// Rendered frames: readback, the texture an acquired frame exposes through its
// backend's getter, the checks every accessor and release makes, borrowed
// views and disposal, the texture ring across a resize, physical sizes, and
// the updates a camera transition publishes.

#include <math.h>

#include "support/frames.h"

#include "support/style.h"
#include "support/test_support.h"

static void attach(
  mln_runtime* runtime, mln_map* map, mln_test_render_fixture* fixture,
  mln_buffer_view style
) {
  *runtime = mln_test_create_runtime();
  *map = mln_test_create_map(*runtime);
  mln_test_load_style_and_wait(*runtime, *map, style);
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(*map, fixture));
}

static void detach(
  mln_runtime runtime, mln_map map, mln_test_render_fixture* fixture
) {
  mln_test_render_fixture_destroy(fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A readback is accepted before any frame renders, and its completion reports
// that no frame is available. Once a frame renders, the readback is an ordered
// operation whose result owns a copy of the pixels.
static void texture_readback_copies_the_latest_rendered_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_test_completion readback = mln_test_completion_readback();
  MLN_TEST_OK(mln_texture_read_premultiplied_rgba8(
    fixture.session, &readback.descriptor, NULL
  ));
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_test_render_fixture_finish_operation(&fixture, &readback)
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_completion_diagnostic(&readback), "no rendered frame"),
    mln_test_completion_diagnostic(&readback)
  );
  mln_test_completion_destroy(&readback);

  mln_test_load_style_and_wait(
    runtime, map, mln_test_red_background_style_json
  );
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 260);
  mln_texture_image_info info = {0};
  uint8_t pixel[4] = {0};
  MLN_TEST_OK(mln_test_render_read_back(&fixture, &info, pixel, 4));
  TEST_ASSERT_EQUAL_UINT32(64, info.width);
  TEST_ASSERT_EQUAL_UINT32(64, info.height);
  const uint8_t red[4] = {255, 0, 0, 255};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(red, pixel, 4);
  mln_test_render_release_frame(&frame);
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
  mln_metal_owned_texture_frame metal;
  mln_vulkan_owned_texture_frame vulkan;
  mln_opengl_owned_texture_frame opengl;
  mln_webgpu_owned_texture_frame webgpu;
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
  return mln_acquired_frame_get_opengl_texture(
    frame, out == NULL ? NULL : &out->opengl, diagnostic
  );
}
static mln_status get_webgpu(
  mln_acquired_frame frame, frame_record* out, mln_diagnostic* diagnostic
) {
  return mln_acquired_frame_get_webgpu_texture(
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

// The texture getters come first, indexed by frame_backend.
static const accessor_entry accessors[] = {
  {"metal texture", get_metal, sizeof(mln_metal_owned_texture_frame),
   PRESET_FRAME_BACKEND == METAL},
  {"vulkan texture", get_vulkan, sizeof(mln_vulkan_owned_texture_frame),
   PRESET_FRAME_BACKEND == VULKAN},
  {"opengl texture", get_opengl, sizeof(mln_opengl_owned_texture_frame),
   PRESET_FRAME_BACKEND == OPENGL},
  {"webgpu texture", get_webgpu, sizeof(mln_webgpu_owned_texture_frame),
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
// session's extent and generation. Returns the frame's ID.
static uint64_t expect_preset_texture(
  const frame_record* record, uint64_t generation
) {
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  const mln_metal_owned_texture_frame* frame = &record->metal;
  TEST_ASSERT_NOT_NULL(frame->texture);
  TEST_ASSERT_NOT_NULL(frame->device);
  TEST_ASSERT_NOT_EQUAL_UINT64(0, frame->pixel_format);
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  const mln_vulkan_owned_texture_frame* frame = &record->vulkan;
  TEST_ASSERT_NOT_EQUAL_UINT64(0, frame->image);
  TEST_ASSERT_NOT_EQUAL_UINT64(0, frame->image_view);
  TEST_ASSERT_NOT_NULL(frame->device);
  TEST_ASSERT_NOT_EQUAL_UINT32(0, frame->format);
#elif defined(MLN_FFI_TEST_BACKEND_OPENGL)
  const mln_opengl_owned_texture_frame* frame = &record->opengl;
  TEST_ASSERT_NOT_EQUAL_UINT32(0, frame->texture);
  // GL_TEXTURE_2D.
  TEST_ASSERT_EQUAL_HEX32(0x0DE1, frame->target);
  TEST_ASSERT_NOT_EQUAL_UINT32(0, frame->internal_format);
#else
  const mln_webgpu_owned_texture_frame* frame = &record->webgpu;
  TEST_ASSERT_NOT_NULL(frame->texture);
  TEST_ASSERT_NOT_NULL(frame->texture_view);
  TEST_ASSERT_NOT_NULL(frame->device);
  TEST_ASSERT_NOT_EQUAL_UINT32(0, frame->format);
#endif
  TEST_ASSERT_EQUAL_UINT64(generation, frame->generation);
  TEST_ASSERT_EQUAL_UINT32(64, frame->width);
  TEST_ASSERT_EQUAL_UINT32(64, frame->height);
  TEST_ASSERT_EQUAL_DOUBLE(1.0, frame->scale_factor);
  TEST_ASSERT_NOT_EQUAL_UINT64(0, frame->frame_id);
  return frame->frame_id;
}

// The getter of the session's backend describes the frame's texture, and
// every other backend's refuses. Each acquisition names a frame of its own.
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

  uint64_t first_frame_id = 0;
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
      TEST_ASSERT_EQUAL_UINT64(
        snapshot.extent_generation, record.result.extent_generation
      );
      TEST_ASSERT_EQUAL_UINT64(
        snapshot.frame_generation, record.result.frame_generation
      );
    } else if (accessor->call != get_producer_sync) {
      first_frame_id = expect_preset_texture(&record, snapshot.generation);
    }
  }
  mln_test_render_release_frame(&frame);

  frame = mln_test_render_and_acquire(&fixture, 2);
  frame_record record;
  MLN_TEST_OK(
    call_with_record(&accessors[PRESET_FRAME_BACKEND], frame, &record, NULL)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(
    first_frame_id, expect_preset_texture(&record, snapshot.generation)
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

// A broken frame or output fails before an accessor looks at the frame's
// backend, so every accessor reports it the same way: a released frame as
// invalid state, and anything else as an invalid argument. Release takes a
// live frame and a sync record it knows, and consumes the frame only when it
// succeeds, so a refused release leaves the frame with the host.
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
  } breakages[] = {
    {"null frame", null_frame, MLN_STATUS_INVALID_ARGUMENT},
    {"released frame", a_released_frame, MLN_STATUS_INVALID_STATE},
    {"null output", null_record, MLN_STATUS_INVALID_ARGUMENT},
    {"undersized output", undersized_record, MLN_STATUS_INVALID_ARGUMENT},
  };
  enum { breakage_count = sizeof(breakages) / sizeof(breakages[0]) };
  for (size_t index = 0; index < accessor_count; index += 1) {
    char labels[breakage_count][64];
    mln_test_validation_case cases[breakage_count];
    for (size_t row = 0; row < breakage_count; row += 1) {
      (void)snprintf(
        labels[row], sizeof(labels[row]), "%s: %s", accessors[index].name,
        breakages[row].label
      );
      cases[row] = (mln_test_validation_case){
        labels[row], breakages[row].mutate, breakages[row].expected, NULL
      };
    }
    const accessor_call defaults = {
      .frame = frame, .record_size = accessors[index].record_size
    };
    mln_test_run_validation_table(
      cases, breakage_count, &defaults, sizeof(defaults), call_accessor,
      (void*)&accessors[index]
    );
  }

  const mln_gpu_sync sync = mln_gpu_sync_default();
  mln_acquired_frame null_handle = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_acquired_frame_release(&null_handle, &sync, NULL));
  MLN_TEST_INVALID(mln_acquired_frame_release(NULL, &sync, NULL));
  mln_acquired_frame stale_copy = stale;
  MLN_TEST_INVALID_STATE(mln_acquired_frame_release(&stale_copy, &sync, NULL));
  mln_gpu_sync refused = sync;
  refused.size = sizeof(mln_gpu_sync) - 1;
  MLN_TEST_INVALID(mln_acquired_frame_release(&frame, &refused, NULL));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);
  refused = sync;
  refused.kind = 999;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_acquired_frame_release(&frame, &refused, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "gpu sync kind"));
  // The host still owns the frame and its accessors still answer.
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);
  mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
  MLN_TEST_OK(mln_acquired_frame_get_result(frame, &result, NULL));
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_test_render_release_frame(&frame);

  released_frame = MLN_HANDLE_NULL;
  detach(runtime, map, &fixture);
}

// Abandon right after the host acquires a frame succeeds even though a core
// worker may still be inside the call that rendered it. The host's GPU may
// still read the frame's texture, so abandon keeps the ring's backend and the
// renderer that draws through it. The frame then reports target loss, and
// releasing it is CPU-only.
static void acquired_frame_release_after_abandon_is_cpu_only(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture, mln_test_empty_style_json);
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 250);

  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  MLN_TEST_OK(mln_render_session_abandon(fixture.session, &abandoned, NULL));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED, abandoned.disposition
  );
  TEST_ASSERT_EQUAL_UINT32(2, abandoned.quarantined_resource_count);
  mln_render_frame_result invalid = {.size = sizeof(mln_render_frame_result)};
  MLN_TEST_STATUS(
    MLN_STATUS_TARGET_LOST, mln_acquired_frame_get_result(frame, &invalid, NULL)
  );
  mln_test_render_release_frame(&frame);
  detach(runtime, map, &fixture);
}

// Each scope holds the frame: an explicit release reports busy, and leaves
// the caller the handle, until every scope has ended.
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
  mln_gpu_sync sync = mln_gpu_sync_default();
  for (size_t index = 0; index < 2; index += 1) {
    const mln_acquired_frame held = frame;
    MLN_TEST_STATUS(
      MLN_STATUS_BUSY, mln_acquired_frame_release(&frame, &sync, NULL)
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

// Waits until the driver has run every work item that it already holds, then
// counts the demands that it parked behind a full texture ring.
static uint32_t parked_demand_count(const mln_test_render_fixture* fixture) {
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_reduce_memory_use(
      fixture->session, &completion.descriptor, NULL
    )
  );
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture->session, &snapshot, NULL)
  );
  return snapshot.pending_demand_count;
}

// Disposing a frame, which a binding's finalizer uses in place of a
// synchronized release, consumes it and quarantines its slot of the ring. The
// session stays attached: a view already open on the disposed frame reads
// until it ends, the other frame stays readable, and later frames render only
// into the remaining slot. Detach still completes, keeping the quarantined
// texture.
static void disposing_a_frame_quarantines_only_its_slot(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture, mln_test_empty_style_json);
  mln_acquired_frame kept = mln_test_render_and_acquire(&fixture, 1);
  const mln_acquired_frame disposed = mln_test_render_and_acquire(&fixture, 2);
  void* open = NULL;
  MLN_TEST_OK(mln_acquired_frame_view_begin(disposed, &open, NULL));

  MLN_TEST_OK(mln_acquired_frame_dispose(disposed, MLN_TEST_DIAGNOSTIC));
  MLN_TEST_INVALID_STATE(mln_acquired_frame_dispose(disposed, NULL));
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
  mln_test_render_request_forced(&fixture, 4);
  TEST_ASSERT_EQUAL_UINT32(1, parked_demand_count(&fixture));
  mln_test_render_release_frame(&held);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(&fixture, 1);
  result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(4, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  detach(runtime, map, &fixture);
}

// Once disposed frames quarantine every slot, no frame can render again. Every
// demand parked behind the full ring resolves as target-not-ready, frame
// requests fail with an invalid-state status, and detach still completes.
static void a_fully_quarantined_ring_takes_no_more_demands(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture, mln_test_empty_style_json);
  const mln_acquired_frame first = mln_test_render_and_acquire(&fixture, 1);
  const mln_acquired_frame second = mln_test_render_and_acquire(&fixture, 2);
  // Each demand has its own coalescing boundary, so neither supersedes the
  // other and both park.
  mln_test_render_request_forced(&fixture, 3);
  mln_test_render_request_forced(&fixture, 4);
  TEST_ASSERT_EQUAL_UINT32(2, parked_demand_count(&fixture));

  MLN_TEST_OK(mln_acquired_frame_dispose(first, NULL));
  MLN_TEST_OK(mln_acquired_frame_dispose(second, NULL));
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(&fixture, 2);
  for (size_t index = 0; index < 2; index += 1) {
    const mln_render_frame_result result =
      mln_test_render_batch_result(batch, index);
    TEST_ASSERT_EQUAL_UINT64(3 + index, result.token);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_TARGET_NOT_READY, result.disposition
    );
  }
  mln_render_frame_batch_release(batch);

  mln_frame_demand demand = mln_frame_demand_default();
  demand.token = 5;
  MLN_TEST_INVALID_STATE(mln_render_session_request_frame(
    fixture.session, &demand, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "quarantined"), mln_test_last_error()
  );
  detach(runtime, map, &fixture);
}

// With both slots of the two-deep ring holding textures of the old size, a
// resize retires each of them, so the next frame renders at the new size
// whichever slot it lands in. Holding the first frame while the second renders
// is what puts a texture in the second slot.
static void a_resize_retires_every_old_size_slot(void) {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture fixture = {0};
  attach(&runtime, &map, &fixture, mln_test_empty_style_json);
  mln_acquired_frame held = mln_test_render_and_acquire(&fixture, 301);
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 302);
  mln_test_render_release_frame(&frame);
  mln_test_render_release_frame(&held);

  const mln_render_target_extent resized = {
    .size = sizeof(mln_render_target_extent),
    .width = 48,
    .height = 24,
    .scale_factor = 1.0,
  };
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_resize(
      fixture.session, &resized, &completion.descriptor, NULL
    )
  );
  for (uint64_t token = 303; token <= 304; token += 1) {
    frame = mln_test_render_and_acquire(&fixture, token);
    mln_test_render_release_frame(&frame);
  }
  mln_texture_image_info info = {0};
  MLN_TEST_OK(mln_test_render_read_back(&fixture, &info, NULL, 0));
  TEST_ASSERT_EQUAL_UINT32(48, info.width);
  TEST_ASSERT_EQUAL_UINT32(24, info.height);
  detach(runtime, map, &fixture);
}

typedef struct physical_row {
  const char* label;
  mln_render_target_extent extent;
  mln_status expected;
  uint32_t width;
  uint32_t height;
} physical_row;

#define EXTENT(w, h, scale)                   \
  ((mln_render_target_extent){                \
    .size = sizeof(mln_render_target_extent), \
    .width = (w),                             \
    .height = (h),                            \
    .scale_factor = (scale)                   \
  })

// Each physical dimension is the logical one times the scale factor, rounded
// up, and must fit in 32 bits.
static void physical_sizes_round_up_and_reject_overflow(void) {
  const physical_row rows[] = {
    {"a unit scale keeps the size", EXTENT(64, 32, 1.0), MLN_STATUS_OK, 64, 32},
    {"a fractional product rounds up", EXTENT(3, 5, 1.5), MLN_STATUS_OK, 5, 8},
    {"a scale below one keeps a pixel", EXTENT(1, 3, 0.25), MLN_STATUS_OK, 1,
     1},
    {"an exact product is not rounded", EXTENT(10, 20, 1.5), MLN_STATUS_OK, 15,
     30},
    {"the largest dimension fits", EXTENT(UINT32_MAX, 1, 1.0), MLN_STATUS_OK,
     UINT32_MAX, 1},
    {"a product just under the limit fits",
     EXTENT(UINT32_C(0x7fffffff), 1, 2.0), MLN_STATUS_OK, UINT32_C(0xfffffffe),
     2},
    {"a product past the limit overflows", EXTENT(UINT32_C(0x80000000), 1, 2.0),
     MLN_STATUS_INVALID_ARGUMENT, 0, 0},
    {"a height past the limit overflows", EXTENT(1, UINT32_MAX, 1.5),
     MLN_STATUS_INVALID_ARGUMENT, 0, 0},
    {"a zero width", EXTENT(0, 1, 1.0), MLN_STATUS_INVALID_ARGUMENT, 0, 0},
    {"a zero scale", EXTENT(1, 1, 0.0), MLN_STATUS_INVALID_ARGUMENT, 0, 0},
    {"a negative scale", EXTENT(1, 1, -1.0), MLN_STATUS_INVALID_ARGUMENT, 0, 0},
    {"a scale that is not a number", EXTENT(1, 1, (double)NAN),
     MLN_STATUS_INVALID_ARGUMENT, 0, 0},
    {"an undersized extent",
     {.size = sizeof(mln_render_target_extent) - 1,
      .width = 1,
      .height = 1,
      .scale_factor = 1.0},
     MLN_STATUS_INVALID_ARGUMENT,
     0,
     0},
  };
  for (size_t index = 0; index < sizeof(rows) / sizeof(rows[0]); index += 1) {
    const physical_row* row = &rows[index];
    uint32_t width = 7;
    uint32_t height = 7;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      row->expected,
      mln_render_target_extent_physical_size(
        &row->extent, &width, &height, NULL
      ),
      row->label
    );
    // A failed call leaves the outputs alone.
    const bool ok = row->expected == MLN_STATUS_OK;
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(ok ? row->width : 7, width, row->label);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(ok ? row->height : 7, height, row->label);
  }
  uint32_t width = 0;
  uint32_t height = 0;
  const mln_render_target_extent extent = EXTENT(1, 1, 1.0);
  MLN_TEST_INVALID(
    mln_render_target_extent_physical_size(NULL, &width, &height, NULL)
  );
  MLN_TEST_INVALID(
    mln_render_target_extent_physical_size(&extent, NULL, &height, NULL)
  );
  MLN_TEST_INVALID(
    mln_render_target_extent_physical_size(&extent, &width, NULL, NULL)
  );
}

// Renders one forced frame and returns its result, releasing any frame the
// ring holds so it never fills.
static mln_render_frame_result render_one(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_test_render_request_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture, 1);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  while (mln_render_session_acquire_frame(fixture->session, &frame, NULL) ==
         MLN_STATUS_OK) {
    MLN_TEST_OK(mln_acquired_frame_release(&frame, NULL, NULL));
    frame = MLN_HANDLE_NULL;
  }
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  return result;
}

static void set_zoom(mln_map map, uint32_t mode, double zoom) {
  mln_camera_update update = mln_camera_update_default();
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
// handling, and after any update the handling published. A runtime barrier
// would not do: it waits only for earlier submissions, not for the worker.
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
    map, MLN_RUNTIME_EVENT_MASK_ALL, &completion.descriptor, NULL
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
  RUN_TEST(acquired_frame_release_after_abandon_is_cpu_only);
  RUN_TEST(borrowed_views_hold_a_frame_until_every_view_ends);
  RUN_TEST(disposing_a_frame_quarantines_only_its_slot);
  RUN_TEST(a_fully_quarantined_ring_takes_no_more_demands);
  RUN_TEST(a_resize_retires_every_old_size_slot);
  RUN_TEST(physical_sizes_round_up_and_reject_overflow);
  RUN_TEST(a_camera_transition_publishes_an_update_after_every_frame);
}
