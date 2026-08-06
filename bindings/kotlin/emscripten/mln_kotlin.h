/**
 * @file mln_kotlin.h
 * The module entry points the Kotlin/Wasm binding calls that the public C API
 * does not provide.
 *
 * Two things a separate WebAssembly module cannot do for itself, and nothing
 * else. It cannot receive a callback raised on a MapLibre worker thread,
 * because a JavaScript function belongs to the agent that defined it and each
 * worker is a different agent; the ring below carries those records to the
 * thread Kotlin runs on. And it cannot create a WebGL context, because
 * mln_webgl_context_descriptor.context indexes this module's own table and
 * EmscriptenWebGLContextAttributes is a sysroot struct the offset generator
 * never sees.
 *
 * Everything else the binding needs is an ordinary C API entry point, declared
 * in generated Kotlin and checked against this module by
 * scripts/check-browser-exports.py.
 *
 * This header targets C23.
 */

#ifndef MLN_KOTLIN_H
#define MLN_KOTLIN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c/callback_adapter.h"
#include "maplibre_native_c/runtime.h"
#include "maplibre_native_c/style.h"

/** What a ring record carries, which selects how its payload is released. */
enum mln_kotlin_record_kind {
  MLN_KOTLIN_RECORD_LOG = 1,
  MLN_KOTLIN_RECORD_LOG_RETIRED = 2,
  MLN_KOTLIN_RECORD_RESOURCE_REQUEST = 3,
  MLN_KOTLIN_RECORD_RESOURCE_PROVIDER_RETIRED = 4,
  MLN_KOTLIN_RECORD_TILE_FETCH = 5,
  MLN_KOTLIN_RECORD_TILE_CANCEL = 6,
};

/**
 * One record on its way from a MapLibre thread to the thread Kotlin runs on.
 *
 * A retirement arrives behind every record it retires, so a drain that stops
 * delivering at the marker is what makes "cleared" mean no later invocation.
 * The two tile kinds mark retirement with a tile_z of 255.
 */
typedef struct mln_kotlin_record {
  uint32_t kind;
  uint32_t tile_z;
  uint32_t tile_x;
  uint32_t tile_y;
  /**
   * The adapter record for MLN_KOTLIN_RECORD_LOG and
   * MLN_KOTLIN_RECORD_RESOURCE_REQUEST, which Kotlin releases with
   * mln_adapter_log_record_destroy() or by completing the request, releasing
   * it, and calling mln_adapter_resource_provider_request_destroy(). The custom
   * geometry callbacks' user_data for the two tile kinds, null for a
   * retirement.
   */
  void* payload;
} mln_kotlin_record;

/**
 * Ends the program with status, for a host that has one -- a test runner.
 *
 * Drops the keepalive that leaves the binding's thread running and forces the
 * exit, because a backend keepalive can outlive it. A host that never calls
 * this keeps the thread parked, which is what a map wants.
 */
void mln_kotlin_exit(int status);

/** Takes the oldest record, or returns false when the ring is empty. */
bool mln_kotlin_take_record(mln_kotlin_record* out);

/** How many records the ring has dropped, cumulative. */
uint64_t mln_kotlin_dropped_records(void);

/**
 * Names the wake source a producing thread signals after pushing.
 *
 * Signalled under the ring lock, so no thread can signal a source Kotlin has
 * already destroyed.
 */
void mln_kotlin_set_wake(mln_wake_source source);

/**
 * Installs the log listener, or updates whether it consumes.
 *
 * The adapter identifies a registration by its state's address and takes no
 * user data, so one state serves the module's lifetime and re-installing it
 * updates consume without retiring anything.
 */
mln_status mln_kotlin_log_install(uint32_t consume);

/** Clears the log listener. */
mln_status mln_kotlin_log_clear(void);

/**
 * The adapter callbacks, by function table index.
 *
 * Kotlin cannot take the address of a wasm function; only C can, and a function
 * has a table index only once something does.
 */
mln_resource_transform_callback mln_kotlin_rewrite_transform_callback(void);
mln_resource_provider_callback mln_kotlin_queued_provider_callback(void);
mln_adapter_queued_resource_request_listener mln_kotlin_resource_request_listener(
);
mln_custom_geometry_source_tile_callback mln_kotlin_tile_fetch_callback(void);
mln_custom_geometry_source_tile_callback mln_kotlin_tile_cancel_callback(void);

/**
 * Registers a private OffscreenCanvas under name.
 *
 * The canvas a host that reads frames back wants: never displayed, and a WebGL2
 * context cannot exist without one. A canvas the page displays arrives instead
 * through -sOFFSCREENCANVASES_TO_PTHREAD, already registered when Kotlin
 * starts. The caller owns the registration.
 *
 * Returns false for a name of 64 bytes or longer, which
 * mln_kotlin_webgl_context_create() could not build a selector for, and for an
 * extent outside 1 to 16384 pixels.
 */
bool mln_kotlin_webgl_canvas_create(
  const char* name, uint32_t width, uint32_t height
);

/**
 * Removes a canvas registration this thread created.
 *
 * Call it after the context created against the canvas is destroyed. A canvas
 * the page transferred stays registered for the thread's lifetime, because the
 * page still displays that element and the transfer cannot be repeated.
 */
void mln_kotlin_webgl_canvas_destroy(const char* name);

/**
 * Sizes a registered canvas's drawing buffer, or reports no usable canvas.
 *
 * A surface session renders into its canvas's default framebuffer, which is
 * only as large as the canvas, so changing such a session's extent takes this
 * call and then mln_render_session_resize() or mln_opengl_surface_set_target().
 * Neither implies the other. Only the drawing buffer is reallocated, so every
 * texture, buffer, and program the session built stays as it was.
 */
bool mln_kotlin_webgl_canvas_resize(
  const char* name, uint32_t width, uint32_t height
);

/**
 * Creates a WebGL2 context against a registered canvas, or returns 0.
 *
 * The extent sizes the canvas's drawing buffer before the context is made, so a
 * caller that registered the canvas at one size and renders at another passes
 * the size it renders at.
 */
int32_t mln_kotlin_webgl_context_create(
  const char* name, uint32_t width, uint32_t height
);

/**
 * Destroys a context on the thread that created it.
 *
 * Call it once every render target using the context is detached or destroyed,
 * because the C API borrows the handle for a target's lifetime. This releases
 * every object made in the context, textures included. The canvas registration
 * outlives it and is released separately.
 */
void mln_kotlin_webgl_context_destroy(int32_t context);

/** Creates an RGBA8 texture in context, or returns 0. */
uint32_t mln_kotlin_webgl_texture_create(
  int32_t context, uint32_t width, uint32_t height
);

/** Destroys a texture created in context. */
void mln_kotlin_webgl_texture_destroy(int32_t context, uint32_t texture);

/**
 * Reads a rendered frame out of a context this thread owns.
 *
 * texture names a two-dimensional texture of context, or is zero for the
 * default framebuffer a surface session renders into. out_pixels receives
 * width * height * 4 bytes of RGBA8, bottom row first, which is GL's order
 * rather than the top-down order mln_texture_read_premultiplied_rgba8() uses.
 *
 * Stalls the calling thread until the frame is done. On failure out_pixels is
 * unspecified rather than unwritten: a read that fails partway has already
 * written.
 */
bool mln_kotlin_webgl_read_pixels(
  int32_t context, uint32_t texture, uint32_t width, uint32_t height,
  uint8_t* out_pixels, size_t out_capacity
);

/**
 * Blits a rendered texture onto the default framebuffer of its context.
 *
 * How a texture session's frame reaches a transferred page canvas without the
 * pixels leaving the GPU. A surface session needs none of this: it already
 * renders into that framebuffer. The browser composites the canvas when the
 * task that drew into it ends, and nothing here can force that sooner.
 */
bool mln_kotlin_webgl_present_texture(
  int32_t context, uint32_t texture, uint32_t width, uint32_t height
);

#endif
