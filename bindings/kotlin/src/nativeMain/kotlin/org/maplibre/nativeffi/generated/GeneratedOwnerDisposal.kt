// Generated from handle disposal relationships. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.cinterop.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.status.Status

@OptIn(ExperimentalForeignApi::class)
internal actual object GeneratedOwnerDisposal {
  actual fun acquiredFrame(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_acquired_frame_dispose",
    )
    Status.check(mln_acquired_frame_dispose(handle.toULong()))
  }

  actual fun buffer(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(handle, "mln_buffer_destroy")
    mln_buffer_destroy(handle.toULong())
  }

  actual fun eventBatch(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_event_batch_release",
    )
    mln_event_batch_release(handle.toULong())
  }

  actual fun geojsonSourceData(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_geojson_source_data_destroy",
    )
    mln_geojson_source_data_destroy(handle.toULong())
  }

  actual fun map(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(handle, "mln_map_dispose")
    Status.check(mln_map_dispose(handle.toULong()))
  }

  actual fun mapProjection(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_map_projection_close",
    )
    Status.check(mln_map_projection_close(handle.toULong()))
  }

  actual fun renderFrameBatch(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_render_frame_batch_release",
    )
    mln_render_frame_batch_release(handle.toULong())
  }

  actual fun renderSession(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_render_session_dispose",
    )
    Status.check(mln_render_session_dispose(handle.toULong()))
  }

  actual fun resourceRequestHandle(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_resource_request_release",
    )
    mln_resource_request_release(handle.toULong())
  }

  actual fun runtime(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(handle, "mln_runtime_dispose")
    Status.check(mln_runtime_dispose(handle.toULong()))
  }
}
