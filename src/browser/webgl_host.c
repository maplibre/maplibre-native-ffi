// The GL work a browser host does on the thread its maps render on.
//
// Every other platform expects the host to own a render thread, create a
// graphics context there, attach a session there, and issue its own graphics
// calls there -- creating the texture a caller-owned target draws into, reading
// a rendered frame back, drawing the result into whatever it presents. A
// browser host owns no thread. The one that renders is the dispatcher's, and a
// page has no way to reach it: a WebGL context is affine to the agent that
// created it, and WebGL shares no objects between contexts, so a texture the
// page made through `canvas.getContext("webgl2")` names nothing a session could
// attach.
//
// This file is that host-side GL, placed where it has to run. It is the same
// shape as src/browser/webgl_context.c: a `_here` entry point for a host that
// already owns the thread it renders on, and a dispatched one that puts the
// same work on the owner thread and answers through the completion ring a page
// host already drains.
//
// **The texture is the interop object**, exactly as it is everywhere else.
// mln_browser_webgl_texture_create() is where a caller-owned target's texture
// comes from, and the name it reports is what
// `mln_opengl_borrowed_texture_descriptor.texture` carries. A session-owned
// target hands its own texture back through
// mln_opengl_owned_texture_acquire_frame() and needs nothing from here.
//
// **mln_browser_webgl_present_texture() is how a texture frame reaches the
// page.** A surface session needs nothing: it renders into the default
// framebuffer of the canvas its context is bound to, and if that canvas is one
// the host transferred, the browser composites it onto the page element at the
// end of the task that drew it. A texture session renders into a framebuffer of
// its own, so something has to move those pixels onto the default framebuffer,
// and that something has to run in the session's context on the session's
// thread -- which is here.
//
// It is a `glBlitFramebuffer` from a scratch read framebuffer with the texture
// attached, and it is what "zero copy" means in a browser: the pixels stay in
// GPU memory, never enter the module's heap, and never cross an agent boundary.
// A textured quad would do the same job and would need a program, a vertex
// buffer, and a vertex array, each of which is state MapLibre's GL backend
// caches; the blit touches two framebuffer bindings and the scissor.
//
// **mln_browser_webgl_read_pixels() is how a frame leaves** when a host is not
// presenting at all -- a test, a screenshot, a host with no page. The C API's
// own readback, mln_texture_read_premultiplied_rgba8(), covers session-owned
// texture targets and refuses the other two families -- for a caller-owned
// target the texture was never the session's to read, and a surface target has
// no texture at all. On every other platform that is not a gap, because the
// host reads its own texture with its own graphics API. Here it would be one,
// so this reads either: a texture name, or zero for the default framebuffer of
// the context's canvas, which is what a surface session renders into.
//
// Every entry point restores the GL state it changed. MapLibre's GL backend
// remembers what it last set and skips a redundant call, so a binding that left
// state changed behind its back is state the next frame renders against without
// knowing.

#include <GLES3/gl3.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "browser/dispatcher.h"
#include "maplibre_native_c/base.h"

// Discards errors left by earlier work, so that a check afterwards reports this
// file's own. Bounded because a context reports a lost context for as long as
// it stays lost, and an unbounded drain would never end.
static void mln_browser_webgl_clear_errors(void) {
  for (int guard = 0; guard < 16; guard += 1) {
    if (glGetError() == GL_NO_ERROR) {
      return;
    }
  }
}

// Makes a context current, or reports that the handle names no context this
// thread can use. Every entry point below starts here: a context is affine to
// the thread that created it, and a render session restores whatever was
// current before its frame, which is not necessarily this.
static bool mln_browser_webgl_bind(int32_t context) {
  if (context <= 0) {
    return false;
  }
  return emscripten_webgl_make_context_current(
           (EMSCRIPTEN_WEBGL_CONTEXT_HANDLE)context
         ) == EMSCRIPTEN_RESULT_SUCCESS;
}

// A rectangle measured from a target's origin, in device pixels, that both a
// texture and a readback have to fit. GLsizei is signed, and the byte length of
// an RGBA8 image of this size has to be a size_t this module can allocate, so
// the bound is what keeps either from wrapping into a smaller number than the
// pixels it describes.
static bool mln_browser_webgl_extent_fits(uint32_t width, uint32_t height) {
  const uint32_t limit = 16384;
  return width > 0 && height > 0 && width <= limit && height <= limit;
}

/**
 * Creates an RGBA8 texture in a context this thread owns, or returns zero.
 *
 * This is how a browser host obtains the texture a caller-owned target draws
 * into. `context` is a handle from mln_browser_webgl_context_create_here(), and
 * the texture belongs to it: pass the same context in the descriptor that names
 * the texture, because the session attaches the texture to a framebuffer of
 * that context and no other context can reach it.
 *
 * `width` and `height` size the texture in device pixels, which is what
 * `mln_opengl_borrowed_texture_descriptor.physical_width` and `physical_height`
 * then state.
 *
 * **The texture is the host's.** Nothing here tracks it, a render target only
 * borrows it, and mln_browser_webgl_texture_destroy_here() is what releases it
 * -- before the context it was created in is destroyed, or with that context,
 * which releases every object made in it.
 */
MLN_API uint32_t mln_browser_webgl_texture_create_here(
  int32_t context, uint32_t width, uint32_t height
) MLN_NOEXCEPT {
  if (
    !mln_browser_webgl_extent_fits(width, height) ||
    !mln_browser_webgl_bind(context)
  ) {
    return 0;
  }

  GLint previous = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
  mln_browser_webgl_clear_errors();

  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0, GL_RGBA,
    GL_UNSIGNED_BYTE, NULL
  );
  // One level, because a render target is drawn at its own size and never
  // sampled from a smaller one. A texture whose minification filter asked for
  // mipmaps would be incomplete until something built them.
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  const bool created = glGetError() == GL_NO_ERROR;

  glBindTexture(GL_TEXTURE_2D, (GLuint)previous);
  if (!created) {
    glDeleteTextures(1, &texture);
    return 0;
  }
  return texture;
}

/**
 * Destroys a texture in a context this thread owns.
 *
 * **Call this once every render target that borrows the texture has been
 * detached or destroyed**, because a session whose target names a destroyed
 * texture renders into nothing. Destroying the context releases the texture
 * with it, so a handle naming a context that is already gone is ignored rather
 * than reported.
 */
MLN_API void mln_browser_webgl_texture_destroy_here(
  int32_t context, uint32_t texture
) MLN_NOEXCEPT {
  if (texture == 0 || !mln_browser_webgl_bind(context)) {
    return;
  }
  GLuint name = texture;
  glDeleteTextures(1, &name);
}

/**
 * Reads a rendered frame out of a context this thread owns, or returns false.
 *
 * `texture` names a two-dimensional texture of `context` -- the one a
 * caller-owned target was given -- or is zero for the default framebuffer of
 * the canvas the context is bound to, which is what a surface session renders
 * into. `width` and `height` are the region to read, measured from the target's
 * origin, and `out_pixels` receives `width * height * 4` bytes of RGBA8 that
 * `out_capacity` has to cover.
 *
 * **Row zero is the bottom row**, because that is where GL's origin is and
 * nothing here flips the image. A host that wants top-down rows, which is what
 * `ImageData` and `mln_texture_read_premultiplied_rgba8` both use, reverses
 * them itself.
 *
 * This is a synchronous read of the GPU: it stalls the owner thread until the
 * frame is done. That is legal there and is what the thread exists for, but it
 * is the expensive way to use a frame, and a host that only wants to show one
 * should draw with the texture instead -- which is also GL work, and also
 * belongs on this thread.
 *
 * Returns false when the context cannot be made current, when the extent does
 * not fit the capacity, when a texture cannot be attached to a framebuffer, or
 * when the read reported a GL error. `out_pixels` is then unspecified rather
 * than unwritten, because a read that fails partway has already written.
 */
MLN_API bool mln_browser_webgl_read_pixels_here(
  int32_t context, uint32_t texture, uint32_t width, uint32_t height,
  uint8_t* out_pixels, size_t out_capacity
) MLN_NOEXCEPT {
  if (
    out_pixels == NULL || !mln_browser_webgl_extent_fits(width, height) ||
    out_capacity < (size_t)width * (size_t)height * 4U ||
    !mln_browser_webgl_bind(context)
  ) {
    return false;
  }

  // Put back before this returns. A read is not affected by the scissor
  // rectangle, so only the binding matters here, but MapLibre leaves its own
  // framebuffer bound between frames and assumes it is still there.
  GLint previous = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous);
  mln_browser_webgl_clear_errors();

  // Framebuffer zero for a surface target: it is the canvas's, and naming a
  // framebuffer of our own would read something the session never drew into.
  GLuint framebuffer = 0;
  bool read = true;
  if (texture != 0) {
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(
      GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, (GLuint)texture,
      0
    );
    read =
      glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
  } else {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
  }

  if (read) {
    // Rows are width * 4 bytes and therefore already four-byte aligned, which
    // is the default pack alignment, so nothing here has to change it and no
    // state is left changed for the next frame.
    glReadPixels(
      0, 0, (GLsizei)width, (GLsizei)height, GL_RGBA, GL_UNSIGNED_BYTE,
      out_pixels
    );
    read = glGetError() == GL_NO_ERROR;
  }

  glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous);
  if (framebuffer != 0) {
    glDeleteFramebuffers(1, &framebuffer);
  }
  return read;
}

/**
 * Blits a rendered texture onto the default framebuffer of its context.
 *
 * This is how a texture session's frame reaches the page. `texture` names a
 * two-dimensional texture of `context` -- the one a caller-owned target was
 * given, or the one mln_opengl_owned_texture_acquire_frame() reported -- and
 * `width` and `height` are its size in device pixels. The destination is the
 * canvas the context was created against, so a context created for a canvas the
 * host transferred puts the frame on the page, with the pixels never leaving
 * the GPU.
 *
 * **This does not present by itself.** The browser composites the canvas when
 * the task that drew into it ends, which for a page host is when the owner
 * thread finishes the batch of work this was submitted in. Nothing here forces
 * that, and nothing can: `emscripten_webgl_commit_frame` is a documented no-op
 * in this emsdk, and an implicit-swap context has no other flip to ask for.
 *
 * **Rows are not reversed.** `srcY0` maps to `dstY0`, which puts the texture's
 * GL-origin row at the framebuffer's GL-origin row -- the same place a surface
 * session's own frame lands in the same framebuffer. Reversing here would make
 * a texture session's page output the mirror of a surface session's.
 *
 * A blit is clipped by the draw framebuffer's scissor rectangle, and MapLibre's
 * GL backend leaves the scissor enabled between frames, so the scissor is
 * disabled for the blit and put back afterwards along with both framebuffer
 * bindings. Leaving any of the three changed would be state the next frame
 * renders against without knowing.
 *
 * Returns false when the context cannot be made current, when the extent does
 * not fit, when the texture cannot be attached to a framebuffer, or when the
 * blit reported a GL error.
 */
MLN_API bool mln_browser_webgl_present_texture_here(
  int32_t context, uint32_t texture, uint32_t width, uint32_t height
) MLN_NOEXCEPT {
  if (
    texture == 0 || !mln_browser_webgl_extent_fits(width, height) ||
    !mln_browser_webgl_bind(context)
  ) {
    return false;
  }

  GLint previous_read = 0;
  GLint previous_draw = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
  const GLboolean scissored = glIsEnabled(GL_SCISSOR_TEST);
  mln_browser_webgl_clear_errors();

  GLuint framebuffer = 0;
  glGenFramebuffers(1, &framebuffer);
  glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
  glFramebufferTexture2D(
    GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, (GLuint)texture, 0
  );
  bool presented =
    glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

  if (presented) {
    // Zero is the canvas's own framebuffer, which is the one the browser
    // composites; naming a framebuffer of our own would present nothing.
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    if (scissored) {
      glDisable(GL_SCISSOR_TEST);
    }
    // GL_NEAREST because source and destination are the same size, so there is
    // nothing to filter, and because a multi-sampled or format-converting blit
    // is the only case where GL_LINEAR is even allowed.
    glBlitFramebuffer(
      0, 0, (GLsizei)width, (GLsizei)height, 0, 0, (GLsizei)width,
      (GLsizei)height, GL_COLOR_BUFFER_BIT, GL_NEAREST
    );
    presented = glGetError() == GL_NO_ERROR;
    if (scissored) {
      glEnable(GL_SCISSOR_TEST);
    }
  }

  glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previous_read);
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previous_draw);
  glDeleteFramebuffers(1, &framebuffer);
  return presented;
}

typedef struct mln_browser_webgl_texture_create_request {
  int32_t context;
  uint32_t width;
  uint32_t height;
  uint32_t* out_texture;
} mln_browser_webgl_texture_create_request;

static void mln_browser_webgl_run_texture_create(void* argument) {
  mln_browser_webgl_texture_create_request* request = argument;
  // Written before this returns, and the dispatcher posts the completion only
  // afterwards, so a host that reads it once its token comes back reads a value
  // this thread has finished writing.
  *request->out_texture = mln_browser_webgl_texture_create_here(
    request->context, request->width, request->height
  );
  free(request);
}

/**
 * Creates a texture on a dispatcher's thread.
 *
 * mln_browser_webgl_texture_create_here()'s work, placed on the thread that
 * owns the context the same way mln_browser_webgl_context_create() places its
 * own. `token` follows the rules mln_browser_dispatcher_submit() sets.
 *
 * `out_texture` is host memory that must stay valid and untouched until the
 * completion for `token` arrives, because the owner thread is what writes it.
 * It receives the texture name, or zero when creation failed.
 *
 * Returns false when the submission was refused, in which case nothing runs, no
 * completion follows `token`, and `out_texture` is unwritten.
 */
MLN_API bool mln_browser_webgl_texture_create(
  mln_browser_dispatcher* dispatcher, int32_t context, uint32_t width,
  uint32_t height, uint32_t* out_texture, uint32_t token
) MLN_NOEXCEPT {
  if (dispatcher == NULL || out_texture == NULL) {
    return false;
  }
  mln_browser_webgl_texture_create_request* request =
    calloc(1, sizeof(*request));
  if (request == NULL) {
    return false;
  }
  request->context = context;
  request->width = width;
  request->height = height;
  request->out_texture = out_texture;
  if (!mln_browser_dispatcher_submit_task(
        dispatcher, mln_browser_webgl_run_texture_create, request, token
      )) {
    free(request);
    return false;
  }
  return true;
}

typedef struct mln_browser_webgl_texture_destroy_request {
  int32_t context;
  uint32_t texture;
} mln_browser_webgl_texture_destroy_request;

static void mln_browser_webgl_run_texture_destroy(void* argument) {
  mln_browser_webgl_texture_destroy_request* request = argument;
  mln_browser_webgl_texture_destroy_here(request->context, request->texture);
  free(request);
}

/**
 * Destroys a texture on the dispatcher's thread that created it.
 *
 * The ordering rule is mln_browser_webgl_texture_destroy_here()'s: every render
 * target that borrowed the texture is detached or destroyed first. Returns
 * false when the submission was refused, in which case nothing is destroyed and
 * no completion follows `token`.
 */
MLN_API bool mln_browser_webgl_texture_destroy(
  mln_browser_dispatcher* dispatcher, int32_t context, uint32_t texture,
  uint32_t token
) MLN_NOEXCEPT {
  if (dispatcher == NULL) {
    return false;
  }
  // A request rather than the argument itself, because a context handle and a
  // texture name are two 32-bit values and a pointer on this target holds one.
  mln_browser_webgl_texture_destroy_request* request =
    calloc(1, sizeof(*request));
  if (request == NULL) {
    return false;
  }
  request->context = context;
  request->texture = texture;
  if (!mln_browser_dispatcher_submit_task(
        dispatcher, mln_browser_webgl_run_texture_destroy, request, token
      )) {
    free(request);
    return false;
  }
  return true;
}

typedef struct mln_browser_webgl_present_request {
  int32_t context;
  uint32_t texture;
  uint32_t width;
  uint32_t height;
  int32_t* out_ok;
} mln_browser_webgl_present_request;

static void mln_browser_webgl_run_present(void* argument) {
  mln_browser_webgl_present_request* request = argument;
  *request->out_ok =
    mln_browser_webgl_present_texture_here(
      request->context, request->texture, request->width, request->height
    )
      ? 1
      : 0;
  free(request);
}

/**
 * Presents a rendered texture on the dispatcher's thread that owns its context.
 *
 * mln_browser_webgl_present_texture_here()'s work, placed on the owner thread,
 * which is what a page host calls: the context, the texture, and the canvas all
 * belong to that thread, and so does the task whose ending composites the
 * canvas.
 *
 * `out_ok` is host memory that must stay valid and untouched until the
 * completion for `token` arrives; it receives one when the frame was blitted.
 *
 * Returns false when the submission was refused, in which case nothing runs and
 * no completion follows `token`.
 */
MLN_API bool mln_browser_webgl_present_texture(
  mln_browser_dispatcher* dispatcher, int32_t context, uint32_t texture,
  uint32_t width, uint32_t height, int32_t* out_ok, uint32_t token
) MLN_NOEXCEPT {
  if (dispatcher == NULL || out_ok == NULL) {
    return false;
  }
  mln_browser_webgl_present_request* request = calloc(1, sizeof(*request));
  if (request == NULL) {
    return false;
  }
  request->context = context;
  request->texture = texture;
  request->width = width;
  request->height = height;
  request->out_ok = out_ok;
  if (!mln_browser_dispatcher_submit_task(
        dispatcher, mln_browser_webgl_run_present, request, token
      )) {
    free(request);
    return false;
  }
  return true;
}

typedef struct mln_browser_webgl_read_request {
  int32_t context;
  uint32_t texture;
  uint32_t width;
  uint32_t height;
  uint8_t* out_pixels;
  size_t out_capacity;
  int32_t* out_ok;
} mln_browser_webgl_read_request;

static void mln_browser_webgl_run_read(void* argument) {
  mln_browser_webgl_read_request* request = argument;
  *request->out_ok =
    mln_browser_webgl_read_pixels_here(
      request->context, request->texture, request->width, request->height,
      request->out_pixels, request->out_capacity
    )
      ? 1
      : 0;
  free(request);
}

/**
 * Reads a rendered frame on the dispatcher's thread that owns its context.
 *
 * mln_browser_webgl_read_pixels_here()'s work, placed on the owner thread,
 * which is what a page host calls: the context, the texture, and the canvas all
 * belong to that thread.
 *
 * `out_pixels` and `out_ok` are host memory that must stay valid and untouched
 * until the completion for `token` arrives; `out_ok` receives one when the
 * frame was read and zero when it was not.
 *
 * Returns false when the submission was refused, in which case nothing runs and
 * no completion follows `token`.
 */
MLN_API bool mln_browser_webgl_read_pixels(
  mln_browser_dispatcher* dispatcher, int32_t context, uint32_t texture,
  uint32_t width, uint32_t height, uint8_t* out_pixels, size_t out_capacity,
  int32_t* out_ok, uint32_t token
) MLN_NOEXCEPT {
  if (dispatcher == NULL || out_pixels == NULL || out_ok == NULL) {
    return false;
  }
  mln_browser_webgl_read_request* request = calloc(1, sizeof(*request));
  if (request == NULL) {
    return false;
  }
  request->context = context;
  request->texture = texture;
  request->width = width;
  request->height = height;
  request->out_pixels = out_pixels;
  request->out_capacity = out_capacity;
  request->out_ok = out_ok;
  if (!mln_browser_dispatcher_submit_task(
        dispatcher, mln_browser_webgl_run_read, request, token
      )) {
    free(request);
    return false;
  }
  return true;
}
