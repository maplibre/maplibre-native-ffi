// Raw C ABI coverage: the render target families a browser build carries, drawn
// for real.
//
// The shared fixture attaches a session-owned texture, so that family is
// covered wherever the suite runs. The other two are not: a surface session
// renders into the default framebuffer of the canvas its context is bound to,
// and a caller-owned texture session renders into a texture the caller made in
// that same context, and neither is reachable from a fixture that hands out one
// descriptor shape.
//
// The context here is on a private OffscreenCanvas this file constructs, which
// is what src/c_api/tests/test_support.c and src/browser/webgl_context.c both
// do and for the same reason: a WebGL2 context cannot exist without a canvas,
// and nothing about these families needs one the page displays. A surface
// session presents by having the browser composite its canvas, and a canvas
// nobody sees is composited by nobody -- so what these tests assert is the
// half that is this project's: that the frame reached the default framebuffer,
// or the caller's texture, read back through the context they own.
//
// That read is also what a browser host does with either family, because
// mln_texture_read_premultiplied_rgba8() covers session-owned textures alone.
// See src/browser/webgl_host.c, which is the same code placed on a page host's
// owner thread.

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

#if defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_WEBGL)

#include <GLES3/gl3.h>
#include <emscripten/em_js.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>
#include <stdint.h>
#include <stdio.h>

// The canvas registry is GL.offscreenCanvases rather than specialHTMLTargets,
// because findCanvasEventTarget() -- what resolves the selector under
// -sOFFSCREENCANVAS_SUPPORT -- searches the former and never consults the
// latter.
EM_JS(
  void, register_offscreen_canvas, (const char* name, int width, int height), {
    const id = UTF8ToString(name);
    Module["GL"].offscreenCanvases[id] = {
      canvas : new OffscreenCanvas(width, height),
      id : id,
    };
  }
);

EM_JS(void, unregister_offscreen_canvas, (const char* name), {
  delete Module["GL"].offscreenCanvases[UTF8ToString(name)];
});

// Sizes the canvas behind a registration, which is what a surface session's
// drawing buffer is. Written directly rather than through
// emscripten_set_canvas_element_size(), which resolves this registry but then
// assigns to the entry rather than to the OffscreenCanvas inside it.
EM_JS(void, size_offscreen_canvas, (const char* name, int width, int height), {
  const entry = Module["GL"].offscreenCanvases[UTF8ToString(name)];
  entry.canvas.width = width;
  entry.canvas.height = height;
});

#define CANVAS_ID "mln-render-target-abi"
#define CANVAS_SELECTOR "#" CANVAS_ID

// No sources, so nothing is fetched and a render is the background alone. Red
// is chosen because it is neither the clear color nor MapLibre's default
// background, so a pixel that reads back red was painted by this style.
static const char background_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"background\",\"type\":"
  "\"background\",\"paint\":{\"background-color\":\"#ff0000\"}}]}";

// Creates the context these tests render through, on a canvas of their own. The
// context is left current on this thread, which is where the sessions below are
// attached and where every GL call in this file is issued.
static EMSCRIPTEN_WEBGL_CONTEXT_HANDLE create_context(
  uint32_t width, uint32_t height
) {
  register_offscreen_canvas(CANVAS_ID, (int)width, (int)height);

  EmscriptenWebGLContextAttributes attributes;
  emscripten_webgl_init_context_attributes(&attributes);
  attributes.majorVersion = 2;
  attributes.minorVersion = 0;
  attributes.depth = EM_TRUE;
  attributes.stencil = EM_TRUE;
  attributes.antialias = EM_FALSE;
  // A surface session's frame is read out of this buffer after the render call
  // returned, so it has to still be there.
  attributes.preserveDrawingBuffer = EM_TRUE;
  attributes.explicitSwapControl = EM_FALSE;
  attributes.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;

  const EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context =
    emscripten_webgl_create_context(CANVAS_SELECTOR, &attributes);
  TEST_ASSERT_TRUE_MESSAGE(
    context > 0,
    "No WebGL2 context on a private OffscreenCanvas. The build needs "
    "-sOFFSCREENCANVAS_SUPPORT for the selector to resolve against "
    "GL.offscreenCanvases."
  );
  TEST_ASSERT_EQUAL_INT(
    EMSCRIPTEN_RESULT_SUCCESS, emscripten_webgl_make_context_current(context)
  );
  return context;
}

static void destroy_context(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context) {
  emscripten_webgl_destroy_context(context);
  unregister_offscreen_canvas(CANVAS_ID);
}

static void fill_webgl_context(
  mln_opengl_context_descriptor* out, EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context
) {
  out->platform = MLN_OPENGL_CONTEXT_PLATFORM_WEBGL;
  out->data.webgl = (mln_webgl_context_descriptor){
    .size = sizeof(mln_webgl_context_descriptor),
    .context = (int32_t)context,
  };
}

static mln_opengl_surface_descriptor surface_descriptor(
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context, uint32_t width, uint32_t height
) {
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  fill_webgl_context(&descriptor.context, context);
  // The settlement for WebGL: the context already names the canvas, so there is
  // no surface object to pass and a handle here is rejected.
  descriptor.surface = NULL;
  return descriptor;
}

// Creates an RGBA8 texture in the current context for a session to draw into,
// which is what a caller-owned target means: the texture is the caller's, and
// the session only attaches it to a framebuffer of its own.
static uint32_t create_caller_texture(uint32_t width, uint32_t height) {
  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0, GL_RGBA,
    GL_UNSIGNED_BYTE, NULL
  );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

// Reads the center pixel of whatever a session drew into and reports whether it
// is the style's background.
//
// `texture` names a caller-owned texture, or is zero for the canvas's default
// framebuffer, which is what a surface session renders into. A caller-owned
// target exposes no frame to acquire -- handing the texture over was the whole
// handover -- so this is what a host does with the result either way, and it is
// what says the session drew into the right place.
static bool center_is_red(uint32_t texture, uint32_t width, uint32_t height) {
  GLuint framebuffer = 0;
  bool complete = true;
  if (texture != 0) {
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(
      GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0
    );
    complete =
      glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  uint8_t pixel[4] = {0};
  if (complete) {
    glReadPixels(
      (GLint)(width / 2), (GLint)(height / 2), 1, 1, GL_RGBA, GL_UNSIGNED_BYTE,
      pixel
    );
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (framebuffer != 0) {
    glDeleteFramebuffers(1, &framebuffer);
  }
  return complete && pixel[0] > 200 && pixel[1] < 60 && pixel[2] < 60;
}

// Renders until whatever the session draws into reads back as the style's
// background, and reports whether it got there.
//
// The context is made current again before each read: a session restores
// whatever was current when its render ended.
//
// The first render has nothing to draw yet -- the style is still parsing on a
// MapLibre worker -- so this pumps the runtime between attempts, which is what
// gives that worker a chance to run.
static bool render_until_red(
  mln_runtime runtime, mln_render_session session,
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context, uint32_t texture, uint32_t width,
  uint32_t height
) {
  for (unsigned int attempt = 0; attempt < 600; attempt += 1) {
    bool rendered = false;
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_render_session_render_update(session, &rendered)
    );
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0));
    if (rendered) {
      TEST_ASSERT_EQUAL_INT(
        EMSCRIPTEN_RESULT_SUCCESS,
        emscripten_webgl_make_context_current(context)
      );
      if (center_is_red(texture, width, height)) {
        return true;
      }
    }
    mln_test_sleep_millisecond();
  }
  return false;
}

// This verifies an OpenGL surface session attaches to a browser WebGL context
// and renders into the default framebuffer of the canvas that context is bound
// to.
static void opengl_surface_session_renders_into_its_canvas(void) {
  const EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = create_context(64, 64);

  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.width = 64;
  options.height = 64;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, background_style_json)
  );

  const mln_opengl_surface_descriptor descriptor =
    surface_descriptor(context, 64, 64);
  mln_render_session session = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_opengl_surface_attach(map, &descriptor, &session)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, session);

  TEST_ASSERT_TRUE_MESSAGE(
    render_until_red(runtime, session, context, 0, 64, 64),
    "The surface session never painted the style's background into the "
    "canvas's default framebuffer."
  );

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_render_session_destroy(session));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  destroy_context(context);
}

// This verifies a WebGL surface session takes a new extent through set-target,
// which is the only thing that call can change here, and that a surface handle
// is refused rather than ignored.
static void opengl_surface_set_target_takes_a_new_extent(void) {
  const EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = create_context(64, 64);

  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.width = 64;
  options.height = 64;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, background_style_json)
  );

  mln_opengl_surface_descriptor descriptor =
    surface_descriptor(context, 64, 64);
  mln_render_session session = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_opengl_surface_attach(map, &descriptor, &session)
  );
  TEST_ASSERT_TRUE(render_until_red(runtime, session, context, 0, 64, 64));

  // The check that rejects a surface handle runs before the extent is taken, so
  // the session is left rendering into what it had.
  mln_opengl_surface_descriptor with_surface = descriptor;
  with_surface.surface = (void*)(uintptr_t)1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_opengl_surface_set_target(session, &with_surface)
  );

  // The drawing buffer is the canvas's, and only the thread that owns the
  // canvas can size it -- which is this one.
  size_offscreen_canvas(CANVAS_ID, 32, 32);
  descriptor.extent.width = 32;
  descriptor.extent.height = 32;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_opengl_surface_set_target(session, &descriptor)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    render_until_red(runtime, session, context, 0, 32, 32),
    "The surface session stopped painting its canvas after the extent changed."
  );

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_render_session_destroy(session));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  destroy_context(context);
}

// This verifies a caller-owned OpenGL texture session renders into the texture
// the host made in the session's context, and goes on doing so after the target
// is replaced with a second one.
static void opengl_borrowed_texture_session_renders_into_the_callers_texture(
  void
) {
  const EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context = create_context(64, 64);
  const uint32_t first = create_caller_texture(64, 64);
  const uint32_t second = create_caller_texture(32, 32);

  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.width = 64;
  options.height = 64;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, background_style_json)
  );

  mln_opengl_borrowed_texture_descriptor descriptor =
    mln_opengl_borrowed_texture_descriptor_default();
  descriptor.extent.width = 64;
  descriptor.extent.height = 64;
  descriptor.physical_width = 64;
  descriptor.physical_height = 64;
  fill_webgl_context(&descriptor.context, context);
  descriptor.texture = first;
  descriptor.target = GL_TEXTURE_2D;

  mln_render_session session = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_opengl_borrowed_texture_attach(map, &descriptor, &session)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    render_until_red(runtime, session, context, first, 64, 64),
    "The caller-owned texture session never painted the style's background "
    "into the texture it was given."
  );

  descriptor.texture = second;
  descriptor.extent.width = 32;
  descriptor.extent.height = 32;
  descriptor.physical_width = 32;
  descriptor.physical_height = 32;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_opengl_borrowed_texture_set_target(session, &descriptor)
  );
  TEST_ASSERT_TRUE_MESSAGE(
    render_until_red(runtime, session, context, second, 32, 32),
    "The caller-owned texture session did not follow its target to the second "
    "texture."
  );

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_render_session_destroy(session));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_INT(
    EMSCRIPTEN_RESULT_SUCCESS, emscripten_webgl_make_context_current(context)
  );
  GLuint textures[] = {first, second};
  glDeleteTextures(2, textures);
  destroy_context(context);
}

#endif

void run_browser_render_target_abi_tests(void) {
  UnitySetTestFile(__FILE__);
#if defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_WEBGL)
  RUN_TEST(opengl_surface_session_renders_into_its_canvas);
  RUN_TEST(opengl_surface_set_target_takes_a_new_extent);
  RUN_TEST(opengl_borrowed_texture_session_renders_into_the_callers_texture);
#endif
}
