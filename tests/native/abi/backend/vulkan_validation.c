// Vulkan rings and devices: what attach, set_target, and
// mln_gpu_device_wrap_vulkan() reject before they reach a device. Every build
// validates a Vulkan ring before it reports that it carries no Vulkan backend,
// so these tables run on every preset.

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

MLN_TEST_BORROWED_EDITS(vulkan_borrowed_textures, vulkan)

static void ring_without_image(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.vulkan[0].image = MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL;
  edited->descriptor.target.data.vulkan_borrowed_textures.textures =
    edited->textures.vulkan;
}
static void ring_without_image_view(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.vulkan[0].image_view =
    MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL;
  edited->descriptor.target.data.vulkan_borrowed_textures.textures =
    edited->textures.vulkan;
}
static void ring_without_format(void* call) {
  ((mln_test_target_call*)call)
    ->descriptor.target.data.vulkan_borrowed_textures.format = 0;
}
static void ring_without_final_layout(void* call) {
  ((mln_test_target_call*)call)
    ->descriptor.target.data.vulkan_borrowed_textures.final_layout = 0;
}
static void ring_with_unknown_owner(void* call) {
  ((mln_test_target_call*)call)
    ->descriptor.target.data.vulkan_borrowed_textures.owner =
    (mln_vulkan_image_owner)99;
}
static void surface_without_surface(void* call) {
  ((mln_test_target_call*)call)->descriptor.target.data.vulkan_surface.surface =
    MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL;
}
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
// A replacement that names two images for a session's ring of one.
static void ring_with_another_depth(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.vulkan[1] = (mln_vulkan_borrowed_texture){
    .image = high_view_handle,
    .image_view = high_handle,
  };
  edited->descriptor.target.data.vulkan_borrowed_textures.textures =
    edited->textures.vulkan;
  edited->descriptor.target.data.vulkan_borrowed_textures.texture_count = 2;
}
#endif

// A ring of one image, whose entry is in the call's storage.
static void describe_ring(mln_test_target_call* call) {
  call->textures.vulkan[0] = (mln_vulkan_borrowed_texture){
    .image = high_handle,
    .image_view = high_view_handle,
  };
  call->descriptor.target = (mln_render_target){
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_VULKAN_BORROWED_TEXTURES,
  };
  call->descriptor.target.data.vulkan_borrowed_textures =
    (mln_vulkan_borrowed_textures_target){
      .logical_size = mln_test_target_size(),
      .physical_width = 64,
      .physical_height = 64,
      .textures = call->textures.vulkan,
      .texture_count = 1,
      // VK_FORMAT_R8G8B8A8_UNORM, left shader-read-only.
      .format = 37,
      .final_layout = 5,
    };
}

static bool vulkan_build(void) {
  return (mln_supported_render_backend_mask() &
          MLN_RENDER_BACKEND_FLAG_VULKAN) != 0;
}

// A ring renders on the device of its textures, so an attach without one
// refuses it, as a build without Vulkan does.
static void vulkan_attach_rejects_malformed_calls(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_target_map(runtime);

  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CORE_WORKER);
  describe_ring(&call);
  static const mln_test_validation_case ring_rows[] = {
    MLN_TEST_BORROWED_CASES(vulkan_borrowed_textures),
    {"null image", ring_without_image, MLN_STATUS_INVALID_ARGUMENT,
     "image and an image view"},
    {"null image view", ring_without_image_view, MLN_STATUS_INVALID_ARGUMENT,
     "image and an image view"},
    {"zero format", ring_without_format, MLN_STATUS_INVALID_ARGUMENT,
     "format and final_layout"},
    {"zero final layout", ring_without_final_layout,
     MLN_STATUS_INVALID_ARGUMENT, "format and final_layout"},
    {"unknown owner", ring_with_unknown_owner, MLN_STATUS_INVALID_ARGUMENT,
     "owner is invalid"},
    {"a well-formed ring without a device", NULL, MLN_STATUS_UNSUPPORTED, NULL},
  };
  mln_test_run_attach_table(
    mln_test_submit_target_attach, &call, ring_rows,
    sizeof(ring_rows) / sizeof(ring_rows[0])
  );

  call.descriptor.target = (mln_render_target){
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_VULKAN_SURFACE,
    .data.vulkan_surface = {
      .logical_size = mln_test_target_size(), .surface = high_handle
    },
  };
  static const mln_test_validation_case surface_rows[] = {
    {"null surface", surface_without_surface, MLN_STATUS_INVALID_ARGUMENT,
     "must not be null"},
    {"a well-formed surface without a device", NULL, MLN_STATUS_UNSUPPORTED,
     NULL},
  };
  mln_test_run_validation_table(
    surface_rows, sizeof(surface_rows) / sizeof(surface_rows[0]), &call,
    sizeof(call), mln_test_submit_target_attach, NULL
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// set_target checks its ring before the session, so these rows cover a row
// for each pointer and handle it reads, and its own steps.
static void vulkan_set_target_rejects_malformed_replacements(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CORE_WORKER);
  call.session = mln_test_create_disposed_session(map);
  describe_ring(&call);
  static const mln_test_validation_case ring_rows[] = {
    MLN_TEST_BORROWED_CASES(vulkan_borrowed_textures),
    {"null image", ring_without_image, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"null image view", ring_without_image_view, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
  };
  mln_test_run_set_target_table(
    mln_test_submit_target_set_target, &call, ring_rows,
    sizeof(ring_rows) / sizeof(ring_rows[0])
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
    {"more images than the session's ring has slots", ring_with_another_depth,
     MLN_STATUS_INVALID_ARGUMENT, "ring depth"},
  };
  mln_test_run_validation_table(
    session_rows, sizeof(session_rows) / sizeof(session_rows[0]), &call,
    sizeof(call), mln_test_submit_target_set_target, NULL
  );
  mln_test_render_fixture_destroy(&fixture);
#endif
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void lock_nothing(void* user_data) { (void)user_data; }

// mln_gpu_device_wrap_vulkan() checks its descriptor and queue lock before it
// looks the device's functions up, and a build without Vulkan wraps none.
static void wrap_vulkan_rejects_malformed_descriptors(void) {
  const mln_vulkan_device_descriptor fake = {
    .size = sizeof(mln_vulkan_device_descriptor),
    .context = {
      .instance = MLN_TEST_FAKE_HANDLE,
      .physical_device = MLN_TEST_FAKE_HANDLE,
      .device = MLN_TEST_FAKE_HANDLE,
      .graphics_queue = MLN_TEST_FAKE_HANDLE,
    },
  };
  mln_gpu_device device = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_gpu_device_wrap_vulkan(NULL, &device, NULL));
  MLN_TEST_INVALID(mln_gpu_device_wrap_vulkan(&fake, NULL, NULL));
  mln_vulkan_device_descriptor edited = fake;
  edited.size = sizeof(mln_vulkan_device_descriptor) - 1;
  MLN_TEST_INVALID(mln_gpu_device_wrap_vulkan(&edited, &device, NULL));
  edited = fake;
  edited.context.device = NULL;
  MLN_TEST_INVALID(mln_gpu_device_wrap_vulkan(&edited, &device, NULL));
  edited = fake;
  edited.context.graphics_queue = NULL;
  MLN_TEST_INVALID(mln_gpu_device_wrap_vulkan(&edited, &device, NULL));
  edited = fake;
  edited.queue_lock.lock = lock_nothing;
  MLN_TEST_INVALID(mln_gpu_device_wrap_vulkan(&edited, &device, NULL));
  edited = fake;
  edited.queue_lock.release_user_data = lock_nothing;
  MLN_TEST_INVALID(mln_gpu_device_wrap_vulkan(&edited, &device, NULL));
  mln_gpu_device occupied = 1;
  MLN_TEST_INVALID(mln_gpu_device_wrap_vulkan(&fake, &occupied, NULL));
  if (!vulkan_build()) {
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED, mln_gpu_device_wrap_vulkan(&fake, &device, NULL)
    );
  }
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
}

MLN_TEST_GROUP {
  RUN_TEST(vulkan_attach_rejects_malformed_calls);
  RUN_TEST(vulkan_set_target_rejects_malformed_replacements);
  RUN_TEST(wrap_vulkan_rejects_malformed_descriptors);
}
