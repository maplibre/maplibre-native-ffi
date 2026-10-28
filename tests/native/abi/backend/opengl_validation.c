// OpenGL target descriptors: what attach and set_target reject before they
// reach a context, and which context providers a build carries. Every build
// validates an OpenGL descriptor before it reports that it carries no OpenGL
// backend, so these tables run on every preset. The descriptors name the
// preset's provider, or EGL on a build without OpenGL.

#include "support/attach_table.h"
#include "support/host_graphics.h"
#include "support/test_support.h"

#if defined(MLN_FFI_TEST_OPENGL_WGL)
#define PRESET_PLATFORM MLN_OPEN_GL_CONTEXT_PLATFORM_WGL
#define PRESET_PROVIDERS MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WGL
#elif defined(MLN_FFI_TEST_OPENGL_WEBGL)
#define PRESET_PLATFORM MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL
#define PRESET_PROVIDERS MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_WEB_GL
#elif defined(MLN_FFI_TEST_BACKEND_OPENGL)
#define PRESET_PLATFORM MLN_OPEN_GL_CONTEXT_PLATFORM_EGL
#define PRESET_PROVIDERS MLN_OPEN_GL_CONTEXT_PROVIDER_FLAG_EGL
#else
#define PRESET_PLATFORM MLN_OPEN_GL_CONTEXT_PLATFORM_EGL
#define PRESET_PROVIDERS 0U
#endif

// A shared context of `platform` whose handles only validation reads.
static mln_open_gl_context_descriptor fake_context(uint32_t platform) {
  mln_open_gl_context_descriptor context = {
    .platform = platform,
    .ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_SHARED,
  };
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

// The one handle a shared context needs beyond the display or device.
static void clear_shared_context(mln_open_gl_context_descriptor* context) {
#if defined(MLN_FFI_TEST_OPENGL_WGL)
  context->data.wgl.share_context = NULL;
#elif defined(MLN_FFI_TEST_OPENGL_WEBGL)
  context->data.web_gl.context = 0;
#else
  context->data.egl.share_context = NULL;
#endif
}

MLN_TEST_DESCRIPTOR_EDITS(opengl_surface, mln_open_gl_surface_descriptor)
MLN_TEST_OVERFLOW_EDIT(opengl_surface)
MLN_TEST_DESCRIPTOR_EDITS(opengl_owned, mln_open_gl_owned_texture_descriptor)
MLN_TEST_OVERFLOW_EDIT(opengl_owned)
MLN_TEST_DESCRIPTOR_EDITS(
  opengl_borrowed, mln_open_gl_borrowed_texture_descriptor
)
MLN_TEST_BORROWED_EDITS(opengl_borrowed, opengl)

static mln_open_gl_surface_descriptor* surface_of(void* call) {
  return &((mln_test_target_call*)call)->descriptor.opengl_surface;
}
static mln_open_gl_owned_texture_descriptor* owned_of(void* call) {
  return &((mln_test_target_call*)call)->descriptor.opengl_owned;
}
static mln_open_gl_borrowed_texture_descriptor* borrowed_of(void* call) {
  return &((mln_test_target_call*)call)->descriptor.opengl_borrowed;
}

// WGL and EGL present to a surface alongside the context. A WebGL context
// names its canvas itself and takes no surface, so there the context is the
// required handle.
static void surface_without_its_drawable(void* call) {
#if defined(MLN_FFI_TEST_OPENGL_WEBGL)
  clear_shared_context(&surface_of(call)->context);
#else
  surface_of(call)->surface = NULL;
#endif
}
static void surface_with_unknown_ownership(void* call) {
  surface_of(call)->context.ownership = 7;
}
static void surface_with_unknown_platform(void* call) {
  surface_of(call)->context.platform = 7;
}
static void surface_with_unspecified_platform(void* call) {
  surface_of(call)->context.platform = MLN_OPEN_GL_CONTEXT_PLATFORM_UNSPECIFIED;
}
#if !defined(MLN_FFI_TEST_OPENGL_WEBGL)
// A dedicated context joins no share group, so naming one contradicts it.
static void dedicated_surface_with_share_context(void* call) {
  mln_open_gl_context_descriptor* context = &surface_of(call)->context;
  context->ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED;
#if !defined(MLN_FFI_TEST_OPENGL_WGL)
  context->data.egl.client_api = MLN_OPEN_GL_CLIENT_API_GLES;
#endif
}
#else
// A WebGL context carries its canvas, so a surface names a second target.
static void webgl_surface_with_surface_handle(void* call) {
  surface_of(call)->surface = MLN_TEST_FAKE_HANDLE;
}
#endif
static void owned_without_shared_context(void* call) {
  clear_shared_context(&owned_of(call)->context);
}
#if !defined(MLN_FFI_TEST_OPENGL_WGL) && !defined(MLN_FFI_TEST_OPENGL_WEBGL)
static void owned_without_egl_display(void* call) {
  owned_of(call)->context.data.egl.display = NULL;
}
#endif
// A pooled EGL context whose handles only validation reads.
static void pool_context(mln_open_gl_context_descriptor* context) {
  *context = fake_context(MLN_OPEN_GL_CONTEXT_PLATFORM_EGL);
  context->ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_POOLED;
  context->data.egl.client_api = MLN_OPEN_GL_CLIENT_API_GLES;
}
static void pooled_owned(void* call) { pool_context(&owned_of(call)->context); }
static void pooled_owned_on_wgl(void* call) {
  mln_open_gl_context_descriptor* context = &owned_of(call)->context;
  *context = fake_context(MLN_OPEN_GL_CONTEXT_PLATFORM_WGL);
  context->ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_POOLED;
}
#if defined(MLN_FFI_TEST_OPENGL_EGL)
// A pooled session renders in the context it names, with the API it names.
static void pooled_owned_without_its_context(void* call) {
  pool_context(&owned_of(call)->context);
  owned_of(call)->context.data.egl.share_context = NULL;
}
static void pooled_owned_without_client_api(void* call) {
  pool_context(&owned_of(call)->context);
  owned_of(call)->context.data.egl.client_api =
    MLN_OPEN_GL_CLIENT_API_UNSPECIFIED;
}
static void pooled_surface(void* call) {
  pool_context(&surface_of(call)->context);
}
#endif
static void borrowed_without_target(void* call) {
  borrowed_of(call)->target = 0;
}
static void borrowed_without_texture(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.opengl[0].texture = 0;
  borrowed_of(call)->textures = edited->textures.opengl;
}
#if defined(MLN_FFI_TEST_BACKEND_OPENGL) && !defined(MLN_FFI_TEST_OPENGL_WEBGL)
// A replacement that names two textures for a session's ring of one.
static void borrowed_with_another_depth(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.opengl[1].texture = 2;
  borrowed_of(call)->textures = edited->textures.opengl;
  borrowed_of(call)->texture_count = 2;
}
#endif
static void borrowed_without_physical_width(void* call) {
  borrowed_of(call)->physical_width = 0;
}

MLN_TEST_ATTACH_SUBMITTER(submit_surface_attach, mln_map_attach_open_gl_surface)
MLN_TEST_ATTACH_SUBMITTER(
  submit_owned_attach, mln_map_attach_open_gl_owned_texture
)
MLN_TEST_ATTACH_SUBMITTER(
  submit_borrowed_attach, mln_map_attach_open_gl_borrowed_texture
)
#if !defined(MLN_FFI_TEST_OPENGL_WEBGL)
MLN_TEST_SET_TARGET_SUBMITTER(
  submit_surface_set_target, mln_render_session_set_open_gl_surface_target
)
MLN_TEST_SET_TARGET_SUBMITTER(
  submit_borrowed_set_target,
  mln_render_session_set_open_gl_borrowed_texture_target
)
#endif

static mln_open_gl_surface_descriptor surface_descriptor(void) {
  mln_open_gl_surface_descriptor descriptor = {
    .size = sizeof(mln_open_gl_surface_descriptor)
  };
  descriptor.extent = mln_test_target_extent();
  descriptor.context = fake_context(PRESET_PLATFORM);
#if !defined(MLN_FFI_TEST_OPENGL_WEBGL)
  descriptor.surface = MLN_TEST_FAKE_HANDLE;
#endif
  return descriptor;
}

static mln_open_gl_owned_texture_descriptor owned_descriptor(
  uint32_t platform
) {
  mln_open_gl_owned_texture_descriptor descriptor = {
    .size = sizeof(mln_open_gl_owned_texture_descriptor)
  };
  descriptor.extent = mln_test_target_extent();
  descriptor.context = fake_context(platform);
  return descriptor;
}

// A ring of one texture, whose entry is in the call's storage.
static void describe_borrowed(mln_test_target_call* call) {
  call->textures.opengl[0] = (mln_open_gl_borrowed_texture){.texture = 1};
  mln_open_gl_borrowed_texture_descriptor* descriptor =
    &call->descriptor.opengl_borrowed;
  *descriptor = (mln_open_gl_borrowed_texture_descriptor){
    .size = sizeof(mln_open_gl_borrowed_texture_descriptor)
  };
  descriptor->extent = mln_test_target_extent();
  descriptor->context = fake_context(PRESET_PLATFORM);
  descriptor->textures = call->textures.opengl;
  descriptor->texture_count = 1;
  // GL_TEXTURE_2D.
  descriptor->target = UINT32_C(0x0de1);
  descriptor->physical_width = 64;
  descriptor->physical_height = 64;
}

static void opengl_attach_rejects_malformed_calls(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD);
  call.descriptor.opengl_surface = surface_descriptor();
  static const mln_test_validation_case surface_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(opengl_surface),
    MLN_TEST_OVERFLOW_CASE(opengl_surface),
    {"no drawable", surface_without_its_drawable, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"unknown ownership", surface_with_unknown_ownership,
     MLN_STATUS_INVALID_ARGUMENT, "ownership is unknown"},
    {"unknown platform", surface_with_unknown_platform,
     MLN_STATUS_INVALID_ARGUMENT, "platform is invalid"},
    {"unspecified platform", surface_with_unspecified_platform,
     MLN_STATUS_INVALID_ARGUMENT, "platform is invalid"},
#if defined(MLN_FFI_TEST_OPENGL_WEBGL)
    {"a surface beside a WebGL context", webgl_surface_with_surface_handle,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
#else
    {"a dedicated context naming a share group",
     dedicated_surface_with_share_context, MLN_STATUS_INVALID_ARGUMENT,
     "joins no share group"},
#endif
#if defined(MLN_FFI_TEST_OPENGL_EGL)
    {"a pooled context", pooled_surface, MLN_STATUS_INVALID_ARGUMENT,
     "renders only owned textures"},
#endif
#if !defined(MLN_FFI_TEST_BACKEND_OPENGL)
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_attach_table(
    submit_surface_attach, &call, surface_rows,
    sizeof(surface_rows) / sizeof(surface_rows[0])
  );

  call.descriptor.opengl_owned = owned_descriptor(PRESET_PLATFORM);
  static const mln_test_validation_case owned_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(opengl_owned),
    MLN_TEST_OVERFLOW_CASE(opengl_owned),
    {"no shared context", owned_without_shared_context,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
#if !defined(MLN_FFI_TEST_OPENGL_WGL) && !defined(MLN_FFI_TEST_OPENGL_WEBGL)
    {"no EGL display", owned_without_egl_display, MLN_STATUS_INVALID_ARGUMENT,
     "EGL display and config must not be null"},
#endif
#if defined(MLN_FFI_TEST_OPENGL_EGL)
    {"a pooled context without its context", pooled_owned_without_its_context,
     MLN_STATUS_INVALID_ARGUMENT, "both are required"},
    {"a pooled context without a client API", pooled_owned_without_client_api,
     MLN_STATUS_INVALID_ARGUMENT, "both are required"},
#endif
    {"a pooled WGL context", pooled_owned_on_wgl, MLN_STATUS_INVALID_ARGUMENT,
     "only an EGL context can be pooled"},
    // A pooled session renders on its own core worker.
    {"a pooled context on the caller thread", pooled_owned,
     MLN_STATUS_UNSUPPORTED, NULL},
#if !defined(MLN_FFI_TEST_BACKEND_OPENGL)
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_attach_table(
    submit_owned_attach, &call, owned_rows,
    sizeof(owned_rows) / sizeof(owned_rows[0])
  );

  describe_borrowed(&call);
  static const mln_test_validation_case borrowed_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(opengl_borrowed),
    MLN_TEST_BORROWED_CASES(opengl_borrowed),
    {"no texture", borrowed_without_texture, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"zero target", borrowed_without_target, MLN_STATUS_INVALID_ARGUMENT,
     "target must be specified"},
    {"zero physical width", borrowed_without_physical_width,
     MLN_STATUS_INVALID_ARGUMENT, "physical texture dimensions"},
#if !defined(MLN_FFI_TEST_BACKEND_OPENGL)
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

// The build reports the providers it carries, and refuses a well-formed
// context of any other.
static void only_the_builds_providers_attach(void) {
  TEST_ASSERT_EQUAL_HEX32(
    PRESET_PROVIDERS, mln_open_gl_supported_context_provider_mask()
  );
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
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
    if ((PRESET_PROVIDERS & providers[index].provider) != 0) {
      continue;
    }
    mln_test_target_call call = mln_test_target_call_default(
      map, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
    );
    call.descriptor.opengl_owned = owned_descriptor(providers[index].platform);
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED,
      submit_owned_attach(NULL, &call, MLN_TEST_DIAGNOSTIC)
    );
    TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(mln_test_last_error(), "not supported"), mln_test_last_error()
    );
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#if !defined(MLN_FFI_TEST_OPENGL_WEBGL)
// set_target runs its attach's descriptor check, so beyond a row for each
// pointer and handle it reads, these rows cover only its own steps. It checks
// the session before the descriptor, and a build without OpenGL reports the
// missing backend last, so there the rows name no session. An OpenGL build
// submits them for a live session of the replacement's kind; a WebGL build has
// no host graphics for one.
static void opengl_set_target_rejects_malformed_descriptors(void) {
  mln_test_target_call call = mln_test_target_call_default(
    MLN_HANDLE_NULL, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
  );
#if defined(MLN_FFI_TEST_BACKEND_OPENGL)
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_surface(map, &fixture),
    mln_test_graphics_last_error()
  );
  call.session = fixture.session;
#endif
  call.descriptor.opengl_surface = surface_descriptor();
  static const mln_test_validation_case surface_rows[] = {
    {"null descriptor", mln_test_call_without_descriptor,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    MLN_TEST_UNDERSIZED_CASE(opengl_surface),
    MLN_TEST_OVERFLOW_CASE(opengl_surface),
    {"no drawable", surface_without_its_drawable, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
#if !defined(MLN_FFI_TEST_BACKEND_OPENGL)
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_validation_table(
    surface_rows, sizeof(surface_rows) / sizeof(surface_rows[0]), &call,
    sizeof(call), submit_surface_set_target, NULL
  );

#if defined(MLN_FFI_TEST_BACKEND_OPENGL)
  mln_test_render_fixture_destroy(&fixture);
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_borrowed_texture(map, &fixture),
    mln_test_graphics_last_error()
  );
  call.session = fixture.session;
#endif
  describe_borrowed(&call);
  static const mln_test_validation_case borrowed_rows[] = {
    {"null descriptor", mln_test_call_without_descriptor,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    MLN_TEST_UNDERSIZED_CASE(opengl_borrowed),
    {"null textures array", opengl_borrowed_without_textures,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"no texture", borrowed_without_texture, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"zero physical width", borrowed_without_physical_width,
     MLN_STATUS_INVALID_ARGUMENT, "physical texture dimensions"},
#if defined(MLN_FFI_TEST_BACKEND_OPENGL)
    {"more textures than the session's ring has slots",
     borrowed_with_another_depth, MLN_STATUS_INVALID_ARGUMENT, "ring depth"},
#else
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_validation_table(
    borrowed_rows, sizeof(borrowed_rows) / sizeof(borrowed_rows[0]), &call,
    sizeof(call), submit_borrowed_set_target, NULL
  );
#if defined(MLN_FFI_TEST_BACKEND_OPENGL)
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
#endif
}
#endif

MLN_TEST_GROUP {
  RUN_TEST(opengl_attach_rejects_malformed_calls);
  RUN_TEST(only_the_builds_providers_attach);
#if !defined(MLN_FFI_TEST_OPENGL_WEBGL)
  RUN_TEST(opengl_set_target_rejects_malformed_descriptors);
#endif
}
