// Vulkan target descriptors: what attach and set_target reject before they
// reach a device. Every build validates a Vulkan descriptor before it reports
// that it carries no Vulkan backend, so these tables run on every preset.

#include "support/attach_table.h"
#include "support/host_graphics.h"
#include "support/test_support.h"

// Non-dispatchable handles with bits above the low 32: a carrier that
// truncated them to a 32-bit pointer would pass a different handle, and null
// for high_view_handle.
static const mln_vulkan_non_dispatchable_handle high_handle =
  UINT64_C(0x8000000000000001);
static const mln_vulkan_non_dispatchable_handle high_view_handle =
  UINT64_C(0x0000000100000000);

MLN_TEST_DESCRIPTOR_EDITS(
  vulkan_borrowed, mln_vulkan_borrowed_texture_descriptor
)
MLN_TEST_BORROWED_EDITS(vulkan_borrowed, vulkan)
MLN_TEST_DESCRIPTOR_EDITS(
  vulkan_borrowed_replacement, mln_vulkan_borrowed_texture_replacement
)
MLN_TEST_BORROWED_EDITS(vulkan_borrowed_replacement, vulkan)

static void borrowed_without_device(void* call) {
  ((mln_test_target_call*)call)->descriptor.vulkan_borrowed.context.device =
    NULL;
}
static void borrowed_without_image(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.vulkan[0].image = MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL;
  edited->descriptor.vulkan_borrowed.textures = edited->textures.vulkan;
}
static void borrowed_without_image_view(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.vulkan[0].image_view =
    MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL;
  edited->descriptor.vulkan_borrowed.textures = edited->textures.vulkan;
}
static void replacement_without_image(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.vulkan[0].image = MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL;
  edited->descriptor.vulkan_borrowed_replacement.textures =
    edited->textures.vulkan;
}
static void replacement_without_image_view(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.vulkan[0].image_view =
    MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL;
  edited->descriptor.vulkan_borrowed_replacement.textures =
    edited->textures.vulkan;
}
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
// A replacement that names two images for a session's ring of one.
static void replacement_with_another_depth(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.vulkan[1] = (mln_vulkan_borrowed_texture){
    .image = high_view_handle,
    .image_view = high_handle,
  };
  edited->descriptor.vulkan_borrowed_replacement.textures =
    edited->textures.vulkan;
  edited->descriptor.vulkan_borrowed_replacement.texture_count = 2;
}
#endif
static void borrowed_without_physical_width(void* call) {
  ((mln_test_target_call*)call)->descriptor.vulkan_borrowed.physical_width = 0;
}
static void replacement_without_physical_width(void* call) {
  ((mln_test_target_call*)call)
    ->descriptor.vulkan_borrowed_replacement.physical_width = 0;
}
static void borrowed_without_format(void* call) {
  ((mln_test_target_call*)call)->descriptor.vulkan_borrowed.format = 0;
}
static void borrowed_without_final_layout(void* call) {
  ((mln_test_target_call*)call)->descriptor.vulkan_borrowed.final_layout = 0;
}
static void borrowed_with_unknown_owner(void* call) {
  ((mln_test_target_call*)call)->descriptor.vulkan_borrowed.owner =
    (mln_vulkan_image_owner)99;
}

MLN_TEST_ATTACH_SUBMITTER(
  submit_borrowed_attach, mln_map_attach_vulkan_borrowed_texture
)
MLN_TEST_SET_TARGET_SUBMITTER(
  submit_borrowed_set_target,
  mln_render_session_set_vulkan_borrowed_texture_target
)

static mln_vulkan_context_descriptor fake_context(void) {
  return (mln_vulkan_context_descriptor){
    .instance = MLN_TEST_FAKE_HANDLE,
    .physical_device = MLN_TEST_FAKE_HANDLE,
    .device = MLN_TEST_FAKE_HANDLE,
    .graphics_queue = MLN_TEST_FAKE_HANDLE,
  };
}

// A ring of one image, whose entry is in the call's storage.
static void describe_borrowed(mln_test_target_call* call) {
  call->textures.vulkan[0] = (mln_vulkan_borrowed_texture){
    .image = high_handle,
    .image_view = high_view_handle,
  };
  mln_vulkan_borrowed_texture_descriptor* descriptor =
    &call->descriptor.vulkan_borrowed;
  *descriptor = (mln_vulkan_borrowed_texture_descriptor){
    .size = sizeof(mln_vulkan_borrowed_texture_descriptor)
  };
  descriptor->logical_size = mln_test_target_size();
  descriptor->context = fake_context();
  descriptor->textures = call->textures.vulkan;
  descriptor->texture_count = 1;
  // VK_FORMAT_R8G8B8A8_UNORM, from undefined to shader-read-only.
  descriptor->format = 37;
  descriptor->final_layout = 5;
  descriptor->physical_width = 64;
  descriptor->physical_height = 64;
}

// A replacement ring of one image, whose entry is in the call's storage.
static void describe_borrowed_replacement(mln_test_target_call* call) {
  call->textures.vulkan[0] = (mln_vulkan_borrowed_texture){
    .image = high_handle,
    .image_view = high_view_handle,
  };
  call->descriptor.vulkan_borrowed_replacement =
    (mln_vulkan_borrowed_texture_replacement){
      .size = sizeof(mln_vulkan_borrowed_texture_replacement),
      .logical_size = mln_test_target_size(),
      .physical_width = 64,
      .physical_height = 64,
      .textures = call->textures.vulkan,
      .texture_count = 1,
  };
}

// A Vulkan build accepts a well-formed descriptor and leaves the handles to
// its driver, so only the other builds submit one. There the well-formed row,
// whose handles live in the high bits, proves they arrive whole.
static void vulkan_attach_rejects_malformed_calls(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_target_map(runtime);

  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD);
  describe_borrowed(&call);
  static const mln_test_validation_case borrowed_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(vulkan_borrowed),
    MLN_TEST_BORROWED_CASES(vulkan_borrowed),
    {"null device", borrowed_without_device, MLN_STATUS_INVALID_ARGUMENT,
     "must not be null"},
    {"null image", borrowed_without_image, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"null image view", borrowed_without_image_view,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"zero physical width", borrowed_without_physical_width,
     MLN_STATUS_INVALID_ARGUMENT, "physical texture dimensions"},
    {"zero format", borrowed_without_format, MLN_STATUS_INVALID_ARGUMENT,
     "format and final_layout"},
    {"zero final layout", borrowed_without_final_layout,
     MLN_STATUS_INVALID_ARGUMENT, "format and final_layout"},
    {"unknown owner", borrowed_with_unknown_owner, MLN_STATUS_INVALID_ARGUMENT,
     "owner is invalid"},
#if !defined(MLN_FFI_TEST_BACKEND_VULKAN)
    {"a well-formed descriptor with high handles", NULL, MLN_STATUS_UNSUPPORTED,
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
static void vulkan_set_target_rejects_malformed_replacements(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CORE_WORKER);
  call.session = mln_test_create_disposed_session(map);
  describe_borrowed_replacement(&call);
  static const mln_test_validation_case borrowed_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(vulkan_borrowed_replacement),
    MLN_TEST_BORROWED_CASES(vulkan_borrowed_replacement),
    {"null image", replacement_without_image, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"null image view", replacement_without_image_view,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"zero physical width", replacement_without_physical_width,
     MLN_STATUS_INVALID_ARGUMENT, "physical texture dimensions"},
  };
  mln_test_run_set_target_table(
    submit_borrowed_set_target, &call, borrowed_rows,
    sizeof(borrowed_rows) / sizeof(borrowed_rows[0]),
    MLN_RENDER_BACKEND_FLAG_VULKAN
  );

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_borrowed_ring(
      map, &fixture, 1, MLN_TEST_PRESET_DRIVER
    ),
    mln_test_graphics_last_error()
  );
  call.session = fixture.session;
  static const mln_test_validation_case session_rows[] = {
    {"more images than the session's ring has slots",
     replacement_with_another_depth, MLN_STATUS_INVALID_ARGUMENT, "ring depth"},
  };
  mln_test_run_validation_table(
    session_rows, sizeof(session_rows) / sizeof(session_rows[0]), &call,
    sizeof(call), submit_borrowed_set_target, NULL
  );
  mln_test_render_fixture_destroy(&fixture);
#endif
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(vulkan_attach_rejects_malformed_calls);
  RUN_TEST(vulkan_set_target_rejects_malformed_replacements);
}
