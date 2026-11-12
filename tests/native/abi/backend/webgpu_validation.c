// WebGPU rings: what attach and set_target reject before they reach a device.
// Every build validates a WebGPU ring before it reports that it renders into
// none, so these tables run on every preset.

#include "support/attach_table.h"
#include "support/test_support.h"

// Any value but WGPUTextureFormat_Undefined: validation rejects that one and
// takes the rest as given.
enum { some_texture_format = 18 };

MLN_TEST_BORROWED_EDITS(web_gpu_borrowed_textures, webgpu)

static void ring_without_texture_view(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.webgpu[0].texture_view = NULL;
  edited->descriptor.target.data.web_gpu_borrowed_textures.textures =
    edited->textures.webgpu;
}
static void ring_without_format(void* call) {
  ((mln_test_target_call*)call)
    ->descriptor.target.data.web_gpu_borrowed_textures.format = 0;
}

// A ring of one texture, whose entry is in the call's storage.
static void describe_ring(mln_test_target_call* call) {
  call->textures.webgpu[0] = (mln_web_gpu_borrowed_texture){
    .texture = MLN_TEST_FAKE_HANDLE,
    .texture_view = MLN_TEST_FAKE_HANDLE,
  };
  call->descriptor.target = (mln_render_target){
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_WEB_GPU_BORROWED_TEXTURES,
  };
  call->descriptor.target.data.web_gpu_borrowed_textures =
    (mln_web_gpu_borrowed_textures_target){
      .logical_size = mln_test_target_size(),
      .physical_width = 64,
      .physical_height = 64,
      .textures = call->textures.webgpu,
      .texture_count = 1,
      .format = some_texture_format,
    };
}

static void webgpu_attach_rejects_malformed_calls(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_target_map(runtime);

  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD);
  describe_ring(&call);
  static const mln_test_validation_case ring_rows[] = {
    MLN_TEST_BORROWED_CASES(web_gpu_borrowed_textures),
    {"null texture view", ring_without_texture_view,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"unspecified format", ring_without_format, MLN_STATUS_INVALID_ARGUMENT,
     "format"},
    {"a well-formed ring without a device", NULL, MLN_STATUS_UNSUPPORTED, NULL},
  };
  mln_test_run_attach_table(
    mln_test_submit_target_attach, &call, ring_rows,
    sizeof(ring_rows) / sizeof(ring_rows[0])
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// set_target checks its ring before the session, so these rows cover a row
// for each pointer and handle it reads, and its own steps.
static void webgpu_set_target_rejects_malformed_replacements(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CORE_WORKER);
  call.session = mln_test_create_disposed_session(map);
  describe_ring(&call);
  static const mln_test_validation_case ring_rows[] = {
    MLN_TEST_BORROWED_CASES(web_gpu_borrowed_textures),
    {"null texture view", ring_without_texture_view,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
  };
  mln_test_run_set_target_table(
    mln_test_submit_target_set_target, &call, ring_rows,
    sizeof(ring_rows) / sizeof(ring_rows[0])
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(webgpu_attach_rejects_malformed_calls);
  RUN_TEST(webgpu_set_target_rejects_malformed_replacements);
}
