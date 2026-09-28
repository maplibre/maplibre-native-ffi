// Generated from handle disposal relationships. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.status.Status

internal actual object GeneratedOwnerDisposal {
  actual fun acquiredFrame(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_acquired_frame_dispose",
    )
    Status.check(MapLibreNativeC.mln_acquired_frame_dispose(handle))
  }

  actual fun buffer(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(handle, "mln_buffer_destroy")
    MapLibreNativeC.mln_buffer_destroy(handle)
  }

  actual fun eventBatch(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_event_batch_release",
    )
    MapLibreNativeC.mln_event_batch_release(handle)
  }

  actual fun geojsonSourceData(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_geojson_source_data_destroy",
    )
    MapLibreNativeC.mln_geojson_source_data_destroy(handle)
  }

  actual fun map(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(handle, "mln_map_dispose")
    Status.check(MapLibreNativeC.mln_map_dispose(handle))
  }

  actual fun mapProjection(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_map_projection_close",
    )
    Status.check(MapLibreNativeC.mln_map_projection_close(handle))
  }

  actual fun renderFrameBatch(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_render_frame_batch_release",
    )
    MapLibreNativeC.mln_render_frame_batch_release(handle)
  }

  actual fun renderSession(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_render_session_dispose",
    )
    Status.check(MapLibreNativeC.mln_render_session_dispose(handle))
  }

  actual fun resourceRequestHandle(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_resource_request_release",
    )
    MapLibreNativeC.mln_resource_request_release(handle)
  }

  actual fun runtime(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(handle, "mln_runtime_dispose")
    Status.check(MapLibreNativeC.mln_runtime_dispose(handle))
  }
}
