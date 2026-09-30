// WebGPU target descriptors: what attach and set_target reject before they
// reach a device. Every build validates a WebGPU descriptor before it reports
// that it carries no WebGPU backend, so these tables run on every preset.

#include <stddef.h>
#include <stdint.h>

#include "support/attach_table.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

// Any value but WGPUTextureFormat_Undefined: validation rejects that one and
// takes the rest as given.
enum { some_texture_format = 18 };

MLN_TEST_DESCRIPTOR_EDITS(webgpu_surface, mln_webgpu_surface_descriptor)
MLN_TEST_OVERFLOW_EDIT(webgpu_surface)
MLN_TEST_DESCRIPTOR_EDITS(webgpu_owned, mln_webgpu_owned_texture_descriptor)
MLN_TEST_OVERFLOW_EDIT(webgpu_owned)
MLN_TEST_DESCRIPTOR_EDITS(
  webgpu_borrowed, mln_webgpu_borrowed_texture_descriptor
)

static void surface_without_surface(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_surface.surface = NULL;
}
static void surface_without_format(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_surface.format = 0;
}
static void surface_with_undersized_context(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_surface.context.size =
    sizeof(mln_webgpu_context_descriptor) - 1;
}
static void owned_without_device(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_owned.context.device = NULL;
}
static void owned_with_undersized_context(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_owned.context.size =
    sizeof(mln_webgpu_context_descriptor) - 1;
}
static void borrowed_without_device(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_borrowed.context.device =
    NULL;
}
static void borrowed_without_texture_view(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_borrowed.texture_view = NULL;
}
static void borrowed_without_format(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_borrowed.format = 0;
}
static void borrowed_without_physical_width(void* call) {
  ((mln_test_target_call*)call)->descriptor.webgpu_borrowed.physical_width = 0;
}

MLN_TEST_ATTACH_SUBMITTER(submit_surface_attach, mln_webgpu_surface_attach)
MLN_TEST_ATTACH_SUBMITTER(submit_owned_attach, mln_webgpu_owned_texture_attach)
MLN_TEST_ATTACH_SUBMITTER(
  submit_borrowed_attach, mln_webgpu_borrowed_texture_attach
)
#if !defined(MLN_FFI_TEST_BACKEND_WEBGPU)
MLN_TEST_SET_TARGET_SUBMITTER(
  submit_surface_set_target, mln_webgpu_surface_set_target
)
MLN_TEST_SET_TARGET_SUBMITTER(
  submit_borrowed_set_target, mln_webgpu_borrowed_texture_set_target
)
#endif

static mln_webgpu_surface_descriptor surface_descriptor(void) {
  mln_webgpu_surface_descriptor descriptor =
    mln_webgpu_surface_descriptor_default();
  descriptor.extent = mln_test_target_extent();
  descriptor.context.device = MLN_TEST_FAKE_HANDLE;
  descriptor.surface = MLN_TEST_FAKE_HANDLE;
  descriptor.format = some_texture_format;
  return descriptor;
}

static mln_webgpu_borrowed_texture_descriptor borrowed_descriptor(void) {
  mln_webgpu_borrowed_texture_descriptor descriptor =
    mln_webgpu_borrowed_texture_descriptor_default();
  descriptor.extent = mln_test_target_extent();
  descriptor.context.device = MLN_TEST_FAKE_HANDLE;
  descriptor.texture = MLN_TEST_FAKE_HANDLE;
  descriptor.texture_view = MLN_TEST_FAKE_HANDLE;
  descriptor.format = some_texture_format;
  descriptor.physical_width = 64;
  descriptor.physical_height = 64;
  return descriptor;
}

static void webgpu_attach_rejects_malformed_calls(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD);
  call.descriptor.webgpu_surface = surface_descriptor();
  // A surface with no format cannot be configured, so attach rejects it
  // rather than leaving the browser to report it.
  static const mln_test_validation_case surface_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(webgpu_surface),
    MLN_TEST_OVERFLOW_CASE(webgpu_surface),
    {"undersized context", surface_with_undersized_context,
     MLN_STATUS_INVALID_ARGUMENT, "mln_webgpu_context_descriptor.size"},
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

  call.descriptor.webgpu_owned = mln_webgpu_owned_texture_descriptor_default();
  call.descriptor.webgpu_owned.extent = mln_test_target_extent();
  call.descriptor.webgpu_owned.context.device = MLN_TEST_FAKE_HANDLE;
  static const mln_test_validation_case owned_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(webgpu_owned),
    MLN_TEST_OVERFLOW_CASE(webgpu_owned),
    {"undersized context", owned_with_undersized_context,
     MLN_STATUS_INVALID_ARGUMENT, "mln_webgpu_context_descriptor.size"},
    {"null device", owned_without_device, MLN_STATUS_INVALID_ARGUMENT, NULL},
#if !defined(MLN_FFI_TEST_BACKEND_WEBGPU)
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_attach_table(
    submit_owned_attach, &call, owned_rows,
    sizeof(owned_rows) / sizeof(owned_rows[0])
  );

  call.descriptor.webgpu_borrowed = borrowed_descriptor();
  static const mln_test_validation_case borrowed_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(webgpu_borrowed),
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

#if !defined(MLN_FFI_TEST_BACKEND_WEBGPU)
// A build without WebGPU checks a replacement descriptor before it reports the
// missing backend, whatever the session.
static void webgpu_set_target_checks_its_descriptor_first(void) {
  mln_test_target_call call = mln_test_target_call_default(
    MLN_HANDLE_NULL, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
  );
  call.descriptor.webgpu_surface = surface_descriptor();
  static const mln_test_validation_case surface_rows[] = {
    {"null descriptor", mln_test_call_without_descriptor,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    MLN_TEST_DESCRIPTOR_CASES(webgpu_surface),
    {"null surface", surface_without_surface, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"unspecified format", surface_without_format, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
  };
  mln_test_run_validation_table(
    surface_rows, sizeof(surface_rows) / sizeof(surface_rows[0]), &call,
    sizeof(call), submit_surface_set_target, NULL
  );

  call.descriptor.webgpu_borrowed = borrowed_descriptor();
  static const mln_test_validation_case borrowed_rows[] = {
    {"null descriptor", mln_test_call_without_descriptor,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    MLN_TEST_DESCRIPTOR_CASES(webgpu_borrowed),
    {"null texture view", borrowed_without_texture_view,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"zero physical width", borrowed_without_physical_width,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
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
  RUN_TEST(webgpu_attach_rejects_malformed_calls);
#if !defined(MLN_FFI_TEST_BACKEND_WEBGPU)
  RUN_TEST(webgpu_set_target_checks_its_descriptor_first);
#endif
}
