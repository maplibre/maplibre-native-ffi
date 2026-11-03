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
MLN_TEST_DESCRIPTOR_EDITS(
  opengl_borrowed, mln_open_gl_borrowed_texture_descriptor
)
MLN_TEST_BORROWED_EDITS(opengl_borrowed, opengl)
MLN_TEST_DESCRIPTOR_EDITS(
  opengl_surface_replacement, mln_open_gl_surface_replacement
)
MLN_TEST_DESCRIPTOR_EDITS(
  opengl_borrowed_replacement, mln_open_gl_borrowed_texture_replacement
)
MLN_TEST_BORROWED_EDITS(opengl_borrowed_replacement, opengl)

static mln_open_gl_surface_descriptor* surface_of(void* call) {
  return &((mln_test_target_call*)call)->descriptor.opengl_surface;
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
static void borrowed_without_shared_context(void* call) {
  clear_shared_context(&borrowed_of(call)->context);
}
#if !defined(MLN_FFI_TEST_OPENGL_WGL) && !defined(MLN_FFI_TEST_OPENGL_WEBGL)
static void borrowed_without_egl_display(void* call) {
  borrowed_of(call)->context.data.egl.display = NULL;
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
static void borrowed_without_physical_width(void* call) {
  borrowed_of(call)->physical_width = 0;
}

static mln_open_gl_borrowed_texture_replacement* replacement_of(void* call) {
  return &((mln_test_target_call*)call)->descriptor.opengl_borrowed_replacement;
}
#if defined(MLN_FFI_TEST_OPENGL_WEBGL)
// A WebGL context names its canvas, so a replacement names no surface.
static void replacement_with_surface(void* call) {
  ((mln_test_target_call*)call)->descriptor.opengl_surface_replacement.surface =
    MLN_TEST_FAKE_HANDLE;
}
#elif defined(MLN_FFI_TEST_BACKEND_OPENGL)
// WGL and EGL present to a surface, which the replacement names.
static void replacement_without_surface(void* call) {
  ((mln_test_target_call*)call)->descriptor.opengl_surface_replacement.surface =
    NULL;
}
// A replacement that names two textures for a session's ring of one.
static void replacement_with_another_depth(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.opengl[1].texture = 2;
  replacement_of(call)->textures = edited->textures.opengl;
  replacement_of(call)->texture_count = 2;
}
#endif
static void replacement_without_texture(void* call) {
  mln_test_target_call* edited = call;
  edited->textures.opengl[0].texture = 0;
  replacement_of(call)->textures = edited->textures.opengl;
}
static void replacement_without_physical_width(void* call) {
  replacement_of(call)->physical_width = 0;
}

MLN_TEST_ATTACH_SUBMITTER(submit_surface_attach, mln_map_attach_open_gl_surface)
MLN_TEST_ATTACH_SUBMITTER(
  submit_borrowed_attach, mln_map_attach_open_gl_borrowed_texture
)
MLN_TEST_SET_TARGET_SUBMITTER(
  submit_surface_set_target, mln_render_session_set_open_gl_surface_target
)
MLN_TEST_SET_TARGET_SUBMITTER(
  submit_borrowed_set_target,
  mln_render_session_set_open_gl_borrowed_texture_target
)

static mln_open_gl_surface_descriptor surface_descriptor(void) {
  mln_open_gl_surface_descriptor descriptor = {
    .size = sizeof(mln_open_gl_surface_descriptor)
  };
  descriptor.logical_size = mln_test_target_size();
  descriptor.context = fake_context(PRESET_PLATFORM);
#if !defined(MLN_FFI_TEST_OPENGL_WEBGL)
  descriptor.surface = MLN_TEST_FAKE_HANDLE;
#endif
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
  descriptor->logical_size = mln_test_target_size();
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
  mln_map map = mln_test_create_target_map(runtime);

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
#if !defined(MLN_FFI_TEST_BACKEND_OPENGL)
    {"a well-formed descriptor", NULL, MLN_STATUS_UNSUPPORTED,
     "not supported by this build"},
#endif
  };
  mln_test_run_attach_table(
    submit_surface_attach, &call, surface_rows,
    sizeof(surface_rows) / sizeof(surface_rows[0])
  );

  describe_borrowed(&call);
  static const mln_test_validation_case borrowed_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(opengl_borrowed),
    MLN_TEST_BORROWED_CASES(opengl_borrowed),
    {"no shared context", borrowed_without_shared_context,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
#if !defined(MLN_FFI_TEST_OPENGL_WGL) && !defined(MLN_FFI_TEST_OPENGL_WEBGL)
    {"no EGL display", borrowed_without_egl_display,
     MLN_STATUS_INVALID_ARGUMENT, "EGL display and config must not be null"},
#endif
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
    describe_borrowed(&call);
    call.descriptor.opengl_borrowed.context =
      fake_context(providers[index].platform);
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED,
      submit_borrowed_attach(NULL, &call, MLN_TEST_DIAGNOSTIC)
    );
    TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(mln_test_last_error(), "not supported"), mln_test_last_error()
    );
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A ring of one replacement texture, whose entry is in the call's storage.
static void describe_borrowed_replacement(mln_test_target_call* call) {
  call->textures.opengl[0] = (mln_open_gl_borrowed_texture){.texture = 1};
  *replacement_of(call) = (mln_open_gl_borrowed_texture_replacement){
    .size = sizeof(mln_open_gl_borrowed_texture_replacement),
    .logical_size = mln_test_target_size(),
    .physical_width = 64,
    .physical_height = 64,
    .textures = call->textures.opengl,
    .texture_count = 1,
  };
}

// set_target checks its replacement, so these rows cover a row for each
// pointer and handle it reads, and its own steps.
static void opengl_set_target_rejects_malformed_replacements(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_target_call call =
    mln_test_target_call_default(map, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD);
  call.session = mln_test_create_disposed_session(map);
  call.descriptor.opengl_surface_replacement =
    (mln_open_gl_surface_replacement){
      .size = sizeof(mln_open_gl_surface_replacement),
      .logical_size = mln_test_target_size(),
#if !defined(MLN_FFI_TEST_OPENGL_WEBGL)
      .surface = MLN_TEST_FAKE_HANDLE,
#endif
  };
  static const mln_test_validation_case surface_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(opengl_surface_replacement),
#if defined(MLN_FFI_TEST_OPENGL_WEBGL)
    {"a surface beside a WebGL context", replacement_with_surface,
     MLN_STATUS_INVALID_ARGUMENT, "WebGL surface must be null"},
#elif defined(MLN_FFI_TEST_BACKEND_OPENGL)
    {"no surface", replacement_without_surface, MLN_STATUS_INVALID_ARGUMENT,
     "must not be null"},
#endif
  };
  mln_test_run_set_target_table(
    submit_surface_set_target, &call, surface_rows,
    sizeof(surface_rows) / sizeof(surface_rows[0]),
    MLN_RENDER_BACKEND_FLAG_OPEN_GL
  );

  describe_borrowed_replacement(&call);
  static const mln_test_validation_case borrowed_rows[] = {
    MLN_TEST_DESCRIPTOR_CASES(opengl_borrowed_replacement),
    MLN_TEST_BORROWED_CASES(opengl_borrowed_replacement),
    {"no texture", replacement_without_texture, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"zero physical width", replacement_without_physical_width,
     MLN_STATUS_INVALID_ARGUMENT, "physical texture dimensions"},
  };
  mln_test_run_set_target_table(
    submit_borrowed_set_target, &call, borrowed_rows,
    sizeof(borrowed_rows) / sizeof(borrowed_rows[0]),
    MLN_RENDER_BACKEND_FLAG_OPEN_GL
  );

  // A WebGL build has no host graphics for a ring of its own.
#if defined(MLN_FFI_TEST_BACKEND_OPENGL) && !defined(MLN_FFI_TEST_OPENGL_WEBGL)
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_borrowed_ring(
      map, &fixture, 1, MLN_TEST_PRESET_DRIVER
    ),
    mln_test_graphics_last_error()
  );
  call.session = fixture.session;
  static const mln_test_validation_case session_rows[] = {
    {"more textures than the session's ring has slots",
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
  RUN_TEST(opengl_attach_rejects_malformed_calls);
  RUN_TEST(only_the_builds_providers_attach);
  RUN_TEST(opengl_set_target_rejects_malformed_replacements);
}
