// WebGPU target descriptors: what attach and set_target reject before they
// reach a device. Every build validates a WebGPU descriptor before it reports
// that it carries no WebGPU backend, so these tables run on every preset.
// Every backend's descriptors share the logical size and ring rules, so this
// file alone checks each of those rules in full.

#include "support/attach_table.h"
#include "support/test_support.h"

// Any value but WGPUTextureFormat_Undefined: validation rejects that one and
// takes the rest as given.
enum { some_texture_format = 18 };

MLN_TEST_DESCRIPTOR_EDITS(webgpu_surface, mln_web_gpu_surface_descriptor)
MLN_TEST_SIZE_RULE_EDITS(webgpu_surface)
MLN_TEST_OVERFLOW_EDIT(webgpu_surface)
MLN_TEST_DESCRIPTOR_EDITS(
  webgpu_borrowed, mln_web_gpu_borrowed_texture_descriptor
)
MLN_TEST_BORROWED_EDITS(webgpu_borrowed, webgpu)
MLN_TEST_RING_RULE_EDITS(webgpu_borrowed, webgpu)
MLN_TEST_DESCRIPTOR_EDITS(
  webgpu_surface_replacement, mln_web_gpu_surface_replacement
)
MLN_TEST_DESCRIPTOR_EDITS(
  webgpu_borrowed_replacement, mln_web_gpu_borrowed_texture_replacement
)
MLN_TEST_BORROWED_EDITS(webgpu_borrowed_replacement, webgpu)

static void surface_without_surface(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_surface.surface = NULL;
}
static void surface_without_format(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_surface.format = 0;
}
static void borrowed_without_device(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_borrowed.context.device =
    NULL;
}
static void borrowed_without_texture_view(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.webgpu[0].texture_view = NULL;
  edited->descriptor.webgpu_borrowed.textures = edited->textures.webgpu;
}
static void borrowed_without_format(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_borrowed.format = 0;
}
static void borrowed_without_physical_width(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_borrowed.physical_width = 0;
}
static void replacement_without_surface(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_surface_replacement.surface =
    NULL;
}
static void replacement_without_texture_view(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.webgpu[0].texture_view = NULL;
  edited->descriptor.webgpu_borrowed_replacement.textures =
    edited->textures.webgpu;
}
static void replacement_without_physical_width(void* call) {
  ((mln_test_target_call*)call)
    ->descriptor.webgpu_borrowed_replacement.physical_width = 0;
}

MLN_TEST_ATTACH_SUBMITTER(submit_surface_attach, mln_map_attach_web_gpu_surface)
MLN_TEST_ATTACH_SUBMITTER(
  submit_borrowed_attach, mln_map_attach_web_gpu_borrowed_texture
)
MLN_TEST_SET_TARGET_SUBMITTER(
  submit_surface_set_target, mln_render_session_set_web_gpu_surface_target
)
MLN_TEST_SET_TARGET_SUBMITTER(
  submit_borrowed_set_target,
  mln_render_session_set_web_gpu_borrowed_texture_target
)

static mln_web_gpu_surface_descriptor surface_descriptor(void) {
  mln_web_gpu_surface_descriptor descriptor = {
    .size = sizeof(mln_web_gpu_surface_descriptor)
  };
  descriptor.logical_size = mln_test_target_size();
  descriptor.context.device = MLN_TEST_FAKE_HANDLE;
  descriptor.surface = MLN_TEST_FAKE_HANDLE;
  descriptor.format = some_texture_format;
  return descriptor;
}

// A ring of one texture, whose entry is in the call's storage.
static void describe_borrowed(mln_test_target_call* call) {
  call->textures.webgpu[0] = (mln_web_gpu_borrowed_texture){
    .texture = MLN_TEST_FAKE_HANDLE,
    .texture_view = MLN_TEST_FAKE_HANDLE,
  };
  mln_web_gpu_borrowed_texture_descriptor* descriptor =
    &call->descriptor.webgpu_borrowed;
  *descriptor = (mln_web_gpu_borrowed_texture_descriptor){
    .size = sizeof(mln_web_gpu_borrowed_texture_descriptor)
  };
  descriptor->logical_size = mln_test_target_size();
  descriptor->context.device = MLN_TEST_FAKE_HANDLE;
  descriptor->textures = call->textures.webgpu;
  descriptor->texture_count = 1;
  descriptor->format = some_texture_format;
  descriptor->physical_width = 64;
  descriptor->physical_height = 64;
}

static void webgpu_attach_rejects_malformed_calls(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_target_map(runtime);

  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD);
  call.descriptor.webgpu_surface = surface_descriptor();
  // A surface with no format cannot be configured, so attach rejects it
  // rather than leaving the browser to report it.
  static const mln_test_validation_case surface_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(webgpu_surface),
    MLN_TEST_SIZE_RULE_CASES(webgpu_surface),
    MLN_TEST_OVERFLOW_CASE(webgpu_surface),
    {"null surface", surface_without_surface, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"unspecified format", surface_without_format, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
#if !defined(MLN_FFI_TEST_BACKEND_WEBGPU)
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_attach_table(
    submit_surface_attach, &call, surface_rows,
    sizeof(surface_rows) / sizeof(surface_rows[0])
  );

  describe_borrowed(&call);
  static const mln_test_validation_case borrowed_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(webgpu_borrowed),
    MLN_TEST_BORROWED_CASES(webgpu_borrowed),
    MLN_TEST_RING_RULE_CASES(webgpu_borrowed),
    {"null device", borrowed_without_device, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"null texture view", borrowed_without_texture_view,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"unspecified format", borrowed_without_format, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"zero physical width", borrowed_without_physical_width,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
#if !defined(MLN_FFI_TEST_BACKEND_WEBGPU)
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_attach_table(
    submit_borrowed_attach, &call, borrowed_rows,
    sizeof(borrowed_rows) / sizeof(borrowed_rows[0])
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// set_target checks its replacement, so these rows cover a row for each
// pointer and handle it reads, and its own steps.
static void webgpu_set_target_rejects_malformed_replacements(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD);
  call.session = mln_test_create_disposed_session(map);
  call.descriptor.webgpu_surface_replacement =
    (mln_web_gpu_surface_replacement){
      .size = sizeof(mln_web_gpu_surface_replacement),
      .logical_size = mln_test_target_size(),
      .surface = MLN_TEST_FAKE_HANDLE,
  };
  static const mln_test_validation_case surface_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(webgpu_surface_replacement),
    {"null surface", replacement_without_surface, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
  };
  mln_test_run_set_target_table(
    submit_surface_set_target, &call, surface_rows,
    sizeof(surface_rows) / sizeof(surface_rows[0]),
    MLN_RENDER_BACKEND_FLAG_WEB_GPU
  );

  call.textures.webgpu[0] = (mln_web_gpu_borrowed_texture){
    .texture = MLN_TEST_FAKE_HANDLE,
    .texture_view = MLN_TEST_FAKE_HANDLE,
  };
  call.descriptor.webgpu_borrowed_replacement =
    (mln_web_gpu_borrowed_texture_replacement){
      .size = sizeof(mln_web_gpu_borrowed_texture_replacement),
      .logical_size = mln_test_target_size(),
      .physical_width = 64,
      .physical_height = 64,
      .textures = call.textures.webgpu,
      .texture_count = 1,
  };
  static const mln_test_validation_case borrowed_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(webgpu_borrowed_replacement),
    MLN_TEST_BORROWED_CASES(webgpu_borrowed_replacement),
    {"null texture view", replacement_without_texture_view,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"zero physical width", replacement_without_physical_width,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
  };
  mln_test_run_set_target_table(
    submit_borrowed_set_target, &call, borrowed_rows,
    sizeof(borrowed_rows) / sizeof(borrowed_rows[0]),
    MLN_RENDER_BACKEND_FLAG_WEB_GPU
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(webgpu_attach_rejects_malformed_calls);
  RUN_TEST(webgpu_set_target_rejects_malformed_replacements);
}
