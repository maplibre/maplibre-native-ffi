/**
 * @file maplibre_native_c/render_session.h
 * Render a map's frames to an attached target, and manage the session.
 */

#ifndef MAPLIBRE_NATIVE_C_RENDER_SESSION_H
#define MAPLIBRE_NATIVE_C_RENDER_SESSION_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "completion.h"
#include "render_target.h"

#ifdef __cplusplus
extern "C" {
#endif

/** How a frame request finished. */
typedef enum mln_render_result : uint32_t {
  MLN_RENDER_RESULT_RENDERED = 0,
  /** The map had nothing new to draw. */
  MLN_RENDER_RESULT_NO_UPDATE = 1,
  /**
   * The target couldn't take a frame, such as while a window is hidden or
   * after mln_render_session_release_target(). Request a frame again later.
   */
  MLN_RENDER_RESULT_TARGET_NOT_READY = 2,
  /** A later request with the same flags replaced this one. */
  MLN_RENDER_RESULT_SUPERSEDED = 3,
  /**
   * The session failed, and mln_render_session_get_failure() reports why. Once
   * mln_render_session_dispose() completes, you can attach a new session.
   */
  MLN_RENDER_RESULT_FAILED = 4,
} mln_render_result;

/** Where a render session is in its life. */
typedef enum mln_render_session_state : uint32_t {
  MLN_RENDER_SESSION_STATE_ATTACHING = 1U,
  MLN_RENDER_SESSION_STATE_ATTACHED = 2U,
  MLN_RENDER_SESSION_STATE_DETACHING = 3U,
  MLN_RENDER_SESSION_STATE_DETACHED = 4U,
  MLN_RENDER_SESSION_STATE_FAILED = 5U,
  MLN_RENDER_SESSION_STATE_ABANDONED = 6U,
} mln_render_session_state;

/** How a frame request behaves. */
typedef enum MLN_BINDING("kind=bitmask") mln_frame_demand_flag : uint32_t {
  /**
   * Draws a frame even when nothing changed since the last one. It can't be
   * combined with MLN_FRAME_DEMAND_WAIT_FOR_UPDATE.
   */
  MLN_FRAME_DEMAND_FORCE = 1U << 0U,
  /** Shows the frame on a surface target. */
  MLN_FRAME_DEMAND_PRESENT = 1U << 1U,
  /**
   * When the map has nothing new to draw, waits for its next update instead of
   * finishing with MLN_RENDER_RESULT_NO_UPDATE.
   */
  MLN_FRAME_DEMAND_WAIT_FOR_UPDATE = 1U << 2U,
} mln_frame_demand_flag;

/** A set of mln_frame_demand_flag bits. */
typedef uint32_t mln_frame_demand_flags;

/**
 * The token of the frame results that a session adds when it renders an image
 * for mln_map_request_still_image(). Your own requests can't use it.
 */
#define MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE UINT64_MAX

/** A request for one frame. */
typedef struct mln_frame_demand {
  uint32_t size;
  mln_frame_demand_flags flags;
  /** A value of your choice that the request's frame result carries. */
  uint64_t token;
} mln_frame_demand;

/** How one frame request finished. */
typedef struct mln_render_frame_result {
  uint32_t size;
  mln_render_result disposition;
  uint64_t token;
  /**
   * The map update that a rendered frame shows. The frame includes a command's
   * change when this is at least the command's
   * mln_completion_result.render_update_generation.
   */
  uint64_t map_update_generation;
  uint64_t generation;
  uint64_t frame_generation;
  bool needs_repaint;
  /** The camera and extent that a rendered frame shows. */
  mln_map_view view;
  mln_render_mode mode;
} mln_render_frame_result;

/** The frame results that mln_render_session_drain_frame_results() took. */
typedef struct mln_render_frame_batch_view {
  uint32_t size;
  uint32_t result_size;
  const mln_render_frame_result* results
    MLN_BINDING("length=result_count;stride=result_size");
  size_t result_count;
} mln_render_frame_batch_view MLN_BINDING("kind=slice");

/** The state of a render session at one moment. */
typedef struct mln_render_session_snapshot {
  uint32_t size;
  mln_render_session_state state;
  mln_logical_extent extent;
  uint64_t generation;
  uint64_t frame_generation;
  uint32_t acquired_frame_count;
  /**
   * MLN_STATUS_OK until the session fails. Then MLN_STATUS_TARGET_LOST when
   * it failed because it lost its target, or MLN_STATUS_NATIVE_ERROR when it
   * failed for another reason.
   */
  mln_status failure_status;
} mln_render_session_snapshot;

/** What mln_render_session_abandon() did with a session's graphics objects. */
typedef enum mln_render_abandon_disposition : uint32_t {
  MLN_RENDER_ABANDON_DISPOSITION_CLEAN = 0U,
  /**
   * It kept some objects that it couldn't destroy safely. They stay allocated
   * until the process exits.
   */
  MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED = 1U,
} mln_render_abandon_disposition;

/** The outcome of mln_render_session_abandon(). */
typedef struct mln_render_abandon_result {
  uint32_t size;
  mln_render_abandon_disposition disposition;
  uint32_t quarantined_resource_count;
  uint32_t reserved MLN_BINDING("kind=reserved");
} mln_render_abandon_result;

/** Reads what the session supports. */
MLN_API mln_status mln_render_session_get_capabilities(
  mln_render_session session,
  mln_render_session_capabilities* out_capabilities
    MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Reads the session's current state. */
MLN_BINDING("execution=snapshot")
MLN_API mln_status mln_render_session_get_snapshot(
  mln_render_session session,
  mln_render_session_snapshot* out_snapshot MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads the message of the error that made the session fail.
 *
 * mln_render_session_snapshot.failure_status reports the error's status. The
 * message is empty while the session hasn't failed, and doesn't change once
 * it fails.
 *
 * Lifetimes:
 * The function writes the message and a terminating NUL to `out_message` only
 * when `message_capacity` is greater than the message's length. It always
 * sets `out_message_length` to the length without the NUL.
 */
MLN_API mln_status mln_render_session_get_failure(
  mln_render_session session,
  char* out_message MLN_BINDING(
    "direction=out;length=message_capacity;result_length=out_message_length"
  ),
  size_t message_capacity,
  size_t* out_message_length MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Asks the session to render a frame.
 *
 * Call it once the attach completes. Every request finishes with one frame
 * result that carries its token, and mln_render_session_drain_frame_results()
 * takes it. A request without MLN_FRAME_DEMAND_FORCE replaces each pending
 * request with the same flags.
 *
 * Returns:
 * - MLN_STATUS_INVALID_STATE when the session has failed.
 */
MLN_API mln_status mln_render_session_request_frame(
  mln_render_session session, const mln_frame_demand* demand,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Takes the frame results that the session has queued.
 *
 * When no result is queued, it succeeds and writes the null handle and an
 * empty view.
 *
 * Lifetimes:
 * `out_view` stays valid until you pass `out_batch` to
 * mln_render_frame_batch_release().
 */
MLN_BINDING("execution=event_batch")
MLN_API mln_status mln_render_session_drain_frame_results(
  mln_render_session session,
  mln_render_frame_batch* out_batch MLN_BINDING("direction=out;nullable=true"),
  mln_render_frame_batch_view* out_view MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Frees a batch of frame results. */
MLN_API void mln_render_frame_batch_release(
  mln_render_frame_batch batch
) MLN_NOEXCEPT;

/**
 * Takes the newest rendered frame from the session's texture ring, if one is
 * ready.
 *
 * Older frames that you haven't acquired are dropped, and the session can
 * render over a frame before you acquire it. While you hold a frame in every
 * texture of the ring, new frames wait until you release one. When no frame is
 * ready, it succeeds and writes the null handle.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when the session lacks
 *   MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION.
 * - MLN_STATUS_INVALID_STATE when the session has failed.
 */
MLN_API mln_status mln_render_session_acquire_frame(
  mln_render_session session,
  mln_acquired_frame* out_frame MLN_BINDING("direction=out;nullable=true"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads the frame result of an acquired frame.
 *
 * It succeeds until you release or discard the frame, whatever the session's
 * state.
 */
MLN_API mln_status mln_acquired_frame_get_result(
  mln_acquired_frame frame,
  mln_render_frame_result* out_result MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads what to wait for before you read an acquired frame's texture.
 *
 * Returns:
 * - MLN_STATUS_TARGET_LOST when the session failed, or you detached, abandoned,
 *   or disposed it.
 *
 * Lifetimes:
 * The object that `out_sync` names belongs to the session. It stays valid until
 * you release or discard the frame, or the session is abandoned or disposed.
 */
MLN_BINDING("view_owner=frame")
MLN_API mln_status mln_acquired_frame_get_producer_sync(
  mln_acquired_frame frame, mln_gpu_sync* out_sync MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gives an acquired frame back to the session once you're done reading it.
 *
 * `consumer_completion` says when your GPU work that reads the texture
 * finishes. Pass MLN_GPU_SYNC_CPU_COMPLETE when that work has already finished.
 * The session waits for that point before it renders into the texture again.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when
 *   mln_render_session_capabilities.consumer_sync_kinds lacks the kind of
 *   `consumer_completion`.
 *
 * Lifetimes:
 * On success, the session takes ownership of a sync file descriptor or an
 * OpenGL fence that `consumer_completion` names.
 */
MLN_API mln_status mln_acquired_frame_release(
  mln_acquired_frame frame, const mln_gpu_sync* consumer_completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Keeps an acquired frame's texture and sync objects in place while you use
 * them.
 *
 * Until mln_acquired_frame_view_end() ends the view,
 * mln_acquired_frame_release() and mln_render_session_abandon() fail with
 * MLN_STATUS_BUSY.
 *
 * Returns:
 * - MLN_STATUS_TARGET_LOST when the session failed, or you detached, abandoned,
 *   or disposed it.
 *
 * Lifetimes:
 * Pass `out_scope` to mln_acquired_frame_view_end().
 */
MLN_API mln_status mln_acquired_frame_view_begin(
  mln_acquired_frame frame,
  void** out_scope MLN_BINDING("direction=out;kind=context"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Ends a view that mln_acquired_frame_view_begin() began. */
MLN_API void mln_acquired_frame_view_end(
  void* scope MLN_BINDING("kind=context")
) MLN_NOEXCEPT;

/**
 * Resizes the session's target and its map.
 *
 * `extent` must keep the session's scale factor. To change the scale factor,
 * give the session a new target or attach a new session. Release every
 * acquired frame first.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when the session renders into a borrowed ring. Give
 *   it new textures of the new size instead.
 *
 * Completes with:
 * - MLN_STATUS_TARGET_LOST when the session loses its target before the resize
 *   applies.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_render_session_resize(
  mln_render_session session, mln_logical_extent extent,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Lets go of the session's surface or textures, and keeps the rest of the
 * session.
 *
 * Use it when the platform takes a surface away, such as when your app goes to
 * the background. Release every acquired frame first. Until you give the
 * session a new target, frame requests finish with
 * MLN_RENDER_RESULT_TARGET_NOT_READY. Once the release completes, you can
 * destroy the old target.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when the session renders offscreen.
 *
 * Completes with:
 * - MLN_STATUS_UNSUPPORTED when the target is a WebGL canvas.
 * - MLN_STATUS_TARGET_LOST when the session loses its target before the
 *   release runs.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_release_target(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Ends the session's pending frame requests and waits for its earlier work.
 *
 * Frame requests that haven't started, including those that wait for a map
 * update, finish with MLN_RENDER_RESULT_NO_UPDATE.
 *
 * Completes with:
 * - MLN_STATUS_TARGET_LOST when the session loses its target before the
 *   barrier runs.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_barrier(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Frees memory that the session's renderer can rebuild, such as caches.
 *
 * Completes with:
 * - MLN_STATUS_TARGET_LOST when the session loses its target before the work
 *   runs.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_reduce_memory_use(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Clears the data that the session's renderer holds, such as loaded tiles, and
 * draws the map again.
 *
 * Completes with:
 * - MLN_STATUS_TARGET_LOST when the session loses its target before the work
 *   runs.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_clear_data(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Logs debugging information about the session's renderer.
 *
 * Completes with:
 * - MLN_STATUS_TARGET_LOST when the session loses its target before the work
 *   runs.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_dump_debug_logs(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Runs the session's ready graphics work on your graphics thread.
 *
 * Use it with MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, and call it from the
 * same thread every time. Call it each time
 * mln_render_session_attach_options.driver_work_wake is called, even while you
 * request no frames, and until a detach completes. It returns once no work is
 * ready.
 */
MLN_API mln_status mln_render_session_service_driver_work(
  mln_render_session session, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Detaches the session from its map and frees its graphics objects.
 *
 * Release every acquired frame first. Pending frame requests finish with
 * MLN_RENDER_RESULT_TARGET_NOT_READY. Once the detach completes, the library no
 * longer uses your graphics objects, and you can attach another session to the
 * map. Dispose the session afterward.
 *
 * Completes with:
 * - MLN_STATUS_TARGET_LOST when the session loses its target before the
 *   detach runs.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_render_session_detach(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Stops the session right away, without running its pending graphics work.
 *
 * Use it when the target's graphics objects are going away and a detach can't
 * finish first. Pending work completes with MLN_STATUS_TARGET_LOST, and
 * pending frame requests finish with MLN_RENDER_RESULT_TARGET_NOT_READY. Once
 * the call returns, the library no longer uses your graphics objects, and you
 * can attach another session to the map. Dispose the session afterward.
 *
 * Returns:
 * - MLN_STATUS_BUSY while mln_render_session_service_driver_work() runs for the
 *   session on another thread, or while a view that
 *   mln_acquired_frame_view_begin() began on one of its frames is open.
 */
MLN_API mln_status mln_render_session_abandon(
  mln_render_session session,
  mln_render_abandon_result* out_result MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases the session in any state.
 *
 * An attached session driven by MLN_RENDER_DRIVER_CORE_WORKER detaches first
 * unless you hold one of its frames. Any other session is abandoned as
 * mln_render_session_abandon() does. To free the graphics objects of a session
 * driven by MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, detach it before you
 * dispose it. Once the disposal completes, the library no longer uses your
 * graphics objects.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_render_session_dispose(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Gives up an acquired frame without saying when your GPU stops reading it.
 *
 * Because your GPU may still read the frame's texture, the session never
 * renders into that texture again.
 */
MLN_API mln_status mln_acquired_frame_discard(
  mln_acquired_frame frame, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_RENDER_SESSION_H
