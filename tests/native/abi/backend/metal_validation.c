// Metal target descriptors: what attach and set_target reject before they
// reach a device. Every build validates a Metal descriptor before it reports
// that it carries no Metal backend, so these tables run on every preset.

#include "support/attach_table.h"
#include "support/host_graphics.h"
#include "support/test_support.h"

MLN_TEST_DESCRIPTOR_EDITS(metal_surface, mln_metal_surface_descriptor)
MLN_TEST_OVERFLOW_EDIT(metal_surface)
MLN_TEST_DESCRIPTOR_EDITS(metal_owned, mln_metal_owned_texture_descriptor)
MLN_TEST_OVERFLOW_EDIT(metal_owned)
MLN_TEST_DESCRIPTOR_EDITS(metal_borrowed, mln_metal_borrowed_texture_descriptor)

static void surface_without_layer(void* call) {
  ((mln_test_target_call*)call)->descriptor.metal_surface.layer = NULL;
}
static void surface_with_undersized_context(void* call) {
  ((mln_test_target_call*)call)->descriptor.metal_surface.context.size =
    sizeof(mln_metal_context_descriptor) - 1;
}
static void owned_without_device(void* call) {
  ((mln_test_target_call*)call)->descriptor.metal_owned.context.device = NULL;
}
static void owned_with_undersized_context(void* call) {
  ((mln_test_target_call*)call)->descriptor.metal_owned.context.size =
    sizeof(mln_metal_context_descriptor) - 1;
}
static void borrowed_without_texture(void* call) {
  ((mln_test_target_call*)call)->descriptor.metal_borrowed.texture = NULL;
}
static void borrowed_without_physical_width(void* call) {
  ((mln_test_target_call*)call)->descriptor.metal_borrowed.physical_width = 0;
}
#if defined(MLN_FFI_TEST_BACKEND_METAL)
static void borrowed_with_another_physical_size(void* call) {
  ((mln_test_target_call*)call)->descriptor.metal_borrowed.physical_height = 32;
}
#endif

MLN_TEST_ATTACH_SUBMITTER(submit_surface_attach, mln_metal_surface_attach)
MLN_TEST_ATTACH_SUBMITTER(submit_owned_attach, mln_metal_owned_texture_attach)
MLN_TEST_ATTACH_SUBMITTER(
  submit_borrowed_attach, mln_metal_borrowed_texture_attach
)
MLN_TEST_SET_TARGET_SUBMITTER(
  submit_surface_set_target, mln_metal_surface_set_target
)
MLN_TEST_SET_TARGET_SUBMITTER(
  submit_borrowed_set_target, mln_metal_borrowed_texture_set_target
)

static mln_metal_surface_descriptor surface_descriptor(void) {
  mln_metal_surface_descriptor descriptor =
    mln_metal_surface_descriptor_default();
  descriptor.extent = mln_test_target_extent();
  descriptor.layer = MLN_TEST_FAKE_HANDLE;
  return descriptor;
}

// A Metal build reads the texture's size and usage when it validates a
// borrowed descriptor, so there the descriptor names a real texture.
static mln_metal_borrowed_texture_descriptor borrowed_descriptor(
  void* texture
) {
  mln_metal_borrowed_texture_descriptor descriptor =
    mln_metal_borrowed_texture_descriptor_default();
  descriptor.extent = mln_test_target_extent();
  descriptor.texture = texture;
  descriptor.physical_width = 64;
  descriptor.physical_height = 64;
  return descriptor;
}

static void metal_attach_rejects_malformed_calls(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  mln_test_graphics* graphics =
    mln_test_graphics_create(MLN_TEST_GRAPHICS_BACKEND_METAL);
  TEST_ASSERT_NOT_NULL_MESSAGE(graphics, mln_test_graphics_last_error());
  mln_test_graphics_texture* texture =
    mln_test_graphics_texture_create(graphics, 64, 64);
  TEST_ASSERT_NOT_NULL_MESSAGE(texture, mln_test_graphics_last_error());
  mln_test_graphics_texture_info texture_info = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_texture_get_info(texture, &texture_info));
  void* borrowed_texture = texture_info.metal_texture;
#else
  void* borrowed_texture = MLN_TEST_FAKE_HANDLE;
#endif

  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CORE_WORKER);
  call.descriptor.metal_surface = surface_descriptor();
  static const mln_test_validation_case surface_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(metal_surface),
    MLN_TEST_OVERFLOW_CASE(metal_surface),
    {"undersized context", surface_with_undersized_context,
     MLN_STATUS_INVALID_ARGUMENT, "mln_metal_context_descriptor.size"},
    {"null layer", surface_without_layer, MLN_STATUS_INVALID_ARGUMENT, NULL},
#if !defined(MLN_FFI_TEST_BACKEND_METAL)
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_attach_table(
    submit_surface_attach, &call, surface_rows,
    sizeof(surface_rows) / sizeof(surface_rows[0])
  );

  call.descriptor.metal_owned = mln_metal_owned_texture_descriptor_default();
  call.descriptor.metal_owned.extent = mln_test_target_extent();
  call.descriptor.metal_owned.context.device = MLN_TEST_FAKE_HANDLE;
  static const mln_test_validation_case owned_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(metal_owned),
    MLN_TEST_OVERFLOW_CASE(metal_owned),
    {"undersized context", owned_with_undersized_context,
     MLN_STATUS_INVALID_ARGUMENT, "mln_metal_context_descriptor.size"},
    {"null device", owned_without_device, MLN_STATUS_INVALID_ARGUMENT, NULL},
#if !defined(MLN_FFI_TEST_BACKEND_METAL)
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
  call.descriptor.metal_borrowed = borrowed_descriptor(borrowed_texture);
  static const mln_test_validation_case borrowed_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(metal_borrowed),
    {"null texture", borrowed_without_texture, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"zero physical width", borrowed_without_physical_width,
     MLN_STATUS_INVALID_ARGUMENT, "physical texture dimensions"},
#if defined(MLN_FFI_TEST_BACKEND_METAL)
    {"a physical size the texture does not have",
     borrowed_with_another_physical_size, MLN_STATUS_INVALID_ARGUMENT,
     "must match descriptor physical size"},
#else
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_attach_table(
    submit_borrowed_attach, &call, borrowed_rows,
    sizeof(borrowed_rows) / sizeof(borrowed_rows[0])
  );

#if defined(MLN_FFI_TEST_BACKEND_METAL)
  mln_test_graphics_texture_destroy(texture);
  mln_test_graphics_destroy(graphics);
#endif
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// set_target checks the session before the descriptor, and a build without
// Metal reports the missing backend last, so there the rows name no session.
// A Metal build submits them for a live session of the replacement's kind.
static void metal_set_target_rejects_malformed_descriptors(void) {
  mln_test_target_call call = mln_test_target_call_default(
    MLN_HANDLE_NULL, MLN_RENDER_DRIVER_CORE_WORKER
  );
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_surface(map, &fixture),
    mln_test_graphics_last_error()
  );
  call.session = fixture.session;
#endif
  call.descriptor.metal_surface = surface_descriptor();
  static const mln_test_validation_case surface_rows[] = {
    {"null descriptor", mln_test_call_without_descriptor,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    MLN_TEST_DESCRIPTOR_CASES(metal_surface),
    {"null layer", surface_without_layer, MLN_STATUS_INVALID_ARGUMENT, NULL},
#if !defined(MLN_FFI_TEST_BACKEND_METAL)
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_validation_table(
    surface_rows, sizeof(surface_rows) / sizeof(surface_rows[0]), &call,
    sizeof(call), submit_surface_set_target, NULL
  );

#if defined(MLN_FFI_TEST_BACKEND_METAL)
  mln_test_render_fixture_destroy(&fixture);
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_borrowed_texture(map, &fixture),
    mln_test_graphics_last_error()
  );
  call.session = fixture.session;
  mln_test_graphics_texture* replacement =
    mln_test_render_fixture_new_texture(&fixture);
  TEST_ASSERT_NOT_NULL_MESSAGE(replacement, mln_test_graphics_last_error());
  mln_test_graphics_texture_info replacement_info = {0};
  TEST_ASSERT_TRUE(
    mln_test_graphics_texture_get_info(replacement, &replacement_info)
  );
  call.descriptor.metal_borrowed =
    borrowed_descriptor(replacement_info.metal_texture);
#else
  call.descriptor.metal_borrowed = borrowed_descriptor(MLN_TEST_FAKE_HANDLE);
#endif
  static const mln_test_validation_case borrowed_rows[] = {
    {"null descriptor", mln_test_call_without_descriptor,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    MLN_TEST_DESCRIPTOR_CASES(metal_borrowed),
    {"null texture", borrowed_without_texture, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"zero physical width", borrowed_without_physical_width,
     MLN_STATUS_INVALID_ARGUMENT, "physical texture dimensions"},
#if defined(MLN_FFI_TEST_BACKEND_METAL)
    {"a physical size the texture does not have",
     borrowed_with_another_physical_size, MLN_STATUS_INVALID_ARGUMENT,
     "must match descriptor physical size"},
#else
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_validation_table(
    borrowed_rows, sizeof(borrowed_rows) / sizeof(borrowed_rows[0]), &call,
    sizeof(call), submit_borrowed_set_target, NULL
  );
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
#endif
}

MLN_TEST_GROUP {
  RUN_TEST(metal_attach_rejects_malformed_calls);
  RUN_TEST(metal_set_target_rejects_malformed_descriptors);
}
