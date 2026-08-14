// Raw C ABI/backend coverage: render target descriptors expose pointer, size,
// nested descriptor, and output-handle states hidden by bindings.

#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static void* const fake_handle = (void*)(uintptr_t)1;

#define EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(                         \
  descriptor_type, default_descriptor, attach_start, driver_kind,    \
  clear_required, shrink                                             \
)                                                                    \
  do {                                                               \
    mln_render_session session = MLN_HANDLE_NULL;                    \
    mln_operation operation = MLN_HANDLE_NULL;                       \
    mln_render_session_attach_options options =                      \
      mln_render_session_attach_options_default();                   \
    options.driver = driver_kind;                                    \
    descriptor_type descriptor = default_descriptor();               \
    TEST_ASSERT_EQUAL_INT(                                           \
      MLN_STATUS_INVALID_ARGUMENT,                                   \
      attach_start(                                                  \
        MLN_HANDLE_NULL, &descriptor, &options, &session, &operation \
      )                                                              \
    );                                                               \
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);              \
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, operation);            \
    mln_runtime runtime = mln_test_create_runtime();                 \
    mln_map map = mln_test_create_map(runtime);                      \
    TEST_ASSERT_EQUAL_INT(                                           \
      MLN_STATUS_INVALID_ARGUMENT,                                   \
      attach_start(map, NULL, &options, &session, &operation)        \
    );                                                               \
    TEST_ASSERT_EQUAL_INT(                                           \
      MLN_STATUS_INVALID_ARGUMENT,                                   \
      attach_start(map, &descriptor, NULL, &session, &operation)     \
    );                                                               \
    TEST_ASSERT_EQUAL_INT(                                           \
      MLN_STATUS_INVALID_ARGUMENT,                                   \
      attach_start(map, &descriptor, &options, NULL, &operation)     \
    );                                                               \
    TEST_ASSERT_EQUAL_INT(                                           \
      MLN_STATUS_INVALID_ARGUMENT,                                   \
      attach_start(map, &descriptor, &options, &session, NULL)       \
    );                                                               \
    session = 1;                                                     \
    TEST_ASSERT_EQUAL_INT(                                           \
      MLN_STATUS_INVALID_ARGUMENT,                                   \
      attach_start(map, &descriptor, &options, &session, &operation) \
    );                                                               \
    session = MLN_HANDLE_NULL;                                       \
    descriptor_type invalid = default_descriptor();                  \
    invalid.size = sizeof(descriptor_type) - 1;                      \
    TEST_ASSERT_EQUAL_INT(                                           \
      MLN_STATUS_INVALID_ARGUMENT,                                   \
      attach_start(map, &invalid, &options, &session, &operation)    \
    );                                                               \
    invalid = default_descriptor();                                  \
    invalid.extent.size = sizeof(mln_render_target_extent) - 1;      \
    TEST_ASSERT_EQUAL_INT(                                           \
      MLN_STATUS_INVALID_ARGUMENT,                                   \
      attach_start(map, &invalid, &options, &session, &operation)    \
    );                                                               \
    invalid = default_descriptor();                                  \
    invalid.extent.width = UINT32_MAX;                               \
    invalid.extent.scale_factor = 2.0;                               \
    TEST_ASSERT_EQUAL_INT(                                           \
      MLN_STATUS_INVALID_ARGUMENT,                                   \
      attach_start(map, &invalid, &options, &session, &operation)    \
    );                                                               \
    invalid = default_descriptor();                                  \
    shrink(&invalid);                                                \
    TEST_ASSERT_EQUAL_INT(                                           \
      MLN_STATUS_INVALID_ARGUMENT,                                   \
      attach_start(map, &invalid, &options, &session, &operation)    \
    );                                                               \
    invalid = default_descriptor();                                  \
    clear_required(&invalid);                                        \
    TEST_ASSERT_EQUAL_INT(                                           \
      MLN_STATUS_INVALID_ARGUMENT,                                   \
      attach_start(map, &invalid, &options, &session, &operation)    \
    );                                                               \
    mln_test_destroy_map(map);                                       \
    mln_test_destroy_runtime(runtime);                               \
  } while (false)

// Every backend's descriptor validation runs on every build: a descriptor error
// is reported before a missing-backend error, whether or not this build carries
// the backend.

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

static void metal_surface_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_metal_surface_descriptor, metal_surface_descriptor,
    mln_metal_surface_attach_start, MLN_RENDER_DRIVER_CORE_WORKER,
    clear_metal_surface, shrink_metal_surface
  );
}

static void metal_owned_texture_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_metal_owned_texture_descriptor, metal_owned_descriptor,
    mln_metal_owned_texture_attach_start, MLN_RENDER_DRIVER_CORE_WORKER,
    clear_metal_owned, shrink_metal_owned
  );
}

static void metal_borrowed_texture_rejects_unsafe_raw_descriptors(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_metal_borrowed_texture_descriptor descriptor =
    mln_metal_borrowed_texture_descriptor_default();
  descriptor.extent.width = 128;
  descriptor.extent.height = 128;
  mln_render_session session = MLN_HANDLE_NULL;
  mln_metal_borrowed_texture_descriptor invalid = descriptor;
  invalid.extent.size = sizeof(mln_render_target_extent) - 1;
  invalid.texture = fake_handle;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_metal_borrowed_texture_attach_start(
      map, &invalid,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_metal_borrowed_texture_attach_start(
      map, &descriptor,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_webgpu_owned_texture_descriptor webgpu_owned_descriptor(void) {
  mln_webgpu_owned_texture_descriptor value =
    mln_webgpu_owned_texture_descriptor_default();
  value.context.device = fake_handle;
  return value;
}
static void clear_webgpu_owned(
  mln_webgpu_owned_texture_descriptor* descriptor
) {
  descriptor->context.device = NULL;
}
static void shrink_webgpu_owned(
  mln_webgpu_owned_texture_descriptor* descriptor
) {
  descriptor->context.size = sizeof(mln_webgpu_context_descriptor) - 1;
}

static void webgpu_owned_texture_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_webgpu_owned_texture_descriptor, webgpu_owned_descriptor,
    mln_webgpu_owned_texture_attach_start,
    MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, clear_webgpu_owned,
    shrink_webgpu_owned
  );
}

static mln_webgpu_surface_descriptor webgpu_surface_descriptor(void) {
  mln_webgpu_surface_descriptor value = mln_webgpu_surface_descriptor_default();
  value.context.device = fake_handle;
  value.surface = fake_handle;
  // Any non-zero value: this reaches descriptor validation only, which rejects
  // WGPUTextureFormat_Undefined and takes the rest as given.
  value.format = 1;
  return value;
}
static void clear_webgpu_surface(mln_webgpu_surface_descriptor* descriptor) {
  descriptor->surface = NULL;
}
static void shrink_webgpu_surface(mln_webgpu_surface_descriptor* descriptor) {
  descriptor->context.size = sizeof(mln_webgpu_context_descriptor) - 1;
}

static void webgpu_surface_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_webgpu_surface_descriptor, webgpu_surface_descriptor,
    mln_webgpu_surface_attach_start, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
    clear_webgpu_surface, shrink_webgpu_surface
  );
}

// A surface with no format cannot be configured, so attach rejects it instead
// of leaving the browser to report it.
static void webgpu_surface_attach_rejects_an_unspecified_format(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_webgpu_surface_descriptor descriptor = webgpu_surface_descriptor();
  descriptor.format = 0;
  mln_render_session session = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_webgpu_surface_attach_start(
      map, &descriptor,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void webgpu_borrowed_texture_rejects_unsafe_raw_descriptors(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_webgpu_borrowed_texture_descriptor descriptor =
    mln_webgpu_borrowed_texture_descriptor_default();
  descriptor.context.device = fake_handle;
  descriptor.texture = fake_handle;
  descriptor.texture_view = fake_handle;
  descriptor.format = 18;
  mln_render_session session = MLN_HANDLE_NULL;

  mln_webgpu_borrowed_texture_descriptor invalid = descriptor;
  invalid.extent.size = sizeof(mln_render_target_extent) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_webgpu_borrowed_texture_attach_start(
      map, &invalid,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  invalid = descriptor;
  invalid.physical_width = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_webgpu_borrowed_texture_attach_start(
      map, &invalid,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  invalid = descriptor;
  invalid.context.device = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_webgpu_borrowed_texture_attach_start(
      map, &invalid,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  invalid = descriptor;
  invalid.texture_view = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_webgpu_borrowed_texture_attach_start(
      map, &invalid,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  invalid = descriptor;
  invalid.format = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_webgpu_borrowed_texture_attach_start(
      map, &invalid,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void configure_opengl_context(mln_opengl_context_descriptor* context) {
#if defined(MLN_FFI_TEST_OPENGL_WGL)
  context->platform = MLN_OPENGL_CONTEXT_PLATFORM_WGL;
  context->data.wgl = (mln_wgl_context_descriptor){
    .size = sizeof(mln_wgl_context_descriptor),
    .device_context = fake_handle,
    .share_context = fake_handle,
  };
#elif defined(MLN_FFI_TEST_OPENGL_WEBGL)
  context->platform = MLN_OPENGL_CONTEXT_PLATFORM_WEBGL;
  context->data.webgl = (mln_webgl_context_descriptor){
    .size = sizeof(mln_webgl_context_descriptor),
    // Not a live context: these tests only reach descriptor validation.
    .context = 1,
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
#if defined(MLN_FFI_TEST_OPENGL_WGL)
  context->data.wgl.size = sizeof(mln_wgl_context_descriptor) - 1;
#elif defined(MLN_FFI_TEST_OPENGL_WEBGL)
  context->data.webgl.size = sizeof(mln_webgl_context_descriptor) - 1;
#else
  context->data.egl.size = sizeof(mln_egl_context_descriptor) - 1;
#endif
}
static void clear_opengl_context(mln_opengl_context_descriptor* context) {
#if defined(MLN_FFI_TEST_OPENGL_WGL)
  context->data.wgl.share_context = NULL;
#elif defined(MLN_FFI_TEST_OPENGL_WEBGL)
  context->data.webgl.context = 0;
#else
  context->data.egl.share_context = NULL;
#endif
}
// WGL and EGL need a surface alongside the context, so the surface is the
// handle these check for. A WebGL context names its canvas itself and takes a
// null surface, so there the context handle is the required one.
static mln_opengl_surface_descriptor opengl_surface_descriptor(void) {
  mln_opengl_surface_descriptor value = mln_opengl_surface_descriptor_default();
  configure_opengl_context(&value.context);
#if !defined(MLN_FFI_TEST_OPENGL_WEBGL)
  value.surface = fake_handle;
#endif
  return value;
}
static void clear_opengl_surface(mln_opengl_surface_descriptor* descriptor) {
#if defined(MLN_FFI_TEST_OPENGL_WEBGL)
  clear_opengl_context(&descriptor->context);
#else
  descriptor->surface = NULL;
#endif
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

static void opengl_owned_texture_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_opengl_owned_texture_descriptor, opengl_owned_descriptor,
    mln_opengl_owned_texture_attach_start,
    MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, clear_opengl_owned,
    shrink_opengl_owned
  );
}

static void opengl_surface_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_opengl_surface_descriptor, opengl_surface_descriptor,
    mln_opengl_surface_attach_start, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
    clear_opengl_surface, shrink_opengl_surface
  );
}

#if defined(MLN_FFI_TEST_OPENGL_WEBGL)
// A WebGL context carries the canvas it presents to, so a surface handle
// alongside it names a second target the session would ignore.
static void opengl_surface_attach_rejects_a_webgl_surface_handle(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_opengl_surface_descriptor descriptor = opengl_surface_descriptor();
  descriptor.surface = fake_handle;
  mln_render_session session = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_opengl_surface_attach_start(
      map, &descriptor,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

// A dedicated OpenGL context names no share group and takes its client API from
// the descriptor, so a descriptor carrying either of the shared session's
// answers is contradictory.
static void opengl_dedicated_context_rejects_shared_session_fields(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  mln_opengl_surface_descriptor with_share = opengl_surface_descriptor();
  with_share.context.ownership = MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED;
  with_share.context.data.egl.client_api = MLN_OPENGL_CLIENT_API_GLES;
  with_share.context.data.egl.share_context = fake_handle;
  with_share.context.data.wgl.share_context = fake_handle;
  mln_render_session session = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_opengl_surface_attach_start(
      map, &with_share,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A texture session hands its texture to a host that samples it from the host's
// own context, which is what the share group the descriptor names is for.
static void opengl_owned_texture_attach_rejects_a_dedicated_context(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_opengl_owned_texture_descriptor descriptor = opengl_owned_descriptor();
  descriptor.context.ownership = MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED;
  mln_render_session session = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_opengl_owned_texture_attach_start(
      map, &descriptor,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static const char dedicated_background_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":"
  "[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#ff0000\"}}]}";

// The contract a dedicated session offers a host: it creates its own context
// from a descriptor that names no share context, renders through it, and leaves
// it current so the next render costs no EGL call.
static void dedicated_egl_surface_renders_and_keeps_its_context_current(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  const mln_test_fixture_result fixture_result =
    mln_test_dedicated_egl_surface_create(map, &fixture);
  if (fixture_result == MLN_TEST_FIXTURE_UNAVAILABLE) {
    mln_test_destroy_map(map);
    mln_test_destroy_runtime(runtime);
    TEST_IGNORE_MESSAGE("this build has no EGL context provider");
    return;
  }
  // Attaching with no share context is the behavior under test, so a rejection
  // here is a failure rather than a reason to skip.
  TEST_ASSERT_EQUAL_INT(MLN_TEST_FIXTURE_OK, fixture_result);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_map_set_style_json(
                     map, MLN_BUFFER_LITERAL(dedicated_background_style_json)
                   )
  );
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_PRESENT;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture.session, &demand)
  );
  mln_operation barrier = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_barrier_start(fixture.session, 0, &barrier)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, barrier)
  );
  mln_operation_release(barrier);
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_drain_frame_results(fixture.session, SIZE_MAX, &batch)
  );
  mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_frame_batch_get(batch, 0, &result)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_TRUE(mln_test_egl_context_is_current());

  mln_test_dedicated_egl_surface_destroy(&fixture);
  // Destroying the session releases the thread it had taken over.
  TEST_ASSERT_FALSE(mln_test_egl_context_is_current());
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void opengl_borrowed_texture_rejects_unsafe_raw_descriptors(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_opengl_borrowed_texture_descriptor descriptor =
    mln_opengl_borrowed_texture_descriptor_default();
  configure_opengl_context(&descriptor.context);
  descriptor.texture = 1;
  descriptor.target = UINT32_C(0x0de1);
  mln_render_session session = MLN_HANDLE_NULL;
  mln_opengl_borrowed_texture_descriptor invalid = descriptor;
  invalid.extent.size = sizeof(mln_render_target_extent) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_opengl_borrowed_texture_attach_start(
      map, &invalid,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  invalid = descriptor;
  invalid.context.size = sizeof(mln_opengl_context_descriptor) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_opengl_borrowed_texture_attach_start(
      map, &invalid,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  invalid = descriptor;
  invalid.texture = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_opengl_borrowed_texture_attach_start(
      map, &invalid,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

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

static void vulkan_surface_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_vulkan_surface_descriptor, vulkan_surface_descriptor,
    mln_vulkan_surface_attach_start, MLN_RENDER_DRIVER_CORE_WORKER,
    clear_vulkan_surface, shrink_vulkan_surface
  );
}

static void vulkan_owned_texture_attach_rejects_unsafe_raw_inputs(void) {
  EXPECT_ATTACH_REJECTS_UNSAFE_INPUTS(
    mln_vulkan_owned_texture_descriptor, vulkan_owned_descriptor,
    mln_vulkan_owned_texture_attach_start, MLN_RENDER_DRIVER_CORE_WORKER,
    clear_vulkan_owned, shrink_vulkan_owned
  );
}

static void vulkan_borrowed_texture_rejects_unsafe_raw_descriptors(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_vulkan_borrowed_texture_descriptor descriptor =
    mln_vulkan_borrowed_texture_descriptor_default();
  descriptor.context = fake_vulkan_context();
  descriptor.image = fake_handle;
  descriptor.image_view = fake_handle;
  descriptor.format = 37;
  descriptor.initial_layout = 0;
  descriptor.final_layout = 5;
  mln_render_session session = MLN_HANDLE_NULL;
  mln_vulkan_borrowed_texture_descriptor invalid = descriptor;
  invalid.extent.size = sizeof(mln_render_target_extent) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_vulkan_borrowed_texture_attach_start(
      map, &invalid,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  invalid = descriptor;
  invalid.context.size = sizeof(mln_vulkan_context_descriptor) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_vulkan_borrowed_texture_attach_start(
      map, &invalid,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  invalid = descriptor;
  invalid.image = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_vulkan_borrowed_texture_attach_start(
      map, &invalid,
      &(mln_render_session_attach_options){
        .size = sizeof(mln_render_session_attach_options),
        .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      },
      &session, &(mln_operation){MLN_HANDLE_NULL}
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_render_backend_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(metal_surface_attach_rejects_unsafe_raw_inputs);
  RUN_TEST(metal_owned_texture_attach_rejects_unsafe_raw_inputs);
  RUN_TEST(metal_borrowed_texture_rejects_unsafe_raw_descriptors);
  RUN_TEST(opengl_surface_attach_rejects_unsafe_raw_inputs);
#if defined(MLN_FFI_TEST_OPENGL_WEBGL)
  RUN_TEST(opengl_surface_attach_rejects_a_webgl_surface_handle);
#endif
  RUN_TEST(opengl_dedicated_context_rejects_shared_session_fields);
  RUN_TEST(opengl_owned_texture_attach_rejects_a_dedicated_context);
  RUN_TEST(dedicated_egl_surface_renders_and_keeps_its_context_current);
  RUN_TEST(opengl_owned_texture_attach_rejects_unsafe_raw_inputs);
  RUN_TEST(opengl_borrowed_texture_rejects_unsafe_raw_descriptors);
  RUN_TEST(vulkan_surface_attach_rejects_unsafe_raw_inputs);
  RUN_TEST(vulkan_owned_texture_attach_rejects_unsafe_raw_inputs);
  RUN_TEST(vulkan_borrowed_texture_rejects_unsafe_raw_descriptors);
  RUN_TEST(webgpu_owned_texture_attach_rejects_unsafe_raw_inputs);
  RUN_TEST(webgpu_surface_attach_rejects_unsafe_raw_inputs);
  RUN_TEST(webgpu_surface_attach_rejects_an_unspecified_format);
  RUN_TEST(webgpu_borrowed_texture_rejects_unsafe_raw_descriptors);
}
