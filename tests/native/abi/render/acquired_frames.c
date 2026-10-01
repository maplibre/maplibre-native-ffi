// Acquired frames: the texture a frame exposes through its backend's getter,
// which every other backend's getter refuses, and the checks every accessor
// makes of its frame and output.

#include <math.h>

#include "support/style.h"
#include "support/test_support.h"

enum frame_backend {
  FRAME_BACKEND_METAL,
  FRAME_BACKEND_VULKAN,
  FRAME_BACKEND_OPENGL,
  FRAME_BACKEND_WEBGPU,
};

#if defined(MLN_FFI_TEST_BACKEND_METAL)
#define PRESET_FRAME_BACKEND FRAME_BACKEND_METAL
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
#define PRESET_FRAME_BACKEND FRAME_BACKEND_VULKAN
#elif defined(MLN_FFI_TEST_BACKEND_OPENGL)
#define PRESET_FRAME_BACKEND FRAME_BACKEND_OPENGL
#else
#define PRESET_FRAME_BACKEND FRAME_BACKEND_WEBGPU
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

typedef mln_status (*frame_accessor)(
  mln_acquired_frame frame, frame_record* out, mln_diagnostic* diagnostic
);

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
  frame_accessor call;
  uint32_t record_size;
  // Whether a frame of this preset's backend answers it.
  bool answers;
} accessor_entry;

static const accessor_entry accessors[] = {
  {"metal texture", get_metal, sizeof(mln_metal_owned_texture_frame),
   PRESET_FRAME_BACKEND == FRAME_BACKEND_METAL},
  {"vulkan texture", get_vulkan, sizeof(mln_vulkan_owned_texture_frame),
   PRESET_FRAME_BACKEND == FRAME_BACKEND_VULKAN},
  {"opengl texture", get_opengl, sizeof(mln_opengl_owned_texture_frame),
   PRESET_FRAME_BACKEND == FRAME_BACKEND_OPENGL},
  {"webgpu texture", get_webgpu, sizeof(mln_webgpu_owned_texture_frame),
   PRESET_FRAME_BACKEND == FRAME_BACKEND_WEBGPU},
  {"result", get_result, sizeof(mln_render_frame_result), true},
  {"producer sync", get_producer_sync, sizeof(mln_gpu_sync), true},
};

static mln_acquired_frame render_and_acquire(
  const mln_test_render_fixture* fixture
) {
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, mln_test_style_render_frame(fixture)
  );
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_render_session_acquire_frame(fixture->session, &frame, NULL));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);
  return frame;
}

static void release_frame(mln_acquired_frame* frame) {
  const mln_gpu_sync sync = mln_gpu_sync_default();
  MLN_TEST_OK(mln_acquired_frame_release(frame, &sync, NULL));
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, *frame);
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

static void only_the_sessions_backend_describes_its_frame_texture(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_acquired_frame frame = render_and_acquire(&fixture);
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture.session, &snapshot, NULL)
  );

  uint64_t first_frame_id = 0;
  for (size_t index = 0; index < sizeof(accessors) / sizeof(accessors[0]);
       index += 1) {
    const accessor_entry* accessor = &accessors[index];
    frame_record record;
    memset(&record, 0, sizeof(record));
    record.size = accessor->record_size;
    const mln_status status =
      accessor->call(frame, &record, MLN_TEST_DIAGNOSTIC);
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
  release_frame(&frame);

  // Each acquisition names a frame of its own.
  frame = render_and_acquire(&fixture);
  const accessor_entry* preset = &accessors[PRESET_FRAME_BACKEND];
  frame_record record;
  memset(&record, 0, sizeof(record));
  record.size = preset->record_size;
  MLN_TEST_OK(preset->call(frame, &record, NULL));
  TEST_ASSERT_NOT_EQUAL_UINT64(
    first_frame_id, expect_preset_texture(&record, snapshot.generation)
  );
  release_frame(&frame);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
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
// backend, so every accessor reports it the same way.
static void accessors_reject_a_broken_frame_or_output(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  released_frame = render_and_acquire(&fixture);
  const mln_acquired_frame stale = released_frame;
  release_frame(&released_frame);
  released_frame = stale;
  mln_acquired_frame frame = render_and_acquire(&fixture);

  static const struct {
    const char* label;
    void (*mutate)(void* call);
  } breakages[] = {
    {"null frame", null_frame},
    {"released frame", a_released_frame},
    {"null output", null_record},
    {"undersized output", undersized_record},
  };
  enum { breakage_count = sizeof(breakages) / sizeof(breakages[0]) };
  for (size_t index = 0; index < sizeof(accessors) / sizeof(accessors[0]);
       index += 1) {
    char labels[breakage_count][64];
    mln_test_validation_case cases[breakage_count];
    for (size_t row = 0; row < breakage_count; row += 1) {
      (void)snprintf(
        labels[row], sizeof(labels[row]), "%s: %s", accessors[index].name,
        breakages[row].label
      );
      cases[row] = (mln_test_validation_case){
        labels[row], breakages[row].mutate, MLN_STATUS_INVALID_ARGUMENT, NULL
      };
    }
    const accessor_call defaults = {
      .frame = frame, .record_size = accessors[index].record_size
    };
    mln_test_run_validation_table(
      cases, sizeof(cases) / sizeof(cases[0]), &defaults, sizeof(defaults),
      call_accessor, (void*)&accessors[index]
    );
  }

  // Release takes a live frame and a sized sync record, and consumes the frame
  // only when it succeeds.
  const mln_gpu_sync sync = mln_gpu_sync_default();
  mln_acquired_frame null_handle = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_acquired_frame_release(&null_handle, &sync, NULL));
  MLN_TEST_INVALID(mln_acquired_frame_release(NULL, &sync, NULL));
  mln_acquired_frame stale_copy = stale;
  TEST_ASSERT_NOT_EQUAL_INT(
    MLN_STATUS_OK, mln_acquired_frame_release(&stale_copy, &sync, NULL)
  );
  mln_gpu_sync undersized = sync;
  undersized.size = sizeof(mln_gpu_sync) - 1;
  MLN_TEST_INVALID(mln_acquired_frame_release(&frame, &undersized, NULL));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);
  release_frame(&frame);

  released_frame = MLN_HANDLE_NULL;
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
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
    if (row->expected == MLN_STATUS_OK) {
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(row->width, width, row->label);
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(row->height, height, row->label);
    } else {
      // A failed call leaves the outputs alone.
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(7, width, row->label);
      TEST_ASSERT_EQUAL_UINT32_MESSAGE(7, height, row->label);
    }
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

MLN_TEST_GROUP {
  RUN_TEST(only_the_sessions_backend_describes_its_frame_texture);
  RUN_TEST(accessors_reject_a_broken_frame_or_output);
  RUN_TEST(physical_sizes_round_up_and_reject_overflow);
}
