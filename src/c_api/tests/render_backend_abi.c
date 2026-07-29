// Raw C ABI/backend coverage: render target descriptors expose pointer, size,
// nested descriptor, and output-handle states hidden by bindings.

#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static void* const fake_handle = (void*)(uintptr_t)1;

#define EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(                           \
  descriptor_type, default_descriptor, attach, clear_required, shrink  \
)                                                                      \
  do {                                                                 \
    mln_render_session* session = NULL;                                \
    descriptor_type descriptor = default_descriptor();                 \
    TEST_ASSERT_EQUAL_INT(                                             \
      MLN_STATUS_INVALID_ARGUMENT, attach(NULL, &descriptor, &session) \
    );                                                                 \
    TEST_ASSERT_NULL(session);                                         \
    mln_runtime* runtime = mln_test_create_runtime();                  \
    mln_map* map = mln_test_create_map(runtime);                       \
    TEST_ASSERT_EQUAL_INT(                                             \
      MLN_STATUS_INVALID_ARGUMENT, attach(map, NULL, &session)         \
    );                                                                 \
    TEST_ASSERT_EQUAL_INT(                                             \
      MLN_STATUS_INVALID_ARGUMENT, attach(map, &descriptor, NULL)      \
    );                                                                 \
    session = (mln_render_session*)(uintptr_t)1;                       \
    TEST_ASSERT_EQUAL_INT(                                             \
      MLN_STATUS_INVALID_ARGUMENT, attach(map, &descriptor, &session)  \
    );                                                                 \
    session = NULL;                                                    \
    descriptor_type invalid = default_descriptor();                    \
    invalid.size = sizeof(descriptor_type) - 1;                        \
    TEST_ASSERT_EQUAL_INT(                                             \
      MLN_STATUS_INVALID_ARGUMENT, attach(map, &invalid, &session)     \
    );                                                                 \
    invalid = default_descriptor();                                    \
    invalid.extent.size = sizeof(mln_render_target_extent) - 1;        \
    TEST_ASSERT_EQUAL_INT(                                             \
      MLN_STATUS_INVALID_ARGUMENT, attach(map, &invalid, &session)     \
    );                                                                 \
    invalid = default_descriptor();                                    \
    shrink(&invalid);                                                  \
    TEST_ASSERT_EQUAL_INT(                                             \
      MLN_STATUS_INVALID_ARGUMENT, attach(map, &invalid, &session)     \
    );                                                                 \
    invalid = default_descriptor();                                    \
    clear_required(&invalid);                                          \
    TEST_ASSERT_EQUAL_INT(                                             \
      MLN_STATUS_INVALID_ARGUMENT, attach(map, &invalid, &session)     \
    );                                                                 \
    mln_test_destroy_map(map);                                         \
    mln_test_destroy_runtime(runtime);                                 \
  } while (false)

// WebGPU is currently configured only for browser builds, whose native test
// target is disabled. Host backend stubs still provide raw ABI validation for
// the descriptor shape and unsupported-backend behavior.
static void webgpu_texture_descriptors_validate_on_host_backends(void) {
  mln_webgpu_owned_texture_descriptor owned =
    mln_webgpu_owned_texture_descriptor_default();
  TEST_ASSERT_EQUAL_UINT32(sizeof(owned), owned.size);
  TEST_ASSERT_EQUAL_UINT32(sizeof(owned.extent), owned.extent.size);
  TEST_ASSERT_EQUAL_UINT32(sizeof(owned.context), owned.context.size);

  mln_webgpu_borrowed_texture_descriptor borrowed =
    mln_webgpu_borrowed_texture_descriptor_default();
  TEST_ASSERT_EQUAL_UINT32(sizeof(borrowed), borrowed.size);
  TEST_ASSERT_EQUAL_UINT32(256, borrowed.physical_width);
  TEST_ASSERT_EQUAL_UINT32(256, borrowed.physical_height);

  mln_runtime* runtime = mln_test_create_runtime();
  mln_map* map = mln_test_create_map(runtime);
  mln_render_session* session = NULL;
  borrowed.context.device = fake_handle;
  borrowed.texture = fake_handle;
  borrowed.texture_view = fake_handle;
  borrowed.format = 18;

  mln_webgpu_borrowed_texture_descriptor invalid = borrowed;
  invalid.physical_width = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_webgpu_borrowed_texture_attach(map, &invalid, &session)
  );
  TEST_ASSERT_NULL(session);

  invalid = borrowed;
  invalid.context.device = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_webgpu_borrowed_texture_attach(map, &invalid, &session)
  );
  TEST_ASSERT_NULL(session);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED,
    mln_webgpu_borrowed_texture_attach(map, &borrowed, &session)
  );
  TEST_ASSERT_NULL(session);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#if defined(MLN_TEST_BACKEND_METAL)

static mln_metal_surface_descriptor metal_surface_descriptor(void) {
  mln_metal_surface_descriptor value = mln_metal_surface_descriptor_default();
  value.layer = fake_handle;
  return value;
}
static void clear_metal_surface(mln_metal_surface_descriptor* descriptor) {
  descriptor->layer = NULL;
}
static void shrink_metal_surface(mln_metal_surface_descriptor* descriptor) {
  descriptor->context.size = sizeof(mln_metal_context_descriptor) - 1;
}
static mln_metal_owned_texture_descriptor metal_owned_descriptor(void) {
  mln_metal_owned_texture_descriptor value =
    mln_metal_owned_texture_descriptor_default();
  value.context.device = fake_handle;
  return value;
}
static void clear_metal_owned(mln_metal_owned_texture_descriptor* descriptor) {
  descriptor->context.device = NULL;
}
static void shrink_metal_owned(mln_metal_owned_texture_descriptor* descriptor) {
  descriptor->context.size = sizeof(mln_metal_context_descriptor) - 1;
}

// This verifies nulls, a non-null output handle, undersized descriptors, and
// missing required Metal surface handles.
static void metal_surface_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_metal_surface_descriptor, metal_surface_descriptor,
    mln_metal_surface_attach, clear_metal_surface, shrink_metal_surface
  );
}

// This verifies nulls, a non-null output handle, undersized descriptors, and
// missing required Metal texture handles.
static void metal_owned_texture_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_metal_owned_texture_descriptor, metal_owned_descriptor,
    mln_metal_owned_texture_attach, clear_metal_owned, shrink_metal_owned
  );
}

// This verifies nested extent sizing and required borrowed Metal texture
// handles hidden by binding descriptors.
static void metal_borrowed_texture_rejects_unsafe_raw_descriptors(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_map* map = mln_test_create_map(runtime);
  mln_metal_borrowed_texture_descriptor descriptor =
    mln_metal_borrowed_texture_descriptor_default();
  descriptor.extent.width = 128;
  descriptor.extent.height = 128;
  mln_render_session* session = NULL;
  mln_metal_borrowed_texture_descriptor invalid = descriptor;
  invalid.extent.size = sizeof(mln_render_target_extent) - 1;
  invalid.texture = fake_handle;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_metal_borrowed_texture_attach(map, &invalid, &session)
  );
  TEST_ASSERT_NULL(session);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_metal_borrowed_texture_attach(map, &descriptor, &session)
  );
  TEST_ASSERT_NULL(session);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#elif defined(MLN_TEST_BACKEND_OPENGL)

static void configure_opengl_context(mln_opengl_context_descriptor* context) {
#if defined(MLN_TEST_OPENGL_WGL)
  context->platform = MLN_OPENGL_CONTEXT_PLATFORM_WGL;
  context->data.wgl = (mln_wgl_context_descriptor){
    .size = sizeof(mln_wgl_context_descriptor),
    .device_context = fake_handle,
    .share_context = fake_handle,
  };
#else
  context->platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL;
  context->data.egl = (mln_egl_context_descriptor){
    .size = sizeof(mln_egl_context_descriptor),
    .display = fake_handle,
    .config = fake_handle,
    .share_context = fake_handle,
  };
#endif
}
static void shrink_opengl_context(mln_opengl_context_descriptor* context) {
#if defined(MLN_TEST_OPENGL_WGL)
  context->data.wgl.size = sizeof(mln_wgl_context_descriptor) - 1;
#else
  context->data.egl.size = sizeof(mln_egl_context_descriptor) - 1;
#endif
}
static void clear_opengl_context(mln_opengl_context_descriptor* context) {
#if defined(MLN_TEST_OPENGL_WGL)
  context->data.wgl.share_context = NULL;
#else
  context->data.egl.share_context = NULL;
#endif
}
static mln_opengl_surface_descriptor opengl_surface_descriptor(void) {
  mln_opengl_surface_descriptor value = mln_opengl_surface_descriptor_default();
  configure_opengl_context(&value.context);
  value.surface = fake_handle;
  return value;
}
static void clear_opengl_surface(mln_opengl_surface_descriptor* descriptor) {
  descriptor->surface = NULL;
}
static void shrink_opengl_surface(mln_opengl_surface_descriptor* descriptor) {
  shrink_opengl_context(&descriptor->context);
}
static mln_opengl_owned_texture_descriptor opengl_owned_descriptor(void) {
  mln_opengl_owned_texture_descriptor value =
    mln_opengl_owned_texture_descriptor_default();
  configure_opengl_context(&value.context);
  return value;
}
static void clear_opengl_owned(
  mln_opengl_owned_texture_descriptor* descriptor
) {
  clear_opengl_context(&descriptor->context);
}
static void shrink_opengl_owned(
  mln_opengl_owned_texture_descriptor* descriptor
) {
  shrink_opengl_context(&descriptor->context);
}

// This verifies nulls, a non-null output handle, undersized descriptors, and
// missing required OpenGL texture handles.
static void opengl_owned_texture_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_opengl_owned_texture_descriptor, opengl_owned_descriptor,
    mln_opengl_owned_texture_attach, clear_opengl_owned, shrink_opengl_owned
  );
}

// This verifies nulls, a non-null output handle, undersized descriptors, and
// missing required OpenGL surface handles.
static void opengl_surface_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_opengl_surface_descriptor, opengl_surface_descriptor,
    mln_opengl_surface_attach, clear_opengl_surface, shrink_opengl_surface
  );
}

// This verifies nested sizes and required raw texture values that typed OpenGL
// descriptors prevent.
static void opengl_borrowed_texture_rejects_unsafe_raw_descriptors(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_map* map = mln_test_create_map(runtime);
  mln_opengl_borrowed_texture_descriptor descriptor =
    mln_opengl_borrowed_texture_descriptor_default();
  configure_opengl_context(&descriptor.context);
  descriptor.texture = 1;
  descriptor.target = UINT32_C(0x0de1);
  mln_render_session* session = NULL;
  mln_opengl_borrowed_texture_descriptor invalid = descriptor;
  invalid.extent.size = sizeof(mln_render_target_extent) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_opengl_borrowed_texture_attach(map, &invalid, &session)
  );
  TEST_ASSERT_NULL(session);
  invalid = descriptor;
  invalid.context.size = sizeof(mln_opengl_context_descriptor) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_opengl_borrowed_texture_attach(map, &invalid, &session)
  );
  TEST_ASSERT_NULL(session);
  invalid = descriptor;
  invalid.texture = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_opengl_borrowed_texture_attach(map, &invalid, &session)
  );
  TEST_ASSERT_NULL(session);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#elif defined(MLN_TEST_BACKEND_VULKAN)

static mln_vulkan_context_descriptor fake_vulkan_context(void) {
  return (mln_vulkan_context_descriptor){
    .size = sizeof(mln_vulkan_context_descriptor),
    .instance = fake_handle,
    .physical_device = fake_handle,
    .device = fake_handle,
    .graphics_queue = fake_handle,
  };
}
static mln_vulkan_surface_descriptor vulkan_surface_descriptor(void) {
  mln_vulkan_surface_descriptor value = mln_vulkan_surface_descriptor_default();
  value.context = fake_vulkan_context();
  value.surface = fake_handle;
  return value;
}
static void clear_vulkan_surface(mln_vulkan_surface_descriptor* descriptor) {
  descriptor->surface = NULL;
}
static void shrink_vulkan_surface(mln_vulkan_surface_descriptor* descriptor) {
  descriptor->context.size = sizeof(mln_vulkan_context_descriptor) - 1;
}
static mln_vulkan_owned_texture_descriptor vulkan_owned_descriptor(void) {
  mln_vulkan_owned_texture_descriptor value =
    mln_vulkan_owned_texture_descriptor_default();
  value.context = fake_vulkan_context();
  return value;
}
static void clear_vulkan_owned(
  mln_vulkan_owned_texture_descriptor* descriptor
) {
  descriptor->context.device = NULL;
}
static void shrink_vulkan_owned(
  mln_vulkan_owned_texture_descriptor* descriptor
) {
  descriptor->context.size = sizeof(mln_vulkan_context_descriptor) - 1;
}

// This verifies nulls, a non-null output handle, undersized descriptors, and
// missing required Vulkan surface handles.
static void vulkan_surface_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_vulkan_surface_descriptor, vulkan_surface_descriptor,
    mln_vulkan_surface_attach, clear_vulkan_surface, shrink_vulkan_surface
  );
}

// This verifies nulls, a non-null output handle, undersized descriptors, and
// missing required Vulkan texture handles.
static void vulkan_owned_texture_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_vulkan_owned_texture_descriptor, vulkan_owned_descriptor,
    mln_vulkan_owned_texture_attach, clear_vulkan_owned, shrink_vulkan_owned
  );
}

// This verifies nested descriptor sizes and required borrowed Vulkan image
// handles hidden by bindings.
static void vulkan_borrowed_texture_rejects_unsafe_raw_descriptors(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_map* map = mln_test_create_map(runtime);
  mln_vulkan_borrowed_texture_descriptor descriptor =
    mln_vulkan_borrowed_texture_descriptor_default();
  descriptor.context = fake_vulkan_context();
  descriptor.image = fake_handle;
  descriptor.image_view = fake_handle;
  descriptor.format = 37;
  descriptor.initial_layout = 0;
  descriptor.final_layout = 5;
  mln_render_session* session = NULL;
  mln_vulkan_borrowed_texture_descriptor invalid = descriptor;
  invalid.extent.size = sizeof(mln_render_target_extent) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_vulkan_borrowed_texture_attach(map, &invalid, &session)
  );
  TEST_ASSERT_NULL(session);
  invalid = descriptor;
  invalid.context.size = sizeof(mln_vulkan_context_descriptor) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_vulkan_borrowed_texture_attach(map, &invalid, &session)
  );
  TEST_ASSERT_NULL(session);
  invalid = descriptor;
  invalid.image = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_vulkan_borrowed_texture_attach(map, &invalid, &session)
  );
  TEST_ASSERT_NULL(session);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#endif

void run_render_backend_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(webgpu_texture_descriptors_validate_on_host_backends);
#if defined(MLN_TEST_BACKEND_METAL)
  RUN_TEST(metal_surface_attach_rejects_unsafe_raw_inputs);
  RUN_TEST(metal_owned_texture_attach_rejects_unsafe_raw_inputs);
  RUN_TEST(metal_borrowed_texture_rejects_unsafe_raw_descriptors);
#elif defined(MLN_TEST_BACKEND_OPENGL)
  RUN_TEST(opengl_owned_texture_attach_rejects_unsafe_raw_inputs);
  RUN_TEST(opengl_surface_attach_rejects_unsafe_raw_inputs);
  RUN_TEST(opengl_borrowed_texture_rejects_unsafe_raw_descriptors);
#elif defined(MLN_TEST_BACKEND_VULKAN)
  RUN_TEST(vulkan_surface_attach_rejects_unsafe_raw_inputs);
  RUN_TEST(vulkan_owned_texture_attach_rejects_unsafe_raw_inputs);
  RUN_TEST(vulkan_borrowed_texture_rejects_unsafe_raw_descriptors);
#endif
}
