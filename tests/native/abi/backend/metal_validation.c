// Metal rings and devices: what attach, set_target, and
// mln_gpu_device_wrap_metal() reject before they reach a device. Every build
// validates a Metal ring before it reports that it carries no Metal backend,
// so these tables run on every preset.

#include "support/attach_table.h"
#include "support/host_graphics.h"
#include "support/test_support.h"

#if defined(MLN_FFI_TEST_BACKEND_METAL)
#include <objc/message.h>
#include <objc/runtime.h>
#endif

MLN_TEST_BORROWED_EDITS(metal_borrowed_textures, metal)

static void ring_without_texture(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.metal[0].texture = NULL;
  edited->descriptor.target.data.metal_borrowed_textures.textures =
    edited->textures.metal;
}
#if defined(MLN_FFI_TEST_BACKEND_METAL)
static void ring_with_another_physical_size(void* call) {
  ((mln_test_target_call*)call)
    ->descriptor.target.data.metal_borrowed_textures.physical_height = 32;
}

// The second texture of a ring of two, in a pixel format the first does not
// have.
static void* other_format_texture = NULL;

static void ring_with_two_pixel_formats(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.metal[1].texture = other_format_texture;
  edited->descriptor.target.data.metal_borrowed_textures.textures =
    edited->textures.metal;
  edited->descriptor.target.data.metal_borrowed_textures.texture_count = 2;
}

// Makes a render-target texture in RGBA order, where tests/graphics makes
// BGRA8Unorm textures.
static void* new_rgba_texture(void* device) {
  // MTLPixelFormatRGBA8Unorm.
  const unsigned long rgba8_unorm = 70;
  id descriptor = ((
    id (*)(Class, SEL, unsigned long, unsigned long, unsigned long, bool)
  )objc_msgSend)(
    objc_getClass("MTLTextureDescriptor"),
    sel_registerName(
      "texture2DDescriptorWithPixelFormat:width:height:"
      "mipmapped:"
    ),
    rgba8_unorm, 64, 64, false
  );
  // MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget.
  ((void (*)(id, SEL, unsigned long))objc_msgSend)(
    descriptor, sel_registerName("setUsage:"), 5
  );
  return ((void* (*)(id, SEL, id))objc_msgSend)(
    (id)device, sel_registerName("newTextureWithDescriptor:"), descriptor
  );
}

static void release_object(void* object) {
  ((void (*)(id, SEL))objc_msgSend)((id)object, sel_registerName("release"));
}

// Another texture of the session's format, which a replacement deeper than
// the session's ring names second.
static void* second_texture = NULL;

static void ring_with_another_depth(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.metal[1].texture = second_texture;
  edited->descriptor.target.data.metal_borrowed_textures.textures =
    edited->textures.metal;
  edited->descriptor.target.data.metal_borrowed_textures.texture_count = 2;
}
#endif

// A ring of one texture, whose entry is in the call's storage. A Metal build
// reads the textures' size and format when it checks a ring against its
// device, so there the ring names a real texture.
static void describe_ring(mln_test_target_call* call, void* texture) {
  call->textures.metal[0] = (mln_metal_borrowed_texture){.texture = texture};
  call->descriptor.target = (mln_render_target){
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_METAL_BORROWED_TEXTURES,
  };
  call->descriptor.target.data.metal_borrowed_textures =
    (mln_metal_borrowed_textures_target){
      .logical_size = mln_test_target_size(),
      .physical_width = 64,
      .physical_height = 64,
      .textures = call->textures.metal,
      .texture_count = 1,
    };
}

static bool metal_build(void) {
  return (mln_supported_render_backend_mask() &
          MLN_RENDER_BACKEND_FLAG_METAL) != 0;
}

static void metal_attach_rejects_malformed_calls(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_target_map(runtime);
  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CORE_WORKER);
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  mln_test_graphics* graphics =
    mln_test_graphics_create(MLN_TEST_GRAPHICS_BACKEND_METAL);
  TEST_ASSERT_NOT_NULL_MESSAGE(graphics, mln_test_graphics_last_error());
  mln_test_graphics_texture* texture =
    mln_test_graphics_texture_create(graphics, 64, 64);
  TEST_ASSERT_NOT_NULL_MESSAGE(texture, mln_test_graphics_last_error());
  mln_test_graphics_texture_info texture_info = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_texture_get_info(texture, &texture_info));
  mln_test_graphics_context context = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_get_context(graphics, &context));
  other_format_texture = new_rgba_texture(context.metal_device);
  TEST_ASSERT_NOT_NULL(other_format_texture);
  describe_ring(&call, texture_info.metal_texture);
  // The ring's checks against its textures need their device.
  const mln_metal_device_descriptor host = {
    .size = sizeof(mln_metal_device_descriptor),
    .device = context.metal_device,
  };
  MLN_TEST_OK(mln_gpu_device_wrap_metal(&host, &call.device, NULL));
#else
  describe_ring(&call, MLN_TEST_FAKE_HANDLE);
#endif
  static const mln_test_validation_case ring_rows[] = {
    MLN_TEST_BORROWED_CASES(metal_borrowed_textures),
    {"null texture", ring_without_texture, MLN_STATUS_INVALID_ARGUMENT, NULL},
#if defined(MLN_FFI_TEST_BACKEND_METAL)
    {"a physical size the texture does not have",
     ring_with_another_physical_size, MLN_STATUS_INVALID_ARGUMENT,
     "physical size"},
    {"textures of two pixel formats", ring_with_two_pixel_formats,
     MLN_STATUS_INVALID_ARGUMENT, "pixel format"},
#else
    {"a well-formed ring", NULL, MLN_STATUS_UNSUPPORTED, NULL},
#endif
  };
  mln_test_run_attach_table(
    mln_test_submit_target_attach, &call, ring_rows,
    sizeof(ring_rows) / sizeof(ring_rows[0])
  );

#if defined(MLN_FFI_TEST_BACKEND_METAL)
  mln_gpu_device_release(call.device);
  release_object(other_format_texture);
  other_format_texture = NULL;
  mln_test_graphics_texture_destroy(texture);
  mln_test_graphics_destroy(graphics);
#endif
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// set_target checks its ring before the session, so these rows cover a row
// for each pointer and handle it reads, and its own steps.
static void metal_set_target_rejects_malformed_replacements(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CORE_WORKER);
  call.session = mln_test_create_disposed_session(map);
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  // The ring-depth row needs a live ring, whose device makes the textures.
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_borrowed_ring(
      map, &fixture, 1, MLN_TEST_PRESET_DRIVER
    ),
    mln_test_graphics_last_error()
  );
  mln_test_graphics_texture* replacement =
    mln_test_render_fixture_new_texture(&fixture);
  TEST_ASSERT_NOT_NULL_MESSAGE(replacement, mln_test_graphics_last_error());
  mln_test_graphics_texture_info replacement_info = {0};
  TEST_ASSERT_TRUE(
    mln_test_graphics_texture_get_info(replacement, &replacement_info)
  );
  describe_ring(&call, replacement_info.metal_texture);
  mln_test_graphics_texture* second =
    mln_test_render_fixture_new_texture(&fixture);
  TEST_ASSERT_NOT_NULL_MESSAGE(second, mln_test_graphics_last_error());
  mln_test_graphics_texture_info second_info = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_texture_get_info(second, &second_info));
  second_texture = second_info.metal_texture;
#else
  describe_ring(&call, MLN_TEST_FAKE_HANDLE);
#endif
  static const mln_test_validation_case ring_rows[] = {
    MLN_TEST_BORROWED_CASES(metal_borrowed_textures),
    {"null texture", ring_without_texture, MLN_STATUS_INVALID_ARGUMENT, NULL},
  };
  mln_test_run_set_target_table(
    mln_test_submit_target_set_target, &call, ring_rows,
    sizeof(ring_rows) / sizeof(ring_rows[0])
  );

#if defined(MLN_FFI_TEST_BACKEND_METAL)
  call.session = fixture.session;
  static const mln_test_validation_case session_rows[] = {
    {"more textures than the session's ring has slots", ring_with_another_depth,
     MLN_STATUS_INVALID_ARGUMENT, "ring depth"},
  };
  mln_test_run_validation_table(
    session_rows, sizeof(session_rows) / sizeof(session_rows[0]), &call,
    sizeof(call), mln_test_submit_target_set_target, NULL
  );
  second_texture = NULL;
  mln_test_render_fixture_destroy(&fixture);
#endif
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// mln_gpu_device_wrap_metal() checks its descriptor, and a build without Metal
// wraps none.
static void wrap_metal_rejects_malformed_descriptors(void) {
  const mln_metal_device_descriptor fake = {
    .size = sizeof(mln_metal_device_descriptor),
    .device = MLN_TEST_FAKE_HANDLE,
  };
  mln_gpu_device device = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_gpu_device_wrap_metal(NULL, &device, NULL));
  MLN_TEST_INVALID(mln_gpu_device_wrap_metal(&fake, NULL, NULL));
  mln_metal_device_descriptor edited = fake;
  edited.size = sizeof(mln_metal_device_descriptor) - 1;
  MLN_TEST_INVALID(mln_gpu_device_wrap_metal(&edited, &device, NULL));
  edited = fake;
  edited.device = NULL;
  MLN_TEST_INVALID(mln_gpu_device_wrap_metal(&edited, &device, NULL));
  if (!metal_build()) {
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED, mln_gpu_device_wrap_metal(&fake, &device, NULL)
    );
  }
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
}

MLN_TEST_GROUP {
  RUN_TEST(metal_attach_rejects_malformed_calls);
  RUN_TEST(metal_set_target_rejects_malformed_replacements);
  RUN_TEST(wrap_metal_rejects_malformed_descriptors);
}
