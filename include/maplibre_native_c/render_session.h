/**
 * @file maplibre_native_c/render_session.h
 * Public C API declarations for asynchronous render sessions.
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

/** Terminal disposition of one accepted frame demand. */
typedef enum mln_render_result : uint32_t {
  /**
   * A frame was rendered for acquisition, presentation, or ordered readback.
   *
   * The result states nothing about GPU completion. On a texture ring, the
   * acquired frame's producer synchronization states when the GPU writes to
   * its texture are complete. See mln_acquired_frame_get_producer_sync().
   */
  MLN_RENDER_RESULT_RENDERED = 0,
  /**
   * No newer map update was available, the map had no complete frame to draw,
   * or the update extent differed from the target. The map publishes another
   * update when it has one. A demand with MLN_FRAME_DEMAND_WAIT_FOR_UPDATE
   * waits for that update instead, and finishes with this result only when a
   * barrier ends its wait.
   */
  MLN_RENDER_RESULT_NO_UPDATE = 1,
  /**
   * The target could not produce a frame. The attempt consumes nothing, so a
   * later demand with the same flags renders what this one would have. This
   * result does not cause a map update, so the host demands again when the
   * target can be ready, such as after a paced delay.
   */
  MLN_RENDER_RESULT_TARGET_NOT_READY = 2,
  /**
   * A newer demand with the same flags, neither of them forced, replaced this
   * demand. A still image's demand is never replaced, and never replaces one.
   */
  MLN_RENDER_RESULT_SUPERSEDED = 3,
} mln_render_result;

/** Render-session lifecycle visible in snapshots. */
typedef enum mln_render_session_state : uint32_t {
  MLN_RENDER_SESSION_STATE_ATTACHING = 1U,
  MLN_RENDER_SESSION_STATE_ATTACHED = 2U,
  MLN_RENDER_SESSION_STATE_DETACHING = 3U,
  MLN_RENDER_SESSION_STATE_DETACHED = 4U,
  /**
   * A frame or driver task failed and the session can no longer render. The
   * map reports the failure as MLN_RUNTIME_EVENT_MAP_RENDER_ERROR. A failed
   * attach reports it through its completion instead.
   */
  MLN_RENDER_SESSION_STATE_TARGET_LOST = 5U,
  MLN_RENDER_SESSION_STATE_ABANDONED = 6U,
} mln_render_session_state;

/**
 * Frame-demand policy bits.
 *
 * A demand without MLN_FRAME_DEMAND_FORCE renders only when a newer map update
 * exists, and otherwise finishes with MLN_RENDER_RESULT_NO_UPDATE.
 */
typedef enum MLN_BINDING("kind=bitmask") mln_frame_demand_flag : uint32_t {
  /** Render even when no map update is newer than the last rendered one. */
  MLN_FRAME_DEMAND_FORCE = 1U << 0U,
  /**
   * Present the rendered frame on a target that supports presentation. A
   * presenting target whose demand clears this bit still renders and keeps
   * whatever it presented last. Ignored by targets without presentation.
   */
  MLN_FRAME_DEMAND_PRESENT = 1U << 1U,
  /**
   * Without MLN_FRAME_DEMAND_FORCE, a demand that would finish with
   * MLN_RENDER_RESULT_NO_UPDATE waits instead, and runs again after the map's
   * next update, a target replacement, or an applied resize. A waiting demand
   * holds no ring slot. A later demand with the same flags supersedes it, a
   * barrier ends its wait with MLN_RENDER_RESULT_NO_UPDATE, and detach,
   * abandon, or the quarantine of the ring's last usable slot end it with
   * MLN_RENDER_RESULT_TARGET_NOT_READY.
   * A waiting demand's result can follow the results of demands accepted after
   * it. The flag does not pace: a host that re-arms a waiting demand as each
   * result arrives renders every update the map publishes, and a command that
   * arrives while a frame renders appears in a later frame. To show several
   * commands in one frame, submit them in a command group, or arm demands once
   * per display frame.
   */
  MLN_FRAME_DEMAND_WAIT_FOR_UPDATE = 1U << 2U,
} mln_frame_demand_flag;

/** A set of mln_frame_demand_flag bits. */
typedef uint32_t mln_frame_demand_flags;

/**
 * Token of the frame demands that mln_map_request_still_image() submits.
 *
 * A host demand cannot carry it, so a frame result that does belongs to a still
 * image rather than to any demand the host made.
 */
#define MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE UINT64_MAX

/** One nonblocking request for a frame. */
typedef struct mln_frame_demand {
  uint32_t size;
  /**
   * Frame-demand policy. Zero renders only when a newer map update exists,
   * without presenting.
   */
  mln_frame_demand_flags flags;
  /**
   * Host identity returned with the terminal frame result. Any value except
   * the maximum uint64_t value.
   *
   * MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE names the reserved value.
   */
  uint64_t token;
} mln_frame_demand;

/**
 * Terminal result of one frame demand, held by an owned frame-result batch and
 * copied by mln_acquired_frame_get_result().
 *
 * Step through a batch's records by mln_render_frame_batch_view.result_size
 * rather than by the size of this struct: a later version may append a member
 * and widen the stride. In a batch, each record's size equals that stride.
 */
typedef struct mln_render_frame_result {
  uint32_t size;
  mln_render_result disposition;
  uint64_t token;
  /**
   * Generation of the map render update the demand evaluated. When disposition
   * is MLN_RENDER_RESULT_RENDERED, the frame drew that update; compare it with
   * mln_completion_result.render_update_generation to find the first frame
   * that includes a command.
   */
  uint64_t map_update_generation;
  /**
   * The session's target generation when the demand reached its result. See
   * mln_render_session_snapshot.generation.
   */
  uint64_t generation;
  /**
   * Position of the frame among the frames that the session rendered, counting
   * from one. Zero unless disposition is MLN_RENDER_RESULT_RENDERED.
   */
  uint64_t frame_generation;
  /**
   * Whether the map asked for another frame while it rendered this one, as
   * during an ongoing paint transition. Set only when disposition is
   * MLN_RENDER_RESULT_RENDERED, and false for every other outcome. It matches
   * the needs_repaint field of MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED. A
   * camera transition does not set it: the map publishes an update for each of
   * the transition's frames instead, which a render-if-needed demand renders.
   */
  bool needs_repaint;
  /**
   * The view that this frame drew, from the transform of the map update that
   * it rendered. A host that displays the frame builds its projection with
   * mln_map_projection_create() to place overlays where the frame drew their
   * coordinates. Zero, with a size of zero, unless disposition is
   * MLN_RENDER_RESULT_RENDERED.
   */
  mln_map_view view;
  /**
   * MLN_RENDER_MODE_FULL when the frame drew its update with the style and
   * every tile and image that the update needs loaded. Partial unless
   * disposition is MLN_RENDER_RESULT_RENDERED. It matches the mode field of
   * MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED.
   *
   * A full frame without needs_repaint, whose map_update_generation is at or
   * past the latest_render_update_generation of a map snapshot that reports
   * no transition in progress, shows the map settled, as
   * MLN_RUNTIME_EVENT_MAP_IDLE reports: fully loaded, needing no further
   * frame, and drawing the state of every command that snapshot observes.
   */
  mln_render_mode mode;
} mln_render_frame_result;

/**
 * A borrowed view of one owned frame-result batch.
 *
 * Step through results by result_size. The results pointer remains valid until
 * the frame-batch handle is released. Bindings present the results alone.
 */
typedef struct mln_render_frame_batch_view {
  uint32_t size;
  /**
   * Stride of one result in bytes, at least sizeof(mln_render_frame_result) in
   * the header a caller compiled against. Index results with this value.
   */
  uint32_t result_size;
  /**
   * Terminal frame results in completion order.
   *
   * Borrowed array of result_count results.
   */
  const mln_render_frame_result* results
    MLN_BINDING("length=result_count;stride=result_size");
  /** Number of results in results. */
  size_t result_count;
} mln_render_frame_batch_view MLN_BINDING("kind=slice");

/** Any-thread render-session snapshot. */
typedef struct mln_render_session_snapshot {
  uint32_t size;
  mln_render_session_state state;
  /** Logical extent applied by the driver. */
  mln_logical_extent extent;
  /**
   * Increases when the session's lifecycle state, target, or applied extent
   * changes. An acquired texture frame reports the value it rendered under.
   */
  uint64_t generation;
  /** Generation of the latest map render update that the session has seen. */
  uint64_t map_update_generation;
  /**
   * Map render update generation that the latest rendered frame drew. Zero
   * before the first frame after attachment or a target replacement.
   */
  uint64_t rendered_update_generation;
  /** Frame generation of the latest rendered frame, zero before the first. */
  uint64_t frame_generation;
  /** Acquired frames that the host has not released or discarded. */
  uint32_t acquired_frame_count;
} mln_render_session_snapshot;

/** What abandon did with a session's graphics resources. */
typedef enum mln_render_abandon_disposition : uint32_t {
  /** Abandon destroyed every graphics resource, or none remained. */
  MLN_RENDER_ABANDON_DISPOSITION_CLEAN = 0U,
  /** Abandon kept graphics resources that it could not safely destroy. */
  MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED = 1U,
} mln_render_abandon_disposition;

typedef struct mln_render_abandon_result {
  uint32_t size;
  mln_render_abandon_disposition disposition;
  /** Backend resource groups intentionally retained until process exit. */
  uint32_t quarantined_resource_count;
  uint32_t reserved MLN_BINDING("kind=reserved");
} mln_render_abandon_result;

/**
 * Returns the immutable capabilities fixed during attachment.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, or
 *   out_capabilities is null or undersized.
 * - MLN_STATUS_INVALID_STATE when session has been released.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_render_session_get_capabilities(
  mln_render_session session,
  mln_render_session_capabilities* out_capabilities
    MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies the latest render-session snapshot from any native thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, or
 *   out_snapshot is null or undersized.
 * - MLN_STATUS_INVALID_STATE when session has been released.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING("execution=snapshot")
MLN_API mln_status mln_render_session_get_snapshot(
  mln_render_session session,
  mln_render_session_snapshot* out_snapshot MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Requests a frame without waiting. Every accepted demand produces one terminal
 * result record. Caller-driver sessions require service on the graphics thread.
 * Core-worker sessions need no host service.
 *
 * Returns:
 * - MLN_STATUS_OK when the demand is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, demand is
 *   null or undersized, demand->flags carries a bit outside
 *   mln_frame_demand_flag, or carries MLN_FRAME_DEMAND_WAIT_FOR_UPDATE with
 *   MLN_FRAME_DEMAND_FORCE, or demand->token is
 *   MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached,
 *   or every slot of its texture ring is quarantined.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * A slot is quarantined when its frame is discarded, or when its release fails
 * to wait for consumer synchronization. A demand accepted before the last
 * usable slot was quarantined receives MLN_RENDER_RESULT_TARGET_NOT_READY.
 */
MLN_API mln_status mln_render_session_request_frame(
  mln_render_session session, const mln_frame_demand* demand,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Drains every currently queued terminal frame result.
 *
 * The results move into an independently owned batch, and the function borrows
 * its view. The records remain stable until the batch is released.
 *
 * Returns:
 * - MLN_STATUS_OK when a batch holding at least one result is published in
 *   *out_batch and its view in *out_view.
 * - MLN_STATUS_NOT_READY when no frame result is queued. This is not an error:
 *   *out_batch and *out_view are left unchanged, no batch is allocated, the
 *   diagnostic message is empty, and the caller drains again after the next
 *   frame wake. Bindings return their language's empty form instead of an
 *   error.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, out_batch
 *   is null or does not point to the null handle, or out_view is null or
 *   out_view->size is too small.
 * - MLN_STATUS_INVALID_STATE when session has been released.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING("execution=event_batch")
MLN_API mln_status mln_render_session_drain_frame_results(
  mln_render_session session,
  mln_render_frame_batch* out_batch MLN_BINDING("direction=out"),
  mln_render_frame_batch_view* out_view MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Releases a frame-result batch. */
MLN_API void mln_render_frame_batch_release(
  mln_render_frame_batch batch
) MLN_NOEXCEPT;

/**
 * Acquires the oldest rendered frame that is not already acquired. The frame
 * owns its slot until release. The call is nonblocking.
 *
 * Returns:
 * - MLN_STATUS_OK when a frame is published in *out_frame.
 * - MLN_STATUS_NOT_READY when no rendered frame is available, which includes
 *   while a replacement of a borrowed texture ring is pending. This is not an
 *   error: *out_frame is left unchanged, and the caller retries after the next
 *   demand reports MLN_RENDER_RESULT_RENDERED. Bindings return their
 *   language's empty form instead of an error.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, or out_frame
 *   is null or does not point to the null handle.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached.
 * - MLN_STATUS_UNSUPPORTED when the target does not grant
 *   MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING("absent_on=MLN_STATUS_NOT_READY")
MLN_API mln_status mln_render_session_acquire_frame(
  mln_render_session session,
  mln_acquired_frame* out_frame MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies common metadata for an acquired frame.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is an invalid handle, or out_result
 *   is null or undersized.
 * - MLN_STATUS_INVALID_STATE when frame has been released.
 * - MLN_STATUS_TARGET_LOST when the session lost or abandoned its target.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_acquired_frame_get_result(
  mln_acquired_frame frame,
  mln_render_frame_result* out_result MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies the producer synchronization for an acquired texture frame.
 *
 * The synchronization states when the producer's GPU writes to the frame's
 * texture are complete, for frames of session-owned and borrowed rings alike.
 * The host waits on it before it reads the texture. See mln_gpu_sync_kind for
 * what each kind promises. Attachment selects the producer kind through
 * mln_render_session_attach_options.producer_sync_kind; acquisition never
 * changes it.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is an invalid handle, or out_sync is
 *   null or undersized.
 * - MLN_STATUS_INVALID_STATE when frame has been released.
 * - MLN_STATUS_TARGET_LOST when the session lost or abandoned its target.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING("view_owner=frame")
MLN_API mln_status mln_acquired_frame_get_producer_sync(
  mln_acquired_frame frame, mln_gpu_sync* out_sync MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases an acquired frame after the host's GPU work that reads it.
 *
 * consumer_completion states when that work completes, as mln_gpu_sync_kind
 * describes. Null means MLN_GPU_SYNC_CPU_COMPLETE: the host's GPU reads
 * completed before this call. The call consumes frame on success. A
 * synchronization kind the backend does not support fails with
 * MLN_STATUS_UNSUPPORTED before the handle is consumed, so the caller keeps
 * frame ownership. After abandonment the call closes the handle without
 * graphics work. When the driver fails to wait for the consumer
 * synchronization, the host's GPU may still read the frame's texture, so the
 * frame's slot is quarantined as mln_acquired_frame_discard() quarantines it.
 *
 * Returns:
 * - MLN_STATUS_OK when the frame is consumed and its slot retirement queued.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is an invalid handle or
 *   consumer_completion is undersized, or an OpenGL fence has a zero handle.
 * - MLN_STATUS_INVALID_STATE when frame has been released.
 * - MLN_STATUS_BUSY while a scope from mln_acquired_frame_view_begin() is
 *   active.
 * - MLN_STATUS_UNSUPPORTED when the backend does not support the named
 *   mln_gpu_sync_kind. The handle is not consumed.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_acquired_frame_release(
  mln_acquired_frame frame,
  const mln_gpu_sync* consumer_completion MLN_BINDING("nullable=true"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Begins an allocation-free scope that borrows an acquired frame.
 *
 * Native objects that the frame's getters lend, such as backend textures and
 * devices, stay valid until the scope ends, even when another thread discards
 * the frame or disposes its session. The scope retains the frame and its
 * session. Discard and disposal reject later scopes immediately and wait for
 * active scopes to end before they retire graphics resources. Explicit frame
 * release and session abandonment report MLN_STATUS_BUSY while a scope is
 * active. Each successful call requires exactly one
 * mln_acquired_frame_view_end() call, on any thread.
 *
 * Returns:
 * - MLN_STATUS_OK when *out_scope receives the scope.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is an invalid handle or out_scope is
 *   null.
 * - MLN_STATUS_INVALID_STATE when frame has been released or discarded.
 * - MLN_STATUS_TARGET_LOST when the session lost or abandoned its target, or
 *   disposal of the session has begun.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_acquired_frame_view_begin(
  mln_acquired_frame frame,
  void** out_scope MLN_BINDING("direction=out;kind=context"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Ends one scope from mln_acquired_frame_view_begin(). Null has no effect. */
MLN_API void mln_acquired_frame_view_end(
  void* scope MLN_BINDING("kind=context")
) MLN_NOEXCEPT;

/**
 * Starts an ordered logical resize of the driver and the map. The driver
 * applies the extent in order with its other work, and later driver work does
 * not wait for the map. The completion runs after both have applied it and the
 * map has published its snapshot.
 * Frames render once a map update matches the applied extent; static and
 * tile maps publish that update for their next still image.
 *
 * scale_factor must equal the session's current value, set at attachment or by
 * the latest target replacement. To change it, replace a surface or borrowed
 * texture through the backend's set_target function, or detach and attach
 * again. The map keeps the scale factor it was created with and takes only the
 * new width and height.
 *
 * Returns:
 * - MLN_STATUS_OK when the resize is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle; completion
 *   is undersized or sets both or neither of callback and queue; extent has a
 *   zero width or height, or a scale_factor that is not finite and positive; or
 *   extent.scale_factor differs from the session's current value.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached,
 *   or a texture frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when the target is a caller-owned texture, which its
 *   owner sizes through the backend's set_target function.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK and MLN_COMMAND_DISPOSITION_COMMITTED once the map and driver
 *   apply the new extent and the map publishes its snapshot.
 * - MLN_STATUS_INVALID_STATE when the session starts detaching before the
 *   driver applies the extent.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_render_session_resize(
  mln_render_session session, mln_logical_extent extent,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Ends every frame demand that has not started with MLN_RENDER_RESULT_NO_UPDATE
 * and completes after earlier driver work has finished. A running demand
 * finishes its attempt and ends any wait for a map update or a free slot.
 * After completion, a new frame demand is required to render again, except
 * that a pending still image submits its next demand itself; see
 * mln_map_request_still_image().
 *
 * Returns:
 * - MLN_STATUS_OK when the barrier is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, or
 *   completion is null or undersized.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK once earlier driver work has finished.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_barrier(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts best-effort release of renderer caches.
 *
 * Returns:
 * - MLN_STATUS_OK when the submission is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, or
 *   completion is null or undersized.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver ran the release.
 * - MLN_STATUS_INVALID_STATE when the session detached first.
 * - MLN_STATUS_TARGET_LOST when the session was abandoned first.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_reduce_memory_use(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts asynchronous renderer-data clearing.
 *
 * Returns:
 * - MLN_STATUS_OK when the submission is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, or
 *   completion is null or undersized.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver cleared the data.
 * - MLN_STATUS_INVALID_STATE when the session detached first.
 * - MLN_STATUS_TARGET_LOST when the session was abandoned first.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_clear_data(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts asynchronous renderer diagnostic-log emission.
 *
 * Returns:
 * - MLN_STATUS_OK when the submission is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, or
 *   completion is null or undersized.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver emitted the logs.
 * - MLN_STATUS_INVALID_STATE when the session detached first.
 * - MLN_STATUS_TARGET_LOST when the session was abandoned first.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_render_session_dump_debug_logs(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Services every queued item for a caller-graphics-thread driver. The first
 * successful service call fixes the session's graphics-thread identity; later
 * calls from another native thread return MLN_STATUS_WRONG_THREAD. The target
 * context must be current. Core-worker sessions return
 * MLN_STATUS_INVALID_STATE.
 *
 * A driver result can finish on a later service, once the graphics thread's
 * event loop delivers what the result waits for: a browser WebGPU readback
 * waits for its copy to map. The session's later driver work waits behind that
 * result, so a service runs none of it until the event loop delivers the
 * result and the driver-work wake fires. A browser host returns the graphics
 * thread to its event loop between service calls, or waits for the wake in a
 * way that runs that loop.
 *
 * A WebGPU texture render likewise returns before the GPU finishes its frame.
 * Once the GPU has as many unfinished frames as the texture ring has slots, a
 * service renders no frame, nor runs the work behind it, until the event loop
 * reports them finished. The driver-work wake fires for such a frame, since
 * the service it prompts asks for that report, and fires again once the report
 * arrives.
 *
 * Returns:
 * - MLN_STATUS_OK once queued driver work has been serviced.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle.
 * - MLN_STATUS_INVALID_STATE when session has been released or is driven by its
 *   own core worker.
 * - MLN_STATUS_TARGET_LOST after abandonment or dispose of the session.
 * - MLN_STATUS_WRONG_THREAD when another native thread already fixed the
 *   session's graphics-thread identity.
 * - MLN_STATUS_BUSY when a driver call is already in flight.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_render_session_service_driver_work(
  mln_render_session session, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts normal graphics-owner teardown and map detachment.
 *
 * Outstanding acquired frames fail preflight with MLN_STATUS_INVALID_STATE and
 * leave the session attached. Once accepted, earlier driver work reaches a
 * terminal result before graphics resources are destroyed.
 *
 * When a discarded frame or a failed release quarantined a slot of a
 * session-owned texture ring, the host's GPU may still read that slot's
 * texture. Detach then keeps the ring and its graphics context until the
 * process exits, and destroys the rest. When it keeps a ring, detach waits for
 * the map's in-flight tile work, and once detach completes the host may
 * destroy its graphics objects. The one exception is a kept Vulkan ring: it is
 * a child of the host's VkDevice, so the host keeps the device until the
 * process exits. A borrowed ring's textures belong to the host, so detach
 * keeps nothing for its quarantined slots.
 *
 * Returns:
 * - MLN_STATUS_OK when the detach is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, or
 *   completion is null or undersized.
 * - MLN_STATUS_INVALID_STATE when session has been released or is not attached,
 *   or a frame is still acquired.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 *
 * Demands still outstanding receive MLN_RENDER_RESULT_TARGET_NOT_READY.
 *
 * Completes with:
 * - MLN_STATUS_OK once the target is released.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_render_session_detach(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Irreversibly ends session work and destroys the session's graphics objects
 * where that is safe.
 *
 * For a core-worker session, the call waits for in-flight driver work to end
 * and prevents later graphics calls. For a caller-graphics-thread session,
 * abandon during in-flight driver work returns MLN_STATUS_BUSY. Calling from
 * inside any of the session's driver calls, including a completion, also
 * returns MLN_STATUS_BUSY.
 *
 * Before returning, the call waits for the map's in-flight tile work and then
 * disposes of the session's graphics objects. It destroys Vulkan and Metal
 * objects on the calling thread. For a Vulkan session it first waits for the
 * work the session submitted to its queue, taking the host's queue lock only
 * to submit that wait, never across it. It keeps an object, until the process
 * exits, when destroying it is not safe: the texture ring of a session-owned
 * target while a frame of it is acquired or a slot is quarantined; every
 * object when the process has begun to exit; and OpenGL and WebGPU objects,
 * which only their graphics thread may destroy. *out_result reports what was
 * kept. Release acquired frames before abandon to let it destroy a
 * session-owned ring.
 *
 * The host keeps the target's graphics objects valid until the call returns,
 * and must not hold the queue lock while it waits on the abandoning thread.
 * After the call returns, no library thread touches them and the host may
 * destroy them. One exception is a kept Vulkan object: it is a child of the
 * host's VkDevice, and a kept swapchain is also a child of its VkSurfaceKHR,
 * so the host keeps those parents until the process exits. The other
 * exception is a browser WebGPU readback whose copy is still mapping: the
 * library finishes it when the graphics thread's event loop delivers the map,
 * and the host may still release its objects. A Vulkan GPU
 * that hangs without reporting device loss blocks the call. Do not call from a
 * MapLibre worker callback.
 *
 * Returns:
 * - MLN_STATUS_OK when control is abandoned and *out_result describes what was
 *   kept.
 * - MLN_STATUS_BUSY when a scope from mln_acquired_frame_view_begin() is
 *   active, a caller-driver call is in flight, or the caller is inside one of
 *   the session's driver calls. Nothing changes.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, or
 *   out_result is null or undersized.
 * - MLN_STATUS_INVALID_STATE when session has been released, or the session
 *   already released its target.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_render_session_abandon(
  mln_render_session session,
  mln_render_abandon_result* out_result MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Consumes a session in any state and retires it, then runs completion, if
 * given, once the host may destroy the target's graphics objects.
 *
 * A null completion disposes without reporting retirement, as a command
 * discards its outcome. The call is CPU-only and may run on any native thread,
 * including from the session's own completions and driver calls.
 *
 * Queued driver work completes with MLN_STATUS_TARGET_LOST. An attached
 * core-worker session with no acquired frame detaches after in-flight driver
 * work ends, following mln_render_session_detach(). Every other session that
 * holds its target is abandoned after in-flight driver work and scopes from
 * mln_acquired_frame_view_begin() end, following mln_render_session_abandon().
 * A session whose target an earlier detach or abandon released follows the
 * rules of that call, and retires once an abandon still running on another
 * thread returns. Acquired frame accessors report target loss after
 * acceptance; their owners still release or discard those frames.
 *
 * Retirement releases the map attachment once no library thread touches the
 * target's graphics objects. mln_map_release() accepts the map before that and
 * completes after it, and the session's wake release callbacks and this
 * completion also run after it, so the host may destroy its graphics objects
 * once any of them runs. Disposal keeps what that detach or abandon keeps and
 * reports nothing about it. A Vulkan session keeps its session-owned texture
 * ring while a frame of it is acquired or a slot is quarantined, and every
 * object once the process has begun to exit or its queue fails to drain; the
 * host then keeps their VkDevice, a kept swapchain's VkSurfaceKHR, and their
 * VkInstance until the process exits. Release acquired frames, rather than
 * discard them, before disposing to let a Vulkan host destroy its device.
 * Teardown may allocate; allocation failure after acceptance terminates the
 * process.
 *
 * Returns:
 * - MLN_STATUS_OK when the disposal is accepted and the handle is consumed.
 * - MLN_STATUS_INVALID_ARGUMENT when session is an invalid handle, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when session has been released.
 * - MLN_STATUS_NATIVE_ERROR when the completion could not be allocated.
 *
 * Completes with MLN_STATUS_OK once the session has retired.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_render_session_dispose(
  mln_render_session session, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Consumes an acquired frame without consumer GPU synchronization, so it
 * quarantines the frame's slot of the texture ring.
 *
 * Discard cleans up a frame whose consumer synchronization is unknown, such as
 * one its owner dropped without releasing it. The host's GPU may still read
 * the frame's texture, so the session keeps that texture and never renders
 * into its slot again, unless a replacement of a borrowed ring gives the slot
 * a new texture. Rendering continues on the other slots, and a view already
 * open on the frame stays valid until it ends. Once every slot is quarantined,
 * frame requests return MLN_STATUS_INVALID_STATE. Detach of a session with a
 * quarantined slot of a session-owned ring keeps the ring allocated until the
 * process exits; see mln_render_session_detach(). A release returns the slot
 * to the ring instead.
 *
 * The call is CPU-only and may run on any native thread. It starts no thread.
 * It allocates only when it quarantines the last slot while a frame demand
 * waits for one, to give that demand its terminal result.
 *
 * Returns MLN_STATUS_OK on acceptance, MLN_STATUS_INVALID_ARGUMENT for an
 * invalid handle, or MLN_STATUS_INVALID_STATE for a frame that has been
 * released.
 */
MLN_API mln_status mln_acquired_frame_discard(
  mln_acquired_frame frame, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_RENDER_SESSION_H
