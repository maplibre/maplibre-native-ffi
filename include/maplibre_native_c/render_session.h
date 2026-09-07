/**
 * @file maplibre_native_c/render_session.h
 * Public C API declarations for asynchronous render sessions.
 */

#ifndef MAPLIBRE_NATIVE_C_RENDER_SESSION_H
#define MAPLIBRE_NATIVE_C_RENDER_SESSION_H

#include <stdbool.h>
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
  /** A frame was rendered for acquisition, presentation, or ordered readback.
   */
  MLN_RENDER_RESULT_RENDERED = 0,
  /** No newer map update was available. */
  MLN_RENDER_RESULT_NO_UPDATE = 1,
  /** An ordered extent change had not reached the driver. */
  MLN_RENDER_RESULT_SIZE_PENDING = 2,
  /** The target could not produce a frame. */
  MLN_RENDER_RESULT_TARGET_NOT_READY = 3,
  /** A newer demand in the same coalescing boundary replaced this demand. */
  MLN_RENDER_RESULT_SUPERSEDED = 4,
  /** The demand's timeout elapsed before driver work began. */
  MLN_RENDER_RESULT_DEADLINE_MISSED = 5,
} mln_render_result;

/** Render-session lifecycle visible in snapshots. */
typedef enum mln_render_session_state : uint32_t {
  MLN_RENDER_SESSION_STATE_ATTACHING = 1U,
  MLN_RENDER_SESSION_STATE_ATTACHED = 2U,
  MLN_RENDER_SESSION_STATE_DETACHING = 3U,
  MLN_RENDER_SESSION_STATE_DETACHED = 4U,
  MLN_RENDER_SESSION_STATE_TARGET_LOST = 5U,
  MLN_RENDER_SESSION_STATE_ABANDONED = 6U,
} mln_render_session_state;

/** Frame-demand policy bits. */
typedef enum mln_frame_demand_flag : uint32_t {
  /** Render only when a newer map update exists. */
  MLN_FRAME_DEMAND_IF_NEEDED = 1U << 0U,
  /**
   * Present the rendered frame on a target that supports presentation. A
   * presenting target whose demand clears this bit still renders and keeps
   * whatever it presented last. Ignored by targets without presentation.
   */
  MLN_FRAME_DEMAND_PRESENT = 1U << 1U,
} mln_frame_demand_flag;

/** One nonblocking request for a frame. */
typedef struct mln_frame_demand {
  uint32_t size;
  /** A bitwise OR of mln_frame_demand_flag values. */
  uint32_t flags;
  /** Host identity returned with the terminal frame result. */
  uint64_t token;
  /** Demands coalesce only when this value and their flags match. */
  uint64_t coalescing_boundary;
  /** Positive time allowed before driver work begins, in nanoseconds; zero has
   * no limit. */
  uint64_t timeout_ns;
} mln_frame_demand;

/** Immutable result record copied into an owned frame-result batch. */
typedef struct mln_render_frame_result {
  uint32_t size;
  /** One mln_render_result value. */
  uint32_t disposition;
  uint64_t token;
  uint64_t map_update_generation;
  uint64_t extent_generation;
  /** Zero unless disposition is MLN_RENDER_RESULT_RENDERED. */
  uint64_t frame_generation;
  /**
   * Whether the map asked for another frame while it rendered this one, as
   * during an ongoing camera transition. Set only when disposition is
   * MLN_RENDER_RESULT_RENDERED, and false for every other outcome. This is the
   * same signal that MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED carries in
   * its needs_repaint field, delivered with the frame result so a host can
   * re-arm its frame loop without the runtime event round trip.
   */
  bool needs_repaint;
} mln_render_frame_result;

/** Any-thread render-session snapshot. */
typedef struct mln_render_session_snapshot {
  uint32_t size;
  /** One mln_render_session_state value. */
  uint32_t state;
  /** One mln_render_driver_kind value. */
  uint32_t driver;
  /** Most recent terminal mln_render_result value. */
  uint32_t latest_result;
  mln_render_target_extent extent;
  uint64_t generation;
  uint64_t map_update_generation;
  uint64_t rendered_update_generation;
  uint64_t extent_generation;
  uint64_t frame_generation;
  uint64_t latest_demand_token;
  uint32_t pending_demand_count;
  uint32_t acquired_frame_count;
  bool target_ready;
  bool pending_changes;
} mln_render_session_snapshot;

/** Result of irreversible CPU-side target abandonment. */
typedef enum mln_render_abandon_disposition : uint32_t {
  /** No graphics resources remained when control was abandoned. */
  MLN_RENDER_ABANDON_DISPOSITION_CLEAN = 0U,
  /** Graphics resources could not be destroyed and were quarantined. */
  MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED = 1U,
} mln_render_abandon_disposition;

typedef struct mln_render_abandon_result {
  uint32_t size;
  /** One mln_render_abandon_disposition value. */
  uint32_t disposition;
  /** Backend resource groups intentionally retained until process exit. */
  uint32_t quarantined_resource_count;
  uint32_t reserved;
} mln_render_abandon_result;

/** Returns a zero-token, render-if-needed, nonpresenting frame demand. */
MLN_API mln_frame_demand mln_frame_demand_default(void) MLN_NOEXCEPT;

/**
 * Returns the immutable capabilities fixed during attachment.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live, or out_capabilities
 *   is null or undersized.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_get_capabilities(
  mln_render_session session, mln_render_session_capabilities* out_capabilities
) MLN_NOEXCEPT;

/**
 * Copies the latest render-session snapshot from any native thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live, or out_snapshot is
 *   null or undersized.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_get_snapshot(
  mln_render_session session, mln_render_session_snapshot* out_snapshot
) MLN_NOEXCEPT;

/**
 * Requests a frame without waiting. Every accepted demand produces one terminal
 * result record. A core worker wakes itself; a caller driver publishes its
 * driver-work endpoint.
 *
 * A demand that does not carry MLN_FRAME_DEMAND_PRESENT still renders; a
 * presenting target keeps whatever it presented last.
 *
 * Returns:
 * - MLN_STATUS_OK when the demand is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live, demand is null or
 *   undersized, or demand->flags carries a bit outside mln_frame_demand_flag.
 * - MLN_STATUS_INVALID_STATE when the session is not attached.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_request_frame(
  mln_render_session session, const mln_frame_demand* demand
) MLN_NOEXCEPT;

/**
 * Drains every currently queued terminal frame result into an independently
 * owned batch. The records remain stable until the batch is released.
 *
 * Returns:
 * - MLN_STATUS_OK when a batch is published in *out_batch.
 * - MLN_STATUS_NOT_READY when no frame result is queued. This is not an error:
 *   *out_batch is left unchanged, no batch is allocated, and the caller retries
 *   after the next demand.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live, or out_batch is null
 *   or does not point to the null handle.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_drain_frame_results(
  mln_render_session session, mln_render_frame_batch* out_batch
) MLN_NOEXCEPT;

/**
 * Returns the number of records in an owned frame-result batch.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when batch is not live, or out_count is null.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_frame_batch_count(
  mln_render_frame_batch batch, size_t* out_count
) MLN_NOEXCEPT;

/**
 * Copies one frame-result record.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when batch is not live, index is out of range,
 *   or out_result is null or undersized.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_frame_batch_get(
  mln_render_frame_batch batch, size_t index,
  mln_render_frame_result* out_result
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
 * - MLN_STATUS_NOT_READY when no rendered frame is available. This is not an
 *   error: *out_frame is left unchanged, and the caller retries after the next
 *   demand reports MLN_RENDER_RESULT_RENDERED.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live, or out_frame is null
 *   or does not point to the null handle.
 * - MLN_STATUS_INVALID_STATE when the session is not attached.
 * - MLN_STATUS_UNSUPPORTED when the target does not grant
 *   MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_acquire_frame(
  mln_render_session session, mln_acquired_frame* out_frame
) MLN_NOEXCEPT;

/**
 * Copies common metadata for an acquired frame.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is not live or already released, or
 *   out_result is null or undersized.
 * - MLN_STATUS_TARGET_LOST when the session lost or abandoned its target.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_acquired_frame_get_result(
  mln_acquired_frame frame, mln_render_frame_result* out_result
) MLN_NOEXCEPT;

/**
 * Copies the producer synchronization for an acquired texture frame.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is not live or already released, or
 *   out_sync is null or undersized.
 * - MLN_STATUS_TARGET_LOST when the session lost or abandoned its target.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_acquired_frame_get_producer_sync(
  mln_acquired_frame frame, mln_gpu_sync* out_sync
) MLN_NOEXCEPT;

/**
 * Releases an acquired frame after optional consumer GPU work.
 *
 * The call consumes *frame on success and sets it to MLN_HANDLE_NULL. The
 * session retires the ring slot through its selected driver before reusing it.
 * A synchronization kind the backend does not support fails with
 * MLN_STATUS_UNSUPPORTED before the handle is consumed, so the caller keeps
 * frame ownership. After abandonment the call closes the handle without
 * graphics work.
 *
 * Returns:
 * - MLN_STATUS_OK when the frame is consumed and its slot retirement queued.
 * - MLN_STATUS_INVALID_ARGUMENT when frame is null, points at the null handle
 *   or a handle that is not live, or consumer_completion is undersized.
 * - MLN_STATUS_INVALID_STATE when the frame was already released.
 * - MLN_STATUS_UNSUPPORTED when the backend does not support the named
 *   mln_gpu_sync_kind. The handle is not consumed.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_acquired_frame_release(
  mln_acquired_frame* frame, const mln_gpu_sync* consumer_completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered logical resize. The completion runs after the selected
 * driver applies the extent and updates the map viewport.
 *
 * scale_factor is fixed when the session attaches, because the renderer bakes
 * its pixel ratio into compiled shaders. An extent that changes it is rejected;
 * destroy the session and attach again instead.
 *
 * Returns:
 * - MLN_STATUS_OK when the resize is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live; extent or completion
 *   is null or undersized; the extent is not positive; or its scale_factor
 *   differs from the one the session attached with.
 * - MLN_STATUS_INVALID_STATE when the session is not attached, or a texture
 *   frame is still acquired.
 * - MLN_STATUS_UNSUPPORTED when the target is a caller-owned texture, which its
 *   owner sizes through the backend's set_target function.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK and MLN_COMMAND_DISPOSITION_COMMITTED once the driver renders
 *   at the new extent.
 * - MLN_STATUS_OK and MLN_COMMAND_DISPOSITION_SUPERSEDED when a later resize
 *   replaced this one.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_render_session_resize(
  mln_render_session session, const mln_render_target_extent* extent,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts a barrier that completes after all render work accepted before it has
 * a terminal result. A barrier does not request a frame.
 *
 * Returns:
 * - MLN_STATUS_OK when the barrier is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live, or completion is null
 *   or undersized.
 * - MLN_STATUS_INVALID_STATE when the session is not attached.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once every earlier demand and ordered operation has a
 *   terminal result.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_render_session_barrier(
  mln_render_session session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts best-effort release of renderer caches.
 *
 * Returns:
 * - MLN_STATUS_OK when the submission is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live, or completion is null
 *   or undersized.
 * - MLN_STATUS_INVALID_STATE when the session is not attached.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver ran the release.
 * - MLN_STATUS_INVALID_STATE when the session detached first.
 * - MLN_STATUS_TARGET_LOST when the session was abandoned first.
 */
MLN_API mln_status mln_render_session_reduce_memory_use(
  mln_render_session session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts asynchronous renderer-data clearing.
 *
 * Returns:
 * - MLN_STATUS_OK when the submission is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live, or completion is null
 *   or undersized.
 * - MLN_STATUS_INVALID_STATE when the session is not attached.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver cleared the data.
 * - MLN_STATUS_INVALID_STATE when the session detached first.
 * - MLN_STATUS_TARGET_LOST when the session was abandoned first.
 */
MLN_API mln_status mln_render_session_clear_data(
  mln_render_session session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts asynchronous renderer diagnostic-log emission.
 *
 * Returns:
 * - MLN_STATUS_OK when the submission is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live, or completion is null
 *   or undersized.
 * - MLN_STATUS_INVALID_STATE when the session is not attached.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_OK once the driver emitted the logs.
 * - MLN_STATUS_INVALID_STATE when the session detached first.
 * - MLN_STATUS_TARGET_LOST when the session was abandoned first.
 */
MLN_API mln_status mln_render_session_dump_debug_logs(
  mln_render_session session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Services up to max_work items for a caller-graphics-thread driver; zero
 * services every item currently queued. The first successful service call
 * fixes the session's graphics-thread identity; later calls from another native
 * thread return MLN_STATUS_WRONG_THREAD. The target context must be current.
 * Core-worker sessions return MLN_STATUS_INVALID_STATE.
 *
 * Returns:
 * - MLN_STATUS_OK when the serviced items are counted in *out_serviced, which
 *   may be zero.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live, or out_serviced is
 *   null.
 * - MLN_STATUS_INVALID_STATE when the session is driven by its own core worker.
 * - MLN_STATUS_WRONG_THREAD when another native thread already fixed the
 *   session's graphics-thread identity.
 * - MLN_STATUS_BUSY when a driver call is already in flight.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_service_driver_work(
  mln_render_session session, size_t max_work, size_t* out_serviced
) MLN_NOEXCEPT;

/**
 * Starts normal graphics-owner teardown and map detachment.
 *
 * Outstanding acquired frames fail preflight with MLN_STATUS_INVALID_STATE and
 * leave the session attached. Once accepted, earlier mailbox operations reach
 * a terminal result before graphics resources are destroyed.
 *
 * Returns:
 * - MLN_STATUS_OK when the detach is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live, or completion is null
 *   or undersized.
 * - MLN_STATUS_INVALID_STATE when the session is not attached, or a frame is
 *   still acquired.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Demands still outstanding receive MLN_RENDER_RESULT_TARGET_NOT_READY.
 *
 * Completes with:
 * - MLN_STATUS_OK once the target is released.
 * - MLN_STATUS_TARGET_LOST when the session is abandoned first.
 */
MLN_API mln_status mln_render_session_detach(
  mln_render_session session, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Irreversibly closes control and mailboxes without graphics calls. The call
 * returns MLN_STATUS_BUSY while a driver call is in flight.
 *
 * Before returning, the call waits for the map's in-flight tile work, which
 * can still reference quarantined renderer resources and through them the
 * host's graphics objects. After it returns, no library thread touches the
 * session's target or device, so the host may destroy them immediately. Do
 * not call from a MapLibre worker callback.
 *
 * Returns:
 * - MLN_STATUS_OK when control is abandoned and *out_result describes what was
 *   quarantined.
 * - MLN_STATUS_BUSY when a driver call is in flight. Nothing changes.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live, or out_result is null
 *   or undersized.
 * - MLN_STATUS_INVALID_STATE when the session already released its target.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_abandon(
  mln_render_session session, mln_render_abandon_result* out_result
) MLN_NOEXCEPT;

/**
 * Retires a detached or abandoned session handle. The call is CPU-only and may
 * run on any native thread, including from one of the session's own
 * completions.
 *
 * Returns:
 * - MLN_STATUS_OK when the handle is retired.
 * - MLN_STATUS_INVALID_ARGUMENT when session is not live.
 * - MLN_STATUS_INVALID_STATE when the session is neither detached nor
 *   abandoned, or a detached session still has an acquired frame.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status
mln_render_session_destroy(mln_render_session session) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_RENDER_SESSION_H
