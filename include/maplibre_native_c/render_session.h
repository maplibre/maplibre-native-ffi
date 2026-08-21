/**
 * @file maplibre_native_c/render_session.h
 * Public C API declarations for render sessions.
 */

#ifndef MAPLIBRE_NATIVE_C_RENDER_SESSION_H
#define MAPLIBRE_NATIVE_C_RENDER_SESSION_H

#include <stdbool.h>
#include <stdint.h>

#include "base.h"
#include "map.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Resizes an attached render session.
 *
 * Width and height are logical map dimensions. The scale_factor value maps
 * them to physical backend pixels. Resizing sets the map size, so the map
 * viewport and the render target extent stay the same value.
 *
 * Surface and session-owned texture sessions resize in place. Caller-owned
 * borrowed texture targets return MLN_STATUS_UNSUPPORTED because the texture is
 * sized by its owner; hand a replacement over with the
 * mln_*_borrowed_texture_set_target() function for the backend. See texture.h.
 *
 * The session renderer survives a resize, carrying the tile pyramid, glyph and
 * image atlases, symbol placement, and feature state set through
 * mln_render_session_set_feature_state() across to the new size. A scale_factor
 * that differs from the session's current value retires the renderer instead,
 * because its shaders are compiled for a fixed pixel ratio, and renderer-held
 * state starts empty on the next mln_render_session_render_update(). Map state
 * such as camera, style, and sources survives either way.
 *
 * Passing a scale_factor that differs from the map's mln_map_options
 * scale_factor logs a warning; see mln_map_options.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, dimensions
 *   are zero, scale_factor is non-positive or non-finite, or scaled dimensions
 *   are too large.
 * - MLN_STATUS_INVALID_STATE when the session is detached or a texture frame is
 *   currently acquired.
 * - MLN_STATUS_UNSUPPORTED when resizing is not supported by the session kind
 *   or mode, such as a caller-owned borrowed texture target.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_resize(
  mln_render_session session, uint32_t width, uint32_t height,
  double scale_factor
) MLN_NOEXCEPT;

/** Outcome of a successful mln_render_session_render_update() call. */
typedef enum mln_render_result : uint32_t {
  /** The call rendered a frame into the render target. */
  MLN_RENDER_RESULT_RENDERED = 0,
  /** The call produced no frame. */
  MLN_RENDER_RESULT_NO_UPDATE,
  /** The map has not applied the session's current size yet. */
  MLN_RENDER_RESULT_SIZE_PENDING,
  /** The render target had no frame to draw into. */
  MLN_RENDER_RESULT_TARGET_NOT_READY,
} mln_render_result;

/**
 * Renders the map's latest render update into the session's render target.
 *
 * A surface session presents the frame. A texture session writes it into the
 * target texture.
 *
 * *out_result reports which of these outcomes the call reached, and each one
 * names the wake that a host waits for before it calls again:
 *
 * - MLN_RENDER_RESULT_RENDERED means the target holds a new frame. The map
 *   retains its latest update, so a host redraws on demand after a resize or a
 *   surface expose and gates a frame loop on
 *   MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE.
 * - MLN_RENDER_RESULT_NO_UPDATE means the call produced no frame. The map
 *   either has no update yet, or the Metal backend has not created an owned
 *   texture because content is not ready. Wait for
 *   MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE.
 * - MLN_RENDER_RESULT_SIZE_PENDING means the session resized and the map,
 *   which applies its size on its own thread, is still behind. The map
 *   publishes an update for the new size on its own, so wait for the next
 *   MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE.
 * - MLN_RENDER_RESULT_TARGET_NOT_READY means the render target had no frame
 *   available, such as a Metal surface whose next drawable is nil. No map
 *   update resolves this, so wait for a host event that changes the target,
 *   or back off and retry.
 *
 * In MLN_MAP_MODE_STATIC, pump a resize through the map before requesting the
 * still image. The session applies its extent on the map's owner thread, and a
 * still image requested before that lands reports
 * MLN_RENDER_RESULT_SIZE_PENDING.
 *
 * *out_needs_repaint reports whether the map asked for another frame while it
 * rendered this one, as during an ongoing camera transition. It is set only
 * when *out_result is MLN_RENDER_RESULT_RENDERED, and reads false for every
 * other outcome. This is the same signal that
 * MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED carries in its needs_repaint
 * field, delivered here without the event round trip, so a host can re-arm its
 * frame loop before it drains events.
 *
 * Returns:
 * - MLN_STATUS_OK on success, with *out_result and *out_needs_repaint set.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, or
 *   out_result or out_needs_repaint is null.
 * - MLN_STATUS_INVALID_STATE when the session is detached or a texture frame
 *   is currently acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when the render backend reports no renderer
 *   backend, or when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_render_update(
  mln_render_session session, mln_render_result* out_result,
  bool* out_needs_repaint
) MLN_NOEXCEPT;

/**
 * Detaches backend-bound render resources from the map while keeping the
 * session handle live for destruction.
 *
 * After detach, resize, render, readback, acquire, and renderer maintenance
 * operations return MLN_STATUS_INVALID_STATE.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live.
 * - MLN_STATUS_INVALID_STATE when already detached or a texture frame is
 *   acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status
mln_render_session_detach(mln_render_session session) MLN_NOEXCEPT;

/**
 * Destroys a render session handle.
 *
 * If the session is still attached, this function detaches it first.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live.
 * - MLN_STATUS_INVALID_STATE when a texture frame is acquired.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status
mln_render_session_destroy(mln_render_session session) MLN_NOEXCEPT;

/**
 * Asks the session renderer to release cached resources where possible.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live.
 * - MLN_STATUS_INVALID_STATE when the session is detached or no renderer has
 *   been created for the session yet.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when the render backend reports no renderer
 *   backend, or when an internal exception is converted to status.
 */
MLN_API mln_status
mln_render_session_reduce_memory_use(mln_render_session session) MLN_NOEXCEPT;

/**
 * Clears renderer data for the session.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live.
 * - MLN_STATUS_INVALID_STATE when the session is detached or no renderer has
 *   been created for the session yet.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when the render backend reports no renderer
 *   backend, or when an internal exception is converted to status.
 */
MLN_API mln_status
mln_render_session_clear_data(mln_render_session session) MLN_NOEXCEPT;

/**
 * Dumps renderer debug logs for the session through MapLibre Native logging.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live.
 * - MLN_STATUS_INVALID_STATE when the session is detached or no renderer has
 *   been created for the session yet.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when the render backend reports no renderer
 *   backend, or when an internal exception is converted to status.
 */
MLN_API mln_status
mln_render_session_dump_debug_logs(mln_render_session session) MLN_NOEXCEPT;

/**
 * Sets per-feature state on a render source for this render session.
 *
 * selector->source_id and selector->feature_id are borrowed for the duration of
 * the call. state must contain one UTF-8 JSON object and is parsed before
 * return. The accepted command requests a map repaint.
 *
 * A call before that source exists on a render update is stored and applied
 * before the first presented frame that includes the source.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, selector is
 *   null or invalid, selector lacks MLN_FEATURE_STATE_SELECTOR_FEATURE_ID,
 *   state is empty, invalid JSON, or not an object.
 * - MLN_STATUS_INVALID_STATE when the session is detached.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when the render backend reports no renderer
 *   backend, or when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_set_feature_state(
  mln_render_session session, const mln_feature_state_selector* selector,
  mln_buffer_view state
) MLN_NOEXCEPT;

/**
 * Copies per-feature state from a render source in this render session.
 *
 * selector->source_id and selector->feature_id are borrowed for the duration of
 * the call. On success, *out_state receives an owned buffer containing a UTF-8
 * JSON object. Destroy it with mln_buffer_destroy(). Missing native source or
 * feature state is reported as an empty object. A call before that source
 * exists on a render update copies any state stored for that feature, or an
 * empty object when none has been set.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, selector is
 *   null or invalid, selector lacks MLN_FEATURE_STATE_SELECTOR_FEATURE_ID,
 *   out_state is null, or *out_state is not null.
 * - MLN_STATUS_INVALID_STATE when the session is detached.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when the render backend reports no renderer
 *   backend, or when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_get_feature_state(
  mln_render_session session, const mln_feature_state_selector* selector,
  mln_buffer* out_state
) MLN_NOEXCEPT;

/**
 * Removes per-feature state from a render source in this render session.
 *
 * selector->source_id is required. selector->feature_id and selector->state_key
 * are optional. Passing both removes one state key from one feature. Passing
 * only feature_id removes all state for that feature. Passing neither removes
 * all feature state for the source/source-layer. The accepted command requests
 * a map repaint.
 *
 * A call before that source exists on a render update is stored and applied
 * before the first presented frame that includes the source.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when session is null or not live, selector is
 *   null or invalid, or selector has MLN_FEATURE_STATE_SELECTOR_STATE_KEY
 *   without MLN_FEATURE_STATE_SELECTOR_FEATURE_ID.
 * - MLN_STATUS_INVALID_STATE when the session is detached.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the session
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when the render backend reports no renderer
 *   backend, or when an internal exception is converted to status.
 */
MLN_API mln_status mln_render_session_remove_feature_state(
  mln_render_session session, const mln_feature_state_selector* selector
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_RENDER_SESSION_H
