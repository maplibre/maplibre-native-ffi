// OpenGL rings, surfaces, and devices: what attach, set_target, and
// mln_gpu_device_wrap_open_gl() reject before they reach a context, and which
// context providers a build carries. Every build validates an OpenGL target
// before it reports that it renders to none, so these tables run on every
// preset. The contexts name the preset's provider, or EGL on a build without
// OpenGL.

#include "support/attach_table.h"
#include "support/host_graphics.h"
#include "support/test_support.h"

#if defined(MLN_FFI_TEST_OPENGL_WGL)
#define PRESET_PLATFORM MLN_OPEN_GL_CONTEXT_PLATFORM_WGL
#define PRESET_PROVIDERS MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WGL
#elif defined(MLN_FFI_TEST_BACKEND_OPENGL)
#define PRESET_PLATFORM MLN_OPEN_GL_CONTEXT_PLATFORM_EGL
#define PRESET_PROVIDERS MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_EGL
#else
#define PRESET_PLATFORM MLN_OPEN_GL_CONTEXT_PLATFORM_EGL
#define PRESET_PROVIDERS 0U
#endif

// A context of `platform` whose handles only validation reads.
static mln_open_gl_context_descriptor fake_context(uint32_t platform) {
  mln_open_gl_context_descriptor context = {.platform = platform};
  if (platform == MLN_OPEN_GL_CONTEXT_PLATFORM_WGL) {
    context.data.wgl = (mln_wgl_context_descriptor){
      .device_context = MLN_TEST_FAKE_HANDLE,
      .share_context = MLN_TEST_FAKE_HANDLE,
    };
  } else if (platform == MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL) {
    context.data.web_gl = (mln_web_gl_context_descriptor){
      .context = 1,
    };
  } else {
    context.data.egl = (mln_egl_context_descriptor){
      .display = MLN_TEST_FAKE_HANDLE,
      .config = MLN_TEST_FAKE_HANDLE,
      .share_context = MLN_TEST_FAKE_HANDLE,
    };
  }
  return context;
}

MLN_TEST_BORROWED_EDITS(open_gl_borrowed_textures, opengl)

static void ring_without_target(void* call) {
  ((mln_test_target_call*)call)
    ->descriptor.target.data.open_gl_borrowed_textures.target = 0;
}
static void ring_without_texture(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.opengl[0].texture = 0;
  edited->descriptor.target.data.open_gl_borrowed_textures.textures =
    edited->textures.opengl;
}
static void surface_without_surface(void* call) {
  ((mln_test_target_call*)call)
    ->descriptor.target.data.open_gl_surface.surface = NULL;
}
#if defined(MLN_FFI_TEST_BACKEND_OPENGL)
// A replacement that names two textures for a session's ring of one.
static void ring_with_another_depth(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.opengl[1].texture = 2;
  edited->descriptor.target.data.open_gl_borrowed_textures.textures =
    edited->textures.opengl;
  edited->descriptor.target.data.open_gl_borrowed_textures.texture_count = 2;
}
#endif

// A ring of one texture, whose entry is in the call's storage.
static void describe_ring(mln_test_target_call* call) {
  call->textures.opengl[0] = (mln_open_gl_borrowed_texture){.texture = 1};
  call->descriptor.target = (mln_render_target){
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_OPEN_GL_BORROWED_TEXTURES,
  };
  call->descriptor.target.data.open_gl_borrowed_textures =
    (mln_open_gl_borrowed_textures_target){
      .logical_size = mln_test_target_size(),
      .physical_width = 64,
      .physical_height = 64,
      .textures = call->textures.opengl,
      .texture_count = 1,
      // GL_TEXTURE_2D.
      .target = UINT32_C(0x0de1),
  };
}

static void opengl_attach_rejects_malformed_calls(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_target_map(runtime);

  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD);
  describe_ring(&call);
  static const mln_test_validation_case ring_rows[] = {
    MLN_TEST_BORROWED_CASES(open_gl_borrowed_textures),
    {"no texture", ring_without_texture, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"zero target", ring_without_target, MLN_STATUS_INVALID_ARGUMENT,
     "GL_TEXTURE_2D"},
    {"a well-formed ring without a device", NULL, MLN_STATUS_UNSUPPORTED, NULL},
  };
  mln_test_run_attach_table(
    mln_test_submit_target_attach, &call, ring_rows,
    sizeof(ring_rows) / sizeof(ring_rows[0])
  );

  call.descriptor.target = (mln_render_target){
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_OPEN_GL_SURFACE,
    .data.open_gl_surface = {
      .logical_size = mln_test_target_size(), .surface = MLN_TEST_FAKE_HANDLE
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
static void opengl_set_target_rejects_malformed_replacements(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CORE_WORKER);
  call.session = mln_test_create_disposed_session(map);
  describe_ring(&call);
  static const mln_test_validation_case ring_rows[] = {
    MLN_TEST_BORROWED_CASES(open_gl_borrowed_textures),
    {"no texture", ring_without_texture, MLN_STATUS_INVALID_ARGUMENT, NULL},
  };
  mln_test_run_set_target_table(
    mln_test_submit_target_set_target, &call, ring_rows,
    sizeof(ring_rows) / sizeof(ring_rows[0])
  );

#if defined(MLN_FFI_TEST_BACKEND_OPENGL)
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_borrowed_ring(
      map, &fixture, 1, MLN_TEST_PRESET_DRIVER
    ),
    mln_test_graphics_last_error()
  );
  call.session = fixture.session;
  static const mln_test_validation_case session_rows[] = {
    {"more textures than the session's ring has slots", ring_with_another_depth,
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

static mln_status wrap(const mln_open_gl_context_descriptor* context) {
  mln_gpu_device device = MLN_HANDLE_NULL;
  const mln_status status =
    mln_gpu_device_wrap_open_gl(context, &device, MLN_TEST_DIAGNOSTIC);
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
  return status;
}

// mln_gpu_device_wrap_open_gl() checks the context's platform and handles
// before it reaches the platform.
static void wrap_open_gl_rejects_malformed_contexts(void) {
  mln_gpu_device device = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_gpu_device_wrap_open_gl(NULL, &device, NULL));
  const mln_open_gl_context_descriptor fake = fake_context(PRESET_PLATFORM);
  MLN_TEST_INVALID(mln_gpu_device_wrap_open_gl(&fake, NULL, NULL));
  mln_open_gl_context_descriptor edited = fake;
  edited.platform = 7;
  MLN_TEST_INVALID(wrap(&edited));
  edited.platform = MLN_OPEN_GL_CONTEXT_PLATFORM_UNSPECIFIED;
  MLN_TEST_INVALID(wrap(&edited));
  edited = fake_context(MLN_OPEN_GL_CONTEXT_PLATFORM_EGL);
  edited.data.egl.share_context = NULL;
  MLN_TEST_INVALID(wrap(&edited));
  edited = fake_context(MLN_OPEN_GL_CONTEXT_PLATFORM_EGL);
  edited.data.egl.display = NULL;
  MLN_TEST_INVALID(wrap(&edited));
  edited = fake_context(MLN_OPEN_GL_CONTEXT_PLATFORM_WGL);
  edited.data.wgl.share_context = NULL;
  MLN_TEST_INVALID(wrap(&edited));
}

// A build carries its preset's provider, and can carry others beside it, as
// one that renders with Vulkan and OpenGL does. A context of a provider that
// the build lacks doesn't wrap.
static void only_the_builds_providers_wrap(void) {
  const uint32_t supported = mln_open_gl_supported_context_provider_mask();
  TEST_ASSERT_EQUAL_HEX32(PRESET_PROVIDERS, supported & PRESET_PROVIDERS);
  TEST_ASSERT_EQUAL_HEX32(
    0, supported & ~(uint32_t)(MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WGL |
                               MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_EGL |
                               MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WEB_GL)
  );
  static const struct {
    uint32_t platform;
    uint32_t provider;
  } providers[] = {
    {MLN_OPEN_GL_CONTEXT_PLATFORM_EGL, MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_EGL},
    {MLN_OPEN_GL_CONTEXT_PLATFORM_WGL, MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WGL},
    {MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL,
     MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WEB_GL},
  };
  for (size_t index = 0; index < sizeof(providers) / sizeof(providers[0]);
       index += 1) {
    if ((supported & providers[index].provider) != 0) {
      continue;
    }
    const mln_open_gl_context_descriptor context =
      fake_context(providers[index].platform);
    MLN_TEST_STATUS(MLN_STATUS_UNSUPPORTED, wrap(&context));
    TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(mln_test_last_error(), "not supported"), mln_test_last_error()
    );
  }
}

MLN_TEST_GROUP {
  RUN_TEST(opengl_attach_rejects_malformed_calls);
  RUN_TEST(opengl_set_target_rejects_malformed_replacements);
  RUN_TEST(wrap_open_gl_rejects_malformed_contexts);
  RUN_TEST(only_the_builds_providers_wrap);
}
