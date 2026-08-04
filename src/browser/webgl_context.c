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
// **WebGL has no share groups**, and that is what decides how a frame leaves.
// A texture belongs to the one context it was created in and to no other, so a
// host that wants to draw with what MapLibre rendered issues its own GL calls
// in *this* context, on *this* thread. That is what the C API's texture
// families are for: the host either lends a texture it created here
// (`mln_opengl_borrowed_texture_attach`) or takes the one the session created
// (`mln_opengl_owned_texture_acquire_frame`). The texture is the interop
// object, and nothing is copied.
//
// **A context is created against one of two kinds of canvas**, and which one
// decides how a frame reaches the page.
//
// - A canvas the host transferred to the owner thread, named by its element id.
//   The page still displays that `<canvas>` element, and the browser pushes
//   whatever the owner thread draws into it at the end of each task, so a
//   surface session's frame -- or a texture blitted onto the default
//   framebuffer by `webgl_host.c` -- appears on the page with nothing copied
//   through the module or through JavaScript. Transferring is
//   `mln_browser_dispatcher_create_with_canvases`'s job, because Emscripten
//   only transfers at `pthread_create`.
// - A private OffscreenCanvas this file creates, for a context with no id
//   named. It is never displayed, which is exactly what a host that reads
//   frames back wants; a WebGL2 context cannot exist without some canvas, and
//   this is the one that costs the page nothing.
//
// Rendering from the owner thread through a context the *page* created was
// considered and is not used. `EMSCRIPTEN_WEBGL_CONTEXT_PROXY_ALWAYS` allows it
// -- a worker waiting on the page is the direction that already exists, and the
// page services its proxy queue even while parked on a promise -- but every one
// of a frame's thousands of GL calls would become a cross-thread round trip
// with the worker blocked on it, and the proxied path needs
// `-sOFFSCREEN_FRAMEBUFFER`, which this module is not linked with.
//
// The registry is `GL.offscreenCanvases` rather than `specialHTMLTargets`,
// because `findCanvasEventTarget()` -- which is what resolves the selector
// under `-sOFFSCREENCANVAS_SUPPORT` -- searches the former and never consults
// the latter. It is also where Emscripten puts a transferred canvas, so both
// kinds are found the same way, and the difference between them is only who
// created the entry and therefore who removes it.

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
#include <string.h>

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

// Reports whether the registry holds an entry for `name`, which for a
// host-named canvas is what says the transfer at `pthread_create` reached this
// thread. Checked before a context is created against it, so a mistyped id
// fails saying the canvas is missing rather than failing as a context that
// could not be made.
EM_JS(int, mln_browser_webgl_has_canvas, (const char* name), {
  return Module["GL"].offscreenCanvases[UTF8ToString(name)] ? 1 : 0;
});

// Sizes a registered canvas's drawing buffer, reporting whether the entry was
// still there. Written here rather than through
// emscripten_set_canvas_element_size(), which resolves the same registry but
// then assigns to the registry entry rather than to the OffscreenCanvas inside
// it.
//
// The two kinds of entry hold their canvas under different names: this file
// constructs `{canvas}`, and Emscripten's `pthread_create` transfer produces
// `{offscreenCanvas, canvasSharedPtr}`. Both are resolved here, the same way
// `_emscripten_webgl_do_create_context` resolves them, so a host sizes a canvas
// without knowing which kind it has.
EM_JS(
  int, mln_browser_webgl_size_canvas, (const char* name, int width, int height),
  {
    const entry = Module["GL"].offscreenCanvases[UTF8ToString(name)];
    if (!entry) {
      return 0;
    }
    const canvas = entry.offscreenCanvas || entry.canvas;
    if (!canvas) {
      return 0;
    }
    canvas.width = width;
    canvas.height = height;
    // A transferred canvas carries its size in shared memory so that the page
    // and this thread agree on it. Left stale,
    // emscripten_get_canvas_element_size would keep reporting whatever the
    // element measured before the transfer.
    if (entry.canvasSharedPtr) {
      HEAP32[entry.canvasSharedPtr >> 2] = width;
      HEAP32[(entry.canvasSharedPtr + 4) >> 2] = height;
    }
    return 1;
  }
);

// A context and the canvas registration that has to outlive it. Emscripten
// keeps the context; this keeps the registry key, which is the only thing that
// says which entry to remove when the context goes away. Without it the
// OffscreenCanvas and its drawing buffer would stay reachable from the module
// for as long as the page lives, and a host that reattaches a session per
// resize would accumulate one per attach.
//
// Long enough for an element id a host would actually write. A longer one is
// refused at creation rather than truncated, because a truncated key names a
// different canvas or none.
#define MLN_BROWSER_WEBGL_CANVAS_ID_BYTES 64

typedef struct mln_browser_webgl_canvas {
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
  char id[MLN_BROWSER_WEBGL_CANVAS_ID_BYTES];
  // Set for the private OffscreenCanvas this file constructs, and clear for a
  // canvas the host transferred at `pthread_create`. It decides who removes the
  // registry entry: a transferred canvas belongs to the host and to the page
  // element it is still displayed through, and removing its entry would leave
  // the host unable to make a second context for the canvas it still owns.
  bool owned;
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

// Copies the registry key of the canvas behind `context`, or reports that this
// module did not create it. Copied under the lock rather than handing back the
// record, because the record is only safe to read while the lock is held and
// the key is all a caller needs.
static bool mln_browser_webgl_copy_id(
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context, char* out, size_t capacity
) {
  bool found = false;
  pthread_mutex_lock(&canvas_mutex);
  for (const mln_browser_webgl_canvas* entry = canvases; entry != NULL;
       entry = entry->next) {
    if (entry->context == context) {
      (void)snprintf(out, capacity, "%s", entry->id);
      found = true;
      break;
    }
  }
  pthread_mutex_unlock(&canvas_mutex);
  return found;
}

/**
 * Creates a WebGL2 context on the calling thread, or returns zero.
 *
 * `canvas_id` names a `<canvas>` element the host transferred to this thread
 * with mln_browser_dispatcher_create_with_canvases(), and the page keeps
 * displaying that element: everything reaching this context's default
 * framebuffer is composited onto it, so a frame reaches the page with nothing
 * copied. Pass null or an empty string for a private OffscreenCanvas instead,
 * which is never displayed and is what a host that reads frames back wants.
 *
 * `width` and `height` size the canvas's drawing buffer. A texture session
 * draws into a framebuffer of its own rather than into the canvas, so for one
 * the size bounds nothing the map renders and only has to be positive, because
 * a zero-sized canvas has no drawing buffer to create a context against. A
 * surface session renders into the canvas's default framebuffer, so for one the
 * size is that session's physical extent -- and so is the size of anything
 * mln_browser_webgl_present_texture_here() blits onto it.
 *
 * **The context is affine to the calling thread**, which must therefore be the
 * thread that owns the render session it is given to. Zero is what a failure
 * reports, and it is also the value the C API refuses in
 * `mln_webgl_context_descriptor`, so a host that passes the result on
 * unchecked is rejected there rather than rendering into nothing. A
 * `canvas_id` that no transfer put on this thread is one such failure.
 *
 * The context is left current on this thread. Sessions make it current for
 * themselves around every frame, so this matters only to a host that wants to
 * issue its own GL calls right after creating one -- which is what a host does
 * with a rendered texture, and it has to do it here.
 */
MLN_API int32_t mln_browser_webgl_context_create_here(
  const char* canvas_id, uint32_t width, uint32_t height
) MLN_NOEXCEPT {
  if (width == 0 || height == 0 || width > INT32_MAX || height > INT32_MAX) {
    return 0;
  }
  const bool host_canvas = canvas_id != NULL && canvas_id[0] != '\0';
  if (host_canvas && strlen(canvas_id) >= MLN_BROWSER_WEBGL_CANVAS_ID_BYTES) {
    return 0;
  }
  mln_browser_webgl_canvas* entry = calloc(1, sizeof(*entry));
  if (entry == NULL) {
    return 0;
  }

  if (host_canvas) {
    (void)snprintf(entry->id, sizeof(entry->id), "%s", canvas_id);
    if (!mln_browser_webgl_has_canvas(entry->id)) {
      free(entry);
      return 0;
    }
    // Sized here as well as for a private canvas, so that `width` and `height`
    // mean the same thing either way. A transferred canvas arrives at whatever
    // the element measured, which is a CSS layout size rather than the device
    // pixels a session renders in.
    if (!mln_browser_webgl_size_canvas(entry->id, (int)width, (int)height)) {
      free(entry);
      return 0;
    }
  } else {
    entry->owned = true;
    const unsigned int serial = atomic_fetch_add(&canvas_serial, 1U) + 1U;
    (void)snprintf(entry->id, sizeof(entry->id), "mln-webgl-%u", serial);
    mln_browser_webgl_register_canvas(entry->id, (int)width, (int)height);
  }

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
  // Preserved, because a host reads a surface session's frame out of this
  // canvas's default framebuffer in a later task than the one that drew it --
  // see mln_browser_webgl_read_pixels(). A canvas the page displays is
  // composited at the end of each task and would otherwise be cleared as part
  // of that, which would also lose a frame between two of a host's own calls.
  // A texture session never touches this buffer, so it pays nothing.
  attributes.preserveDrawingBuffer = EM_TRUE;
  // Left implicit, which is what presenting depends on: with implicit swap the
  // browser pushes what this context drew when the task that drew it ends, and
  // the owner thread's task ends after every drain. Taking explicit control
  // would mean calling emscripten_webgl_commit_frame(), which is a documented
  // no-op in this emsdk and would present nothing at all.
  attributes.explicitSwapControl = EM_FALSE;
  // The context stays on this thread. Proxying it to the page would turn every
  // GL call MapLibre makes into a cross-thread round trip, and needs a build
  // linked with -sOFFSCREEN_FRAMEBUFFER, which this module is not.
  attributes.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;

  char target[sizeof(entry->id) + 1];
  (void)snprintf(target, sizeof(target), "#%s", entry->id);
  entry->context = emscripten_webgl_create_context(target, &attributes);
  if (entry->context == 0) {
    if (entry->owned) {
      mln_browser_webgl_unregister_canvas(entry->id);
    }
    free(entry);
    return 0;
  }
  if (
    emscripten_webgl_make_context_current(entry->context) !=
    EMSCRIPTEN_RESULT_SUCCESS
  ) {
    (void)emscripten_webgl_destroy_context(entry->context);
    if (entry->owned) {
      mln_browser_webgl_unregister_canvas(entry->id);
    }
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
 * A private OffscreenCanvas goes with the context that was created against it.
 * A canvas the host transferred does not: the page still displays the element,
 * the transfer cannot be repeated, and a host may create a second context
 * against it -- so its registry entry stays for the thread's lifetime.
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
  if (entry->owned) {
    mln_browser_webgl_unregister_canvas(entry->id);
  }
  free(entry);
}

/**
 * Sizes the drawing buffer behind a context this thread owns.
 *
 * A surface session renders into the default framebuffer of the canvas its
 * context was created on, and that framebuffer is only as large as the canvas.
 * So a host that changes such a session's extent changes this too, in two
 * steps: this call, and then mln_render_session_resize() or
 * mln_opengl_surface_set_target() with the matching extent. Neither implies the
 * other -- the session's extent is what MapLibre lays a frame out for, and this
 * is what the frame has room to land in.
 *
 * **The canvas belongs to the thread that created it**, which is why this is
 * affine the way everything else here is. The context's contents survive:
 * resizing a canvas reallocates its drawing buffer and nothing else, so every
 * texture, buffer, and program the session built stays as it was.
 *
 * A texture session has no reason to call this, because it draws into a
 * framebuffer of its own and never touches the canvas.
 *
 * Returns false for a non-positive size and for a handle this module did not
 * create, or has already destroyed.
 */
MLN_API bool mln_browser_webgl_canvas_resize_here(
  int32_t context, uint32_t width, uint32_t height
) MLN_NOEXCEPT {
  if (width == 0 || height == 0 || width > INT32_MAX || height > INT32_MAX) {
    return false;
  }
  char id[MLN_BROWSER_WEBGL_CANVAS_ID_BYTES];
  if (!mln_browser_webgl_copy_id(
        (EMSCRIPTEN_WEBGL_CONTEXT_HANDLE)context, id, sizeof(id)
      )) {
    return false;
  }
  return mln_browser_webgl_size_canvas(id, (int)width, (int)height) != 0;
}

typedef struct mln_browser_webgl_resize_request {
  int32_t context;
  uint32_t width;
  uint32_t height;
  int32_t* out_ok;
} mln_browser_webgl_resize_request;

static void mln_browser_webgl_run_resize(void* argument) {
  mln_browser_webgl_resize_request* request = argument;
  *request->out_ok = mln_browser_webgl_canvas_resize_here(
                       request->context, request->width, request->height
                     )
                       ? 1
                       : 0;
  free(request);
}

/**
 * Sizes a canvas's drawing buffer on the dispatcher's thread that owns it.
 *
 * mln_browser_webgl_canvas_resize_here()'s work, placed on the owner thread.
 * `out_ok` is host memory that must stay valid and untouched until the
 * completion for `token` arrives; it receives one when the canvas was sized.
 *
 * Returns false when the submission was refused, in which case nothing runs and
 * no completion follows `token`.
 */
MLN_API bool mln_browser_webgl_canvas_resize(
  mln_browser_dispatcher* dispatcher, int32_t context, uint32_t width,
  uint32_t height, int32_t* out_ok, uint32_t token
) MLN_NOEXCEPT {
  if (dispatcher == NULL || out_ok == NULL) {
    return false;
  }
  mln_browser_webgl_resize_request* request = calloc(1, sizeof(*request));
  if (request == NULL) {
    return false;
  }
  request->context = context;
  request->width = width;
  request->height = height;
  request->out_ok = out_ok;
  if (!mln_browser_dispatcher_submit_task(
        dispatcher, mln_browser_webgl_run_resize, request, token
      )) {
    free(request);
    return false;
  }
  return true;
}

typedef struct mln_browser_webgl_create_request {
  // Copied rather than borrowed, because a canvas id is the one argument here
  // that a host is likely to build from a string it then drops. Everything else
  // crossing to the owner thread is a scalar or host memory the host was
  // already told to keep.
  char canvas_id[MLN_BROWSER_WEBGL_CANVAS_ID_BYTES];
  uint32_t width;
  uint32_t height;
  int32_t* out_context;
} mln_browser_webgl_create_request;

static void mln_browser_webgl_run_create(void* argument) {
  mln_browser_webgl_create_request* request = argument;
  // Written before this returns, and the dispatcher posts the completion only
  // afterwards, so a host that reads it once its token comes back reads a value
  // this thread has finished writing.
  *request->out_context = mln_browser_webgl_context_create_here(
    request->canvas_id, request->width, request->height
  );
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
 * `canvas_id` is mln_browser_webgl_context_create_here()'s, and names a canvas
 * this dispatcher was created with; it is copied here rather than borrowed, so
 * it need not outlive this call. An id longer than 63 bytes is refused.
 *
 * `out_context` is host memory that must stay valid and untouched until that
 * completion arrives, because the owner thread is what writes it. It receives
 * the context handle, or zero when creation failed.
 *
 * Returns false when the submission was refused, in which case nothing runs, no
 * completion follows `token`, and `out_context` is unwritten.
 */
MLN_API bool mln_browser_webgl_context_create(
  mln_browser_dispatcher* dispatcher, const char* canvas_id, uint32_t width,
  uint32_t height, int32_t* out_context, uint32_t token
) MLN_NOEXCEPT {
  if (dispatcher == NULL || out_context == NULL) {
    return false;
  }
  if (
    canvas_id != NULL && strlen(canvas_id) >= MLN_BROWSER_WEBGL_CANVAS_ID_BYTES
  ) {
    return false;
  }
  mln_browser_webgl_create_request* request = calloc(1, sizeof(*request));
  if (request == NULL) {
    return false;
  }
  if (canvas_id != NULL) {
    (void)snprintf(
      request->canvas_id, sizeof(request->canvas_id), "%s", canvas_id
    );
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
