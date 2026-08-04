// The WebGL contexts a browser host renders through.
//
// Every other platform hands a session a context it made with its own platform
// API -- WGL, EGL, Metal, Vulkan. A browser host cannot: the handle in
// `mln_webgl_context_descriptor` is not a pointer but an index into the
// Emscripten module's own context table, so a context the page created with
// `canvas.getContext("webgl2")` is not one this module can look up. The context
// has to be made *inside* this module, which is what the entry points below
// are.
//
// **A WebGL context belongs to the thread that created it.** That is the whole
// design constraint. MapLibre issues its GL calls from the thread that owns the
// render session, which for a page host is the dispatcher's thread, so the
// context is created there and nowhere else. `mln_browser_webgl_context_create`
// places the work on that thread through the dispatcher and reports its answer
// through the completion ring the host already drains;
// `mln_browser_webgl_context_create_here` is the same work for a host that
// already owns the thread it renders on, such as one running under
// `-sPROXY_TO_PTHREAD`.
//
// **The canvas is a private OffscreenCanvas the render thread makes.** Three
// other arrangements were possible and this is the one that fits what this
// build can render.
//
// - Transferring a page canvas to the render thread at `pthread_create`, which
//   is Emscripten's supported way to draw to something the page displays. It is
//   also the only moment such a transfer can happen, so the canvas would have
//   to be chosen before the module's first call, by a host that may not have
//   one yet. And it would buy nothing here: this build compiles texture
//   sessions only, which render into a framebuffer of their own and never touch
//   the default framebuffer the transferred canvas would present.
// - Creating the context on the page and rendering from the owner thread with
//   `EMSCRIPTEN_WEBGL_CONTEXT_PROXY_ALWAYS`. The wait graph allows it -- a
//   worker waiting on the page is the direction that already exists, and the
//   page services its proxy queue even while parked on a promise -- but every
//   one of a frame's thousands of GL calls would become a cross-thread round
//   trip with the worker blocked on it, and the proxied path needs
//   `-sOFFSCREEN_FRAMEBUFFER`, which this module is not linked with.
// - Rendering to the default framebuffer of a canvas and reading it back, which
//   is not reachable at all: `mln_opengl_surface_attach` is an unsupported stub
//   in this build.
//
// So the context is backed by an OffscreenCanvas constructed on the render
// thread, exactly as the C API's own browser fixtures do it, and a frame leaves
// through `mln_texture_read_premultiplied_rgba8`. A host displays it by putting
// those pixels into a canvas of its own. Nothing about that is affine to any
// thread, which is what makes a private canvas enough.
//
// The registry is `GL.offscreenCanvases` rather than `specialHTMLTargets`,
// because `findCanvasEventTarget()` -- which is what resolves the selector
// under `-sOFFSCREENCANVAS_SUPPORT` -- searches the former and never consults
// the latter.

#include <emscripten/em_js.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "browser/dispatcher.h"
#include "maplibre_native_c/base.h"

EM_JS(
  void, mln_browser_webgl_register_canvas,
  (const char* name, int width, int height), {
    const id = UTF8ToString(name);
    Module["GL"].offscreenCanvases[id] = {
      canvas : new OffscreenCanvas(width, height),
      id : id,
    };
  }
);

EM_JS(void, mln_browser_webgl_unregister_canvas, (const char* name), {
  delete Module["GL"].offscreenCanvases[UTF8ToString(name)];
});

// A context and the canvas registration that has to outlive it. Emscripten
// keeps the context; this keeps the registry key, which is the only thing that
// says which entry to remove when the context goes away. Without it the
// OffscreenCanvas and its drawing buffer would stay reachable from the module
// for as long as the page lives, and a host that reattaches a session per
// resize would accumulate one per attach.
typedef struct mln_browser_webgl_canvas {
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
  char id[32];
  struct mln_browser_webgl_canvas* next;
} mln_browser_webgl_canvas;

static pthread_mutex_t canvas_mutex = PTHREAD_MUTEX_INITIALIZER;
static mln_browser_webgl_canvas* canvases;

// Distinct per registry entry rather than per thread, because the registry is
// this module's and a host may render on more than one thread. Atomic for the
// same reason.
static atomic_uint canvas_serial;

static void mln_browser_webgl_link(mln_browser_webgl_canvas* entry) {
  pthread_mutex_lock(&canvas_mutex);
  entry->next = canvases;
  canvases = entry;
  pthread_mutex_unlock(&canvas_mutex);
}

// Detaches the record for `context`, or returns null when this module did not
// create it. Returning null is how a destroy of a foreign or already-destroyed
// handle is refused before it deletes a registry entry that is not its own.
static mln_browser_webgl_canvas* mln_browser_webgl_unlink(
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context
) {
  pthread_mutex_lock(&canvas_mutex);
  mln_browser_webgl_canvas** link = &canvases;
  while (*link != NULL && (*link)->context != context) {
    link = &(*link)->next;
  }
  mln_browser_webgl_canvas* entry = *link;
  if (entry != NULL) {
    *link = entry->next;
  }
  pthread_mutex_unlock(&canvas_mutex);
  return entry;
}

/**
 * Creates a WebGL2 context on the calling thread, or returns zero.
 *
 * `width` and `height` size the OffscreenCanvas that backs it. A texture
 * session draws into a framebuffer of its own rather than into this canvas, so
 * the size bounds nothing the map renders; it only has to be positive, because
 * a zero-sized canvas has no drawing buffer to create a context against.
 *
 * **The context is affine to the calling thread**, which must therefore be the
 * thread that owns the render session it is given to. Zero is what a failure
 * reports, and it is also the value the C API refuses in
 * `mln_webgl_context_descriptor`, so a host that passes the result on
 * unchecked is rejected there rather than rendering into nothing.
 *
 * The context is left current on this thread. Sessions make it current for
 * themselves around every frame, so this matters only to a host that wants to
 * issue its own GL calls right after creating one.
 */
MLN_API int32_t mln_browser_webgl_context_create_here(
  uint32_t width, uint32_t height
) MLN_NOEXCEPT {
  if (width == 0 || height == 0 || width > INT32_MAX || height > INT32_MAX) {
    return 0;
  }
  mln_browser_webgl_canvas* entry = calloc(1, sizeof(*entry));
  if (entry == NULL) {
    return 0;
  }

  const unsigned int serial = atomic_fetch_add(&canvas_serial, 1U) + 1U;
  (void)snprintf(entry->id, sizeof(entry->id), "mln-webgl-%u", serial);
  mln_browser_webgl_register_canvas(entry->id, (int)width, (int)height);

  EmscriptenWebGLContextAttributes attributes;
  emscripten_webgl_init_context_attributes(&attributes);
  // WebGL2 is the GLES 3.0 MapLibre's OpenGL backend targets, and the module is
  // linked with MIN_WEBGL_VERSION=2, so anything else would fail later and less
  // clearly.
  attributes.majorVersion = 2;
  attributes.minorVersion = 0;
  attributes.depth = EM_TRUE;
  attributes.stencil = EM_TRUE;
  attributes.antialias = EM_FALSE;
  // Nothing presents this canvas, so there is no drawing buffer worth
  // preserving between frames and no swap to take control of.
  attributes.preserveDrawingBuffer = EM_FALSE;
  attributes.explicitSwapControl = EM_FALSE;
  // The context stays on this thread. Proxying it to the page would turn every
  // GL call MapLibre makes into a cross-thread round trip, and needs a build
  // linked with -sOFFSCREEN_FRAMEBUFFER, which this module is not.
  attributes.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;

  char target[sizeof(entry->id) + 1];
  (void)snprintf(target, sizeof(target), "#%s", entry->id);
  entry->context = emscripten_webgl_create_context(target, &attributes);
  if (entry->context == 0) {
    mln_browser_webgl_unregister_canvas(entry->id);
    free(entry);
    return 0;
  }
  if (
    emscripten_webgl_make_context_current(entry->context) !=
    EMSCRIPTEN_RESULT_SUCCESS
  ) {
    (void)emscripten_webgl_destroy_context(entry->context);
    mln_browser_webgl_unregister_canvas(entry->id);
    free(entry);
    return 0;
  }
  mln_browser_webgl_link(entry);
  return (int32_t)entry->context;
}

/**
 * Destroys a context this module created, on the calling thread.
 *
 * **Call this on the thread that created the context**, and only once every
 * render target using it has been detached or destroyed: the C API borrows the
 * handle for a target's lifetime, and a target left holding a destroyed context
 * renders into nothing.
 *
 * A handle this module did not create, or one already destroyed, is ignored.
 */
MLN_API void mln_browser_webgl_context_destroy_here(
  int32_t context
) MLN_NOEXCEPT {
  mln_browser_webgl_canvas* entry =
    mln_browser_webgl_unlink((EMSCRIPTEN_WEBGL_CONTEXT_HANDLE)context);
  if (entry == NULL) {
    return;
  }
  (void)emscripten_webgl_destroy_context(entry->context);
  mln_browser_webgl_unregister_canvas(entry->id);
  free(entry);
}

typedef struct mln_browser_webgl_create_request {
  uint32_t width;
  uint32_t height;
  int32_t* out_context;
} mln_browser_webgl_create_request;

static void mln_browser_webgl_run_create(void* argument) {
  mln_browser_webgl_create_request* request = argument;
  // Written before this returns, and the dispatcher posts the completion only
  // afterwards, so a host that reads it once its token comes back reads a value
  // this thread has finished writing.
  *request->out_context =
    mln_browser_webgl_context_create_here(request->width, request->height);
  free(request);
}

/**
 * Creates a WebGL2 context on a dispatcher's thread.
 *
 * This is how a page host obtains one: the page may not block, and the context
 * has to belong to the thread that renders, so the work is placed on that
 * thread and the answer comes back through the ordinary completion ring.
 * `token` follows the same rules mln_browser_dispatcher_submit() sets, and the
 * completion for it reports true.
 *
 * `out_context` is host memory that must stay valid and untouched until that
 * completion arrives, because the owner thread is what writes it. It receives
 * the context handle, or zero when creation failed.
 *
 * Returns false when the submission was refused, in which case nothing runs, no
 * completion follows `token`, and `out_context` is unwritten.
 */
MLN_API bool mln_browser_webgl_context_create(
  mln_browser_dispatcher* dispatcher, uint32_t width, uint32_t height,
  int32_t* out_context, uint32_t token
) MLN_NOEXCEPT {
  if (dispatcher == NULL || out_context == NULL) {
    return false;
  }
  mln_browser_webgl_create_request* request = calloc(1, sizeof(*request));
  if (request == NULL) {
    return false;
  }
  request->width = width;
  request->height = height;
  request->out_context = out_context;
  if (!mln_browser_dispatcher_submit_task(
        dispatcher, mln_browser_webgl_run_create, request, token
      )) {
    free(request);
    return false;
  }
  return true;
}

static void mln_browser_webgl_run_destroy(void* argument) {
  // The handle travels as the argument itself rather than through an
  // allocation. It is an index into the module's context table, which fits a
  // pointer on this target, and a destroy that had to allocate would have a
  // failure path with nothing useful to do about it.
  mln_browser_webgl_context_destroy_here((int32_t)(intptr_t)argument);
}

/**
 * Destroys a context on the dispatcher's thread that created it.
 *
 * The ordering rule is mln_browser_webgl_context_destroy_here()'s: every render
 * target that borrowed the handle is detached or destroyed first. Returns false
 * when the submission was refused, in which case nothing is destroyed and no
 * completion follows `token`.
 */
MLN_API bool mln_browser_webgl_context_destroy(
  mln_browser_dispatcher* dispatcher, int32_t context, uint32_t token
) MLN_NOEXCEPT {
  return mln_browser_dispatcher_submit_task(
    dispatcher, mln_browser_webgl_run_destroy, (void*)(intptr_t)context, token
  );
}
