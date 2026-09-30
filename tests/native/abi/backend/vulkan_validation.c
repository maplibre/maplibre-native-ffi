// Vulkan target descriptors: what attach and set_target reject before they
// reach a device. Every build validates a Vulkan descriptor before it reports
// that it carries no Vulkan backend, so these tables run on every preset.

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "support/attach_table.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static_assert(
  sizeof(((mln_vulkan_borrowed_texture_descriptor*)0)->image) ==
    sizeof(uint64_t),
  "a Vulkan non-dispatchable handle is 64 bits wide on every ABI"
);

// Non-dispatchable handles whose value lies entirely in the high 32 bits: a
// carrier that truncated them to a 32-bit pointer would pass null.
static const mln_vulkan_non_dispatchable_handle high_handle =
  UINT64_C(0x8000000000000001);
static const mln_vulkan_non_dispatchable_handle high_view_handle =
  UINT64_C(0x0000000100000000);

MLN_TEST_DESCRIPTOR_EDITS(vulkan_surface, mln_vulkan_surface_descriptor)
MLN_TEST_OVERFLOW_EDIT(vulkan_surface)
MLN_TEST_DESCRIPTOR_EDITS(vulkan_owned, mln_vulkan_owned_texture_descriptor)
MLN_TEST_OVERFLOW_EDIT(vulkan_owned)
MLN_TEST_DESCRIPTOR_EDITS(
  vulkan_borrowed, mln_vulkan_borrowed_texture_descriptor
)

static void surface_without_surface(void* call) {
  ((mln_test_target_call*)call)->descriptor.vulkan_surface.surface =
    MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL;
}
static void surface_with_undersized_context(void* call) {
  ((mln_test_target_call*)call)->descriptor.vulkan_surface.context.size =
    sizeof(mln_vulkan_context_descriptor) - 1;
}
static void owned_without_device(void* call) {
  ((mln_test_target_call*)call)->descriptor.vulkan_owned.context.device = NULL;
}
static void owned_with_undersized_context(void* call) {
  ((mln_test_target_call*)call)->descriptor.vulkan_owned.context.size =
    sizeof(mln_vulkan_context_descriptor) - 1;
}
static void borrowed_with_undersized_context(void* call) {
  ((mln_test_target_call*)call)->descriptor.vulkan_borrowed.context.size =
    sizeof(mln_vulkan_context_descriptor) - 1;
}
static void borrowed_without_image(void* call) {
  ((mln_test_target_call*)call)->descriptor.vulkan_borrowed.image =
    MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL;
}
static void borrowed_without_image_view(void* call) {
  ((mln_test_target_call*)call)->descriptor.vulkan_borrowed.image_view =
    MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL;
}
static void borrowed_without_physical_width(void* call) {
  ((mln_test_target_call*)call)->descriptor.vulkan_borrowed.physical_width = 0;
}

MLN_TEST_ATTACH_SUBMITTER(submit_surface_attach, mln_vulkan_surface_attach)
MLN_TEST_ATTACH_SUBMITTER(submit_owned_attach, mln_vulkan_owned_texture_attach)
MLN_TEST_ATTACH_SUBMITTER(
  submit_borrowed_attach, mln_vulkan_borrowed_texture_attach
)
#if !defined(MLN_FFI_TEST_BACKEND_VULKAN)
MLN_TEST_SET_TARGET_SUBMITTER(
  submit_surface_set_target, mln_vulkan_surface_set_target
)
MLN_TEST_SET_TARGET_SUBMITTER(
  submit_borrowed_set_target, mln_vulkan_borrowed_texture_set_target
)
#endif

static mln_vulkan_context_descriptor fake_context(void) {
  return (mln_vulkan_context_descriptor){
    .size = sizeof(mln_vulkan_context_descriptor),
    .instance = MLN_TEST_FAKE_HANDLE,
    .physical_device = MLN_TEST_FAKE_HANDLE,
    .device = MLN_TEST_FAKE_HANDLE,
    .graphics_queue = MLN_TEST_FAKE_HANDLE,
  };
}

static mln_vulkan_surface_descriptor surface_descriptor(void) {
  mln_vulkan_surface_descriptor descriptor =
    mln_vulkan_surface_descriptor_default();
  descriptor.extent = mln_test_target_extent();
  descriptor.context = fake_context();
  descriptor.surface = high_handle;
  return descriptor;
}

static mln_vulkan_borrowed_texture_descriptor borrowed_descriptor(void) {
  mln_vulkan_borrowed_texture_descriptor descriptor =
    mln_vulkan_borrowed_texture_descriptor_default();
  descriptor.extent = mln_test_target_extent();
  descriptor.context = fake_context();
  descriptor.image = high_handle;
  descriptor.image_view = high_view_handle;
  // VK_FORMAT_R8G8B8A8_UNORM, from undefined to shader-read-only.
  descriptor.format = 37;
  descriptor.initial_layout = 0;
  descriptor.final_layout = 5;
  descriptor.physical_width = 64;
  descriptor.physical_height = 64;
  return descriptor;
}

// A Vulkan build accepts a well-formed descriptor and leaves the handles to
// its driver, so only the other builds submit one. There the well-formed row,
// whose handles live in the high bits, proves they arrive whole.
static void vulkan_attach_rejects_malformed_calls(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CORE_WORKER);
  call.descriptor.vulkan_surface = surface_descriptor();
  static const mln_test_validation_case surface_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(vulkan_surface),
    MLN_TEST_OVERFLOW_CASE(vulkan_surface),
    {"undersized context", surface_with_undersized_context,
     MLN_STATUS_INVALID_ARGUMENT, "mln_vulkan_context_descriptor.size"},
    {"null surface", surface_without_surface, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
#if !defined(MLN_FFI_TEST_BACKEND_VULKAN)
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_attach_table(
    submit_surface_attach, &call, surface_rows,
    sizeof(surface_rows) / sizeof(surface_rows[0])
  );

  call.descriptor.vulkan_owned = mln_vulkan_owned_texture_descriptor_default();
  call.descriptor.vulkan_owned.extent = mln_test_target_extent();
  call.descriptor.vulkan_owned.context = fake_context();
  static const mln_test_validation_case owned_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(vulkan_owned),
    MLN_TEST_OVERFLOW_CASE(vulkan_owned),
    {"undersized context", owned_with_undersized_context,
     MLN_STATUS_INVALID_ARGUMENT, "mln_vulkan_context_descriptor.size"},
    {"null device", owned_without_device, MLN_STATUS_INVALID_ARGUMENT,
     "must not be null"},
#if !defined(MLN_FFI_TEST_BACKEND_VULKAN)
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_attach_table(
    submit_owned_attach, &call, owned_rows,
    sizeof(owned_rows) / sizeof(owned_rows[0])
  );

  call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD);
  call.descriptor.vulkan_borrowed = borrowed_descriptor();
  static const mln_test_validation_case borrowed_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(vulkan_borrowed),
    {"undersized context", borrowed_with_undersized_context,
     MLN_STATUS_INVALID_ARGUMENT, "mln_vulkan_context_descriptor.size"},
    {"null image", borrowed_without_image, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"null image view", borrowed_without_image_view,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"zero physical width", borrowed_without_physical_width,
     MLN_STATUS_INVALID_ARGUMENT, "physical texture dimensions"},
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

#if !defined(MLN_FFI_TEST_BACKEND_VULKAN)
// A build without Vulkan checks a replacement descriptor before it reports the
// missing backend, whatever the session. A Vulkan build checks the session
// first; retarget.c covers its replacements.
static void vulkan_set_target_checks_its_descriptor_first(void) {
  mln_test_target_call call = mln_test_target_call_default(
    MLN_HANDLE_NULL, MLN_RENDER_DRIVER_CORE_WORKER
  );
  call.descriptor.vulkan_surface = surface_descriptor();
  static const mln_test_validation_case surface_rows[] = {
    {"null descriptor", mln_test_call_without_descriptor,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    MLN_TEST_DESCRIPTOR_CASES(vulkan_surface),
    {"null surface", surface_without_surface, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
  };
  mln_test_run_validation_table(
    surface_rows, sizeof(surface_rows) / sizeof(surface_rows[0]), &call,
    sizeof(call), submit_surface_set_target, NULL
  );

  call.descriptor.vulkan_borrowed = borrowed_descriptor();
  static const mln_test_validation_case borrowed_rows[] = {
    {"null descriptor", mln_test_call_without_descriptor,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    MLN_TEST_DESCRIPTOR_CASES(vulkan_borrowed),
    {"null image", borrowed_without_image, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"zero physical width", borrowed_without_physical_width,
     MLN_STATUS_INVALID_ARGUMENT, "physical texture dimensions"},
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
  };
  mln_test_run_validation_table(
    borrowed_rows, sizeof(borrowed_rows) / sizeof(borrowed_rows[0]), &call,
    sizeof(call), submit_borrowed_set_target, NULL
  );
}
#endif

MLN_TEST_GROUP {
  RUN_TEST(vulkan_attach_rejects_malformed_calls);
#if !defined(MLN_FFI_TEST_BACKEND_VULKAN)
  RUN_TEST(vulkan_set_target_checks_its_descriptor_first);
#endif
}
