
#include <GLES3/gl3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "render.h"
#include "render_backend.h"
#include "unity.h"
#include "wait.h"

// A fixture needs a GL context but no on-page canvas, and an OffscreenCanvas
// belongs to a single thread, so each fixture gets its own on the calling
// thread. The registry must be GL.offscreenCanvases, where
// findCanvasEventTarget() resolves the selector under
// -sOFFSCREENCANVAS_SUPPORT.
// The entry carries the canvas under both names its consumers unwrap:
// `offscreenCanvas` for WebGL and emdawnwebgpu, `canvas` for the transfer
// path.
EM_JS(
  void, mln_test_register_offscreen_canvas,
  (const char* name, int width, int height), {
    const id = UTF8ToString(name);
    const canvas = new OffscreenCanvas(width, height);
    Module["GL"].offscreenCanvases[id] = {
      canvas : canvas,
      offscreenCanvas : canvas,
      id : id,
    };
  }
);
EM_JS(
  void*, mln_test_register_transferred_offscreen_canvas,
  (const char* name, int width, int height), {
    const id = UTF8ToString(name);
    const selector = "#" + id;
    const canvas = new OffscreenCanvas(width, height);
    const canvasSharedPtr = _malloc(12);
    HEAP32[canvasSharedPtr >> 2] = width;
    HEAP32[canvasSharedPtr + 4 >> 2] = height;
    HEAPU32[canvasSharedPtr + 8 >> 2] = 0;
    Module["GL"].offscreenCanvases[selector] = {
      canvas : canvas,
      offscreenCanvas : canvas,
      canvasSharedPtr : canvasSharedPtr,
      id : id,
    };
    return canvasSharedPtr;
  }
);
EM_JS(
  void, mln_test_unregister_transferred_offscreen_canvas, (const char* name),
  { delete Module["GL"].offscreenCanvases["#" + UTF8ToString(name)]; }
);
EM_JS(void, mln_test_unregister_offscreen_canvas, (const char* name), {
  delete Module["GL"].offscreenCanvases[UTF8ToString(name)];
});

// The calling thread's host context, on a canvas of its own.
typedef struct webgl_state {
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
  char id[32];
} webgl_state;

// What a fixture holds: the textures of a borrowed ring, which live in the
// thread's context, or a transferred canvas.
typedef struct webgl_fixture_state {
  bool transferred;
  GLuint textures[MLN_TEXTURE_RING_MAX_DEPTH];
  size_t texture_count;
  char id[32];
  void* canvas_shared_ptr;
} webgl_fixture_state;

static atomic_uint webgl_canvas_counter;
static MLN_TEST_THREAD_LOCAL webgl_state thread_webgl_state;

static mln_open_gl_context_descriptor describe_context(
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context
) {
  return (mln_open_gl_context_descriptor){
    .platform = MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL,
    .data = {
      .web_gl = {
        .context = context,
      }
    },
  };
}

// Makes the thread's host context current, creating it first, and describes
// it.
static bool thread_context(mln_open_gl_context_descriptor* out_context) {
  webgl_state* state = &thread_webgl_state;
  if (state->context > 0) {
    if (
      emscripten_webgl_make_context_current(state->context) !=
      EMSCRIPTEN_RESULT_SUCCESS
    ) {
      return false;
    }
    *out_context = describe_context(state->context);
    return true;
  }

  EmscriptenWebGLContextAttributes attributes;
  emscripten_webgl_init_context_attributes(&attributes);
  attributes.majorVersion = 2;
  attributes.minorVersion = 0;
  attributes.depth = EM_TRUE;
  attributes.stencil = EM_TRUE;
  attributes.antialias = EM_FALSE;
  attributes.preserveDrawingBuffer = EM_FALSE;
  // The context stays on the thread that created it, and nothing renders to
  // the page, so there is no swap to proxy.
  attributes.explicitSwapControl = EM_FALSE;
  attributes.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;

  const unsigned int serial = atomic_fetch_add(&webgl_canvas_counter, 1U) + 1U;
  (void)snprintf(state->id, sizeof(state->id), "mln-test-%u", serial);
  mln_test_register_offscreen_canvas(state->id, 64, 64);

  char target[sizeof(state->id) + 1];
  (void)snprintf(target, sizeof(target), "#%s", state->id);
  state->context = emscripten_webgl_create_context(target, &attributes);
  if (state->context <= 0) {
    fprintf(
      stderr, "creating the fixture's WebGL context failed: %ld\n",
      (long)state->context
    );
    mln_test_unregister_offscreen_canvas(state->id);
    *state = (webgl_state){0};
    return false;
  }
  const EMSCRIPTEN_RESULT current_result =
    emscripten_webgl_make_context_current(state->context);
  if (current_result != EMSCRIPTEN_RESULT_SUCCESS) {
    fprintf(
      stderr, "making the fixture's WebGL context current failed: %d\n",
      current_result
    );
    emscripten_webgl_destroy_context(state->context);
    mln_test_unregister_offscreen_canvas(state->id);
    *state = (webgl_state){0};
    return false;
  }

  *out_context = describe_context(state->context);
  return true;
}

static void destroy_backend_state(void* opaque_state) {
  // WebGL contexts are expensive and browsers cap their live count. Sequential
  // fixtures on one test thread share one host-owned context; the thread-level
  // cleanup releases it after the suite or foreign thread exits. A transferred
  // canvas has no host-owned context and is released with its fixture.
  webgl_fixture_state* state = opaque_state;
  if (state == NULL) {
    return;
  }
  if (state->transferred) {
    mln_test_unregister_transferred_offscreen_canvas(state->id);
    free(state->canvas_shared_ptr);
  } else if (
    state->texture_count > 0 && thread_webgl_state.context > 0 &&
    emscripten_webgl_make_context_current(thread_webgl_state.context) ==
      EMSCRIPTEN_RESULT_SUCCESS
  ) {
    glDeleteTextures((GLsizei)state->texture_count, state->textures);
  }
  free(state);
}

uint32_t mln_test_backend_driver(void) {
  return MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
}

// The textures live in the thread's context, which the session renders in
// when it services on this thread.
bool mln_test_backend_attach_borrowed(
  mln_map map, const mln_render_session_attach_options* options,
  void** out_state, mln_render_session* out_session,
  const mln_completion* completion, mln_status* out_status
) {
  const size_t depth = mln_test_render_ring_depth();
  mln_open_gl_context_descriptor context = {0};
  if (!thread_context(&context)) {
    return false;
  }
  webgl_fixture_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return false;
  }
  glGenTextures((GLsizei)depth, state->textures);
  state->texture_count = depth;
  mln_open_gl_borrowed_texture entries[MLN_TEXTURE_RING_MAX_DEPTH];
  bool named = true;
  for (size_t index = 0; index < depth; index += 1) {
    named = named && state->textures[index] != 0;
    glBindTexture(GL_TEXTURE_2D, state->textures[index]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(
      GL_TEXTURE_2D, 0, GL_RGBA8, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL
    );
    entries[index] = (mln_open_gl_borrowed_texture){
      .texture = state->textures[index],
    };
  }
  glBindTexture(GL_TEXTURE_2D, 0);
  const GLenum error = glGetError();
  if (!named || error != GL_NO_ERROR) {
    fprintf(stderr, "creating a WebGL ring texture failed: 0x%x\n", error);
    destroy_backend_state(state);
    return false;
  }
  mln_open_gl_borrowed_texture_descriptor descriptor = {
    .size = sizeof(mln_open_gl_borrowed_texture_descriptor)
  };
  descriptor.logical_size.width = 64;
  descriptor.logical_size.height = 64;
  descriptor.physical_width = 64;
  descriptor.physical_height = 64;
  descriptor.context = context;
  descriptor.textures = entries;
  descriptor.texture_count = depth;
  descriptor.target = GL_TEXTURE_2D;
  *out_state = state;
  *out_status = mln_map_attach_open_gl_borrowed_texture(
    map, &descriptor, options, out_session, completion, MLN_TEST_DIAGNOSTIC
  );
  return true;
}

void mln_test_backend_destroy(void* state) { destroy_backend_state(state); }

void mln_test_release_thread_gpu_resources(void) {
  if (thread_webgl_state.context > 0) {
    emscripten_webgl_destroy_context(thread_webgl_state.context);
    mln_test_unregister_offscreen_canvas(thread_webgl_state.id);
  }
  thread_webgl_state = (webgl_state){0};
}

// Attaches a core-worker session to the canvas with `state->id`, which the
// fixture then owns along with `state`.
static bool attach_transferred_surface(
  mln_map map, webgl_fixture_state* state, mln_test_render_fixture* fixture
) {
  mln_test_render_reserve_session();
  *fixture = (mln_test_render_fixture){0};
  fixture->driver = MLN_RENDER_DRIVER_CORE_WORKER;
  fixture->backend_state = state;

  char target[sizeof(state->id) + 1];
  (void)snprintf(target, sizeof(target), "#%s", state->id);
  mln_open_gl_surface_descriptor descriptor = {
    .size = sizeof(mln_open_gl_surface_descriptor)
  };
  descriptor.logical_size.width = 64;
  descriptor.logical_size.height = 64;
  descriptor.context.platform = MLN_OPEN_GL_CONTEXT_PLATFORM_WEB_GL;
  descriptor.context.ownership = MLN_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED;
  descriptor.context.data.web_gl = (mln_web_gl_context_descriptor){
    .kind = MLN_WEB_GL_CONTEXT_TRANSFERRED_CANVAS,
    .canvas_selector = mln_buffer_view_from_cstr(target),
  };
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.driver = fixture->driver;
  options.frame_wake = (mln_wake){
    .callback = mln_test_render_count_wake, .user_data = &fixture->frame_wakes
  };
  options.driver_work_wake = (mln_wake){
    .callback = mln_test_render_count_wake, .user_data = &fixture->driver_wakes
  };
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_map_attach_open_gl_surface(
    map, &descriptor, &options, &fixture->session, &completion.descriptor,
    MLN_TEST_DIAGNOSTIC
  );
  const mln_status finish_status =
    status == MLN_STATUS_OK && fixture->session != MLN_HANDLE_NULL
      ? mln_test_render_fixture_finish_operation(fixture, &completion)
      : MLN_STATUS_INVALID_STATE;
  if (
    status != MLN_STATUS_OK || finish_status != MLN_STATUS_OK ||
    fixture->session == MLN_HANDLE_NULL
  ) {
    if (status != MLN_STATUS_OK) {
      completion.descriptor.release_user_data(completion.descriptor.user_data);
    }
    fprintf(
      stderr,
      "transferred WebGL attach failed: submit=%d finish=%d diagnostic=%s\n",
      status, finish_status, mln_test_completion_diagnostic(&completion)
    );
    mln_test_completion_destroy(&completion);
    if (fixture->session != MLN_HANDLE_NULL) {
      mln_render_abandon_result abandoned = {
        .size = sizeof(mln_render_abandon_result)
      };
      (void)mln_render_session_abandon(
        fixture->session, &abandoned, MLN_TEST_DIAGNOSTIC
      );
      (void)mln_test_render_session_release(fixture->session);
    }
    destroy_backend_state(fixture->backend_state);
    *fixture = (mln_test_render_fixture){0};
    return false;
  }
  mln_test_completion_destroy(&completion);
  mln_test_render_track_session(fixture);
  return true;
}

bool mln_test_transferred_webgl_surface_create(
  mln_map map, mln_test_render_fixture* fixture
) {
  if (map == MLN_HANDLE_NULL || fixture == NULL) {
    return false;
  }
  webgl_fixture_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return false;
  }
  state->transferred = true;
  const unsigned int serial = atomic_fetch_add(&webgl_canvas_counter, 1U) + 1U;
  (void)snprintf(state->id, sizeof(state->id), "mln-transfer-%u", serial);
  state->canvas_shared_ptr =
    mln_test_register_transferred_offscreen_canvas(state->id, 64, 64);
  return attach_transferred_surface(map, state, fixture);
}

bool mln_test_page_webgl_surface_create(
  mln_map map, const char* canvas_id, mln_test_render_fixture* fixture
) {
  if (map == MLN_HANDLE_NULL || canvas_id == NULL || fixture == NULL) {
    return false;
  }
  webgl_fixture_state* state = calloc(1, sizeof(*state));
  if (state == NULL) {
    return false;
  }
  // The test thread doesn't own the page's canvas, so the library's worker
  // creation runs on the main thread, which transfers the canvas itself.
  state->transferred = true;
  (void)snprintf(state->id, sizeof(state->id), "%s", canvas_id);
  return attach_transferred_surface(map, state, fixture);
}
