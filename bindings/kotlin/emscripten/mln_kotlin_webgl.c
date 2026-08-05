// The WebGL work Kotlin does on the thread its maps render on.
//
// Every other platform hands a session a context the host made with its own
// platform API. A browser host cannot: mln_webgl_context_descriptor.context is
// an index into this Emscripten module's context table, so a context the page
// created with canvas.getContext("webgl2") names nothing this module can look
// up. The context is made here instead, on the thread that renders, because a
// WebGL context belongs to the thread that created it.
//
// This is C rather than Kotlin externs for one reason:
// EmscriptenWebGLContextAttributes is a sixteen-field struct in the emsdk
// sysroot, which the offset generator never sees. Hardcoded offsets would
// survive an emsdk bump as a context that quietly falls back to WebGL 1.
//
// Every entry point restores the GL state it changed. MapLibre's GL backend
// remembers what it last set and skips a redundant call, so state left changed
// behind its back is state the next frame renders against without knowing.

#include <GLES3/gl3.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// The canvas registry, from mln_kotlin_host.js.
void mln_kotlin_canvas_register(const char* name, int width, int height);
void mln_kotlin_canvas_unregister(const char* name);
int mln_kotlin_canvas_size(const char* name, int width, int height);

// Long enough for an element id a host would write, and bounded because the
// selector below is built on the stack. A longer id is refused rather than
// truncated, because a truncated id names a different canvas or none.
#define MLN_KOTLIN_CANVAS_ID_BYTES 64

// A rectangle in device pixels that both a texture and a readback have to fit.
// GLsizei is signed and an RGBA8 image of this size has to fit a size_t, so the
// bound keeps either from wrapping into a smaller number than it describes.
static bool mln_kotlin_extent_fits(uint32_t width, uint32_t height) {
  const uint32_t limit = 16384;
  return width > 0 && height > 0 && width <= limit && height <= limit;
}

// Discards errors left by earlier work so that a check afterwards reports this
// file's own. Bounded because a lost context reports the loss for as long as it
// stays lost, and an unbounded drain would never end.
static void mln_kotlin_clear_gl_errors(void) {
  for (int guard = 0; guard < 16; guard += 1) {
    if (glGetError() == GL_NO_ERROR) {
      return;
    }
  }
}

// Makes a context current, or reports that the handle names no context this
// thread can use. Every entry point below starts here, because a render session
// restores whatever was current before its frame.
static bool mln_kotlin_bind(int32_t context) {
  if (context <= 0) {
    return false;
  }
  return emscripten_webgl_make_context_current(
           (EMSCRIPTEN_WEBGL_CONTEXT_HANDLE)context
         ) == EMSCRIPTEN_RESULT_SUCCESS;
}

/**
 * Registers a private OffscreenCanvas on this thread under name.
 *
 * This is the canvas a host that reads frames back wants: it is never
 * displayed, and a WebGL2 context cannot exist without some canvas. A canvas
 * the page displays reaches this thread through -sOFFSCREENCANVASES_TO_PTHREAD
 * instead, and is already registered under its element id when Kotlin starts.
 *
 * The caller owns the registration and removes it with
 * mln_kotlin_webgl_canvas_destroy(). Returns false for a name of 64 bytes or
 * longer, which mln_kotlin_webgl_context_create() could not build a selector
 * for, and for an extent outside 1 to 16384 pixels.
 */
EMSCRIPTEN_KEEPALIVE bool mln_kotlin_webgl_canvas_create(
  const char* name, uint32_t width, uint32_t height
) {
  if (
    name == NULL || name[0] == '\0' || !mln_kotlin_extent_fits(width, height) ||
    strlen(name) >= MLN_KOTLIN_CANVAS_ID_BYTES
  ) {
    return false;
  }
  mln_kotlin_canvas_register(name, (int)width, (int)height);
  return true;
}

/**
 * Removes a canvas registration this thread created.
 *
 * Call this after the context created against the canvas is destroyed. A canvas
 * that reached this thread through -sOFFSCREENCANVASES_TO_PTHREAD stays
 * registered for the thread's lifetime, because the page still displays that
 * element and the transfer cannot be repeated.
 */
EMSCRIPTEN_KEEPALIVE void mln_kotlin_webgl_canvas_destroy(const char* name) {
  if (name != NULL) {
    mln_kotlin_canvas_unregister(name);
  }
}

/**
 * Sizes the drawing buffer of a registered canvas, or reports that the registry
 * holds no usable canvas under name.
 *
 * A surface session renders into the default framebuffer of its canvas, and
 * that framebuffer is only as large as the canvas. So a host that changes such
 * a session's extent changes this too, in two steps: this call, and then
 * mln_render_session_resize() or mln_opengl_surface_set_target() with the
 * matching extent. Neither implies the other.
 *
 * Resizing reallocates the drawing buffer and nothing else, so every texture,
 * buffer, and program the session built stays as it was. A texture session
 * draws into a framebuffer of its own and has no reason to call this.
 */
EMSCRIPTEN_KEEPALIVE bool mln_kotlin_webgl_canvas_resize(
  const char* name, uint32_t width, uint32_t height
) {
  if (name == NULL || !mln_kotlin_extent_fits(width, height)) {
    return false;
  }
  return mln_kotlin_canvas_size(name, (int)width, (int)height) != 0;
}

/**
 * Creates a WebGL2 context against a registered canvas, or returns zero.
 *
 * The context belongs to the calling thread, which must be the thread that owns
 * the render session it is given to. Zero is what a failure reports and is also
 * the value mln_webgl_context_descriptor refuses, so a host that passes the
 * result on unchecked is rejected there rather than rendering into nothing.
 *
 * width and height size the canvas's drawing buffer, which for a surface
 * session is that session's physical extent and for a texture session only has
 * to be positive. A canvas with no registration under name, and an id of 64
 * bytes or longer, are both refused.
 *
 * The context is left current on this thread, which is what a host needs when
 * it issues its own GL calls right after creating one.
 */
EMSCRIPTEN_KEEPALIVE int32_t mln_kotlin_webgl_context_create(
  const char* name, uint32_t width, uint32_t height
) {
  if (
    name == NULL || name[0] == '\0' || !mln_kotlin_extent_fits(width, height) ||
    strlen(name) >= MLN_KOTLIN_CANVAS_ID_BYTES
  ) {
    return 0;
  }
  // Sized here so that width and height mean the same thing for a transferred
  // canvas as for a private one. A transferred canvas arrives at whatever its
  // element measured, which is a CSS layout size rather than device pixels.
  if (mln_kotlin_canvas_size(name, (int)width, (int)height) == 0) {
    return 0;
  }

  EmscriptenWebGLContextAttributes attributes;
  emscripten_webgl_init_context_attributes(&attributes);
  // WebGL2 is the GLES 3.0 that MapLibre's OpenGL backend targets.
  attributes.majorVersion = 2;
  attributes.minorVersion = 0;
  attributes.depth = EM_TRUE;
  attributes.stencil = EM_TRUE;
  attributes.antialias = EM_FALSE;
  // A host reads a surface session's frame out of this canvas's default
  // framebuffer in a later task than the one that drew it. A displayed canvas
  // is composited at the end of each task and would otherwise be cleared as
  // part of that. A texture session never touches this buffer.
  attributes.preserveDrawingBuffer = EM_TRUE;
  // Implicit swap is what presenting depends on: the browser pushes what this
  // context drew when the task that drew it ends. emscripten_webgl_commit_frame
  // is a documented no-op in this emsdk, so explicit control would present
  // nothing at all.
  attributes.explicitSwapControl = EM_FALSE;
  // Proxying the context to the page would turn every GL call MapLibre makes
  // into a cross-thread round trip, and needs -sOFFSCREEN_FRAMEBUFFER, which
  // this module does not link with.
  attributes.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_DISALLOW;

  // A registry key with Emscripten's prefix on it, and deliberately not a CSS
  // selector, so escaping the id here would be a bug.
  // findCanvasEventTarget(), the resolver under -sOFFSCREENCANVAS_SUPPORT,
  // drops one leading character and looks the remainder up in
  // GL.offscreenCanvases as a property name.
  char target[MLN_KOTLIN_CANVAS_ID_BYTES + 1];
  (void)snprintf(target, sizeof(target), "#%s", name);
  const EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context =
    emscripten_webgl_create_context(target, &attributes);
  if (context == 0) {
    return 0;
  }
  if (
    emscripten_webgl_make_context_current(context) != EMSCRIPTEN_RESULT_SUCCESS
  ) {
    (void)emscripten_webgl_destroy_context(context);
    return 0;
  }
  return (int32_t)context;
}

/**
 * Destroys a context on the thread that created it.
 *
 * Call this once every render target using the context is detached or
 * destroyed, because the C API borrows the handle for a target's lifetime.
 * Destroying a context releases every object made in it, including textures.
 * The canvas registration outlives the context: release it separately with
 * mln_kotlin_webgl_canvas_destroy().
 */
EMSCRIPTEN_KEEPALIVE void mln_kotlin_webgl_context_destroy(int32_t context) {
  if (context > 0) {
    (void)emscripten_webgl_destroy_context(
      (EMSCRIPTEN_WEBGL_CONTEXT_HANDLE)context
    );
  }
}

/**
 * Creates an RGBA8 texture in a context this thread owns, or returns zero.
 *
 * This is where a caller-owned target's texture comes from, and the name it
 * reports is what mln_opengl_borrowed_texture_descriptor.texture carries. Pass
 * the same context in the descriptor that names the texture, because the
 * session attaches the texture to a framebuffer of that context.
 *
 * The texture belongs to the caller. Nothing here tracks it, a render target
 * only borrows it, and mln_kotlin_webgl_texture_destroy() releases it.
 */
EMSCRIPTEN_KEEPALIVE uint32_t mln_kotlin_webgl_texture_create(
  int32_t context, uint32_t width, uint32_t height
) {
  if (!mln_kotlin_extent_fits(width, height) || !mln_kotlin_bind(context)) {
    return 0;
  }

  GLint previous = 0;
  glGetIntegerv(GL_TEXTURE_BINDING_2D, &previous);
  mln_kotlin_clear_gl_errors();

  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_RGBA8, (GLsizei)width, (GLsizei)height, 0, GL_RGBA,
    GL_UNSIGNED_BYTE, NULL
  );
  // One level, because a render target is drawn at its own size and never
  // sampled from a smaller one. A texture asking for mipmaps would be
  // incomplete until something built them.
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
 * Call this once every render target that borrows the texture is detached or
 * destroyed, because a session whose target names a destroyed texture renders
 * into nothing. A handle naming a context that is already gone is ignored.
 */
EMSCRIPTEN_KEEPALIVE void mln_kotlin_webgl_texture_destroy(
  int32_t context, uint32_t texture
) {
  if (texture == 0 || !mln_kotlin_bind(context)) {
    return;
  }
  GLuint name = texture;
  glDeleteTextures(1, &name);
}

/**
 * Reads a rendered frame out of a context this thread owns, or returns false.
 *
 * mln_texture_read_premultiplied_rgba8() covers session-owned texture targets
 * and refuses the other two families, because elsewhere a host reads its own
 * texture with its own graphics API. This is that read. texture names a
 * two-dimensional texture of context, or is zero for the default framebuffer of
 * the context's canvas, which is what a surface session renders into.
 *
 * width and height are the region to read, measured from the target's origin,
 * and out_pixels receives width * height * 4 bytes of RGBA8 that out_capacity
 * has to cover. Row zero is the bottom row, because that is where GL's origin
 * is and nothing here flips the image. A host that wants the top-down rows that
 * ImageData and mln_texture_read_premultiplied_rgba8() use reverses them
 * itself.
 *
 * This is a synchronous read of the GPU that stalls the calling thread until
 * the frame is done. A host that only wants to show a frame draws with the
 * texture instead.
 *
 * Returns false when the context cannot be made current, when the extent does
 * not fit the capacity, when the texture cannot be attached to a framebuffer,
 * or when the read reported a GL error. out_pixels is then unspecified rather
 * than unwritten, because a read that fails partway has already written.
 */
EMSCRIPTEN_KEEPALIVE bool mln_kotlin_webgl_read_pixels(
  int32_t context, uint32_t texture, uint32_t width, uint32_t height,
  uint8_t* out_pixels, size_t out_capacity
) {
  if (
    out_pixels == NULL || !mln_kotlin_extent_fits(width, height) ||
    out_capacity < (size_t)width * (size_t)height * 4U ||
    !mln_kotlin_bind(context)
  ) {
    return false;
  }

  // Put back before this returns. A read ignores the scissor rectangle, so only
  // the binding matters here, but MapLibre leaves its own framebuffer bound
  // between frames and assumes it is still there.
  GLint previous = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous);
  mln_kotlin_clear_gl_errors();

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
    // is the default pack alignment, so no state is left changed for the next
    // frame.
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
 * This is how a texture session's frame reaches the page. texture names a
 * two-dimensional texture of context, and width and height are its size in
 * device pixels. The destination is the canvas the context was created against,
 * so a context created for a canvas the page transferred puts the frame on the
 * page with the pixels never leaving the GPU. A surface session needs none of
 * this, because it already renders into that framebuffer.
 *
 * The browser composites the canvas when the task that drew into it ends, which
 * is what presents the frame. Nothing here forces that, and nothing can:
 * emscripten_webgl_commit_frame is a documented no-op in this emsdk, and an
 * implicit-swap context has no other flip to ask for.
 *
 * Rows are not reversed. srcY0 maps to dstY0, which puts the texture's
 * GL-origin row where a surface session's own frame lands in the same
 * framebuffer.
 *
 * A blit is clipped by the draw framebuffer's scissor rectangle, and MapLibre's
 * GL backend leaves the scissor enabled between frames, so the scissor is
 * disabled for the blit and put back afterwards along with both framebuffer
 * bindings.
 *
 * Returns false when the context cannot be made current, when the extent does
 * not fit, when the texture cannot be attached to a framebuffer, or when the
 * blit reported a GL error.
 */
EMSCRIPTEN_KEEPALIVE bool mln_kotlin_webgl_present_texture(
  int32_t context, uint32_t texture, uint32_t width, uint32_t height
) {
  if (
    texture == 0 || !mln_kotlin_extent_fits(width, height) ||
    !mln_kotlin_bind(context)
  ) {
    return false;
  }

  GLint previous_read = 0;
  GLint previous_draw = 0;
  glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous_read);
  glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previous_draw);
  const GLboolean scissored = glIsEnabled(GL_SCISSOR_TEST);
  mln_kotlin_clear_gl_errors();

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
    // GL_NEAREST because source and destination are the same size, and because
    // a multi-sampled or format-converting blit is the only case where
    // GL_LINEAR is allowed.
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
