// Generated from handle disposal relationships. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.status.Status

internal actual object GeneratedOwnerDisposal {
  actual fun acquiredFrame(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_acquired_frame_dispose",
    )
    Status.check(MaplibreNativeC.mln_acquired_frame_dispose(handle))
  }

  actual fun buffer(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(handle, "mln_buffer_destroy")
    MaplibreNativeC.mln_buffer_destroy(handle)
  }

  actual fun eventBatch(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_event_batch_release",
    )
    MaplibreNativeC.mln_event_batch_release(handle)
  }

  actual fun geojsonSourceData(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_geojson_source_data_destroy",
    )
    MaplibreNativeC.mln_geojson_source_data_destroy(handle)
  }

  actual fun map(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(handle, "mln_map_dispose")
    Status.check(MaplibreNativeC.mln_map_dispose(handle))
  }

  actual fun mapProjection(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_map_projection_close",
    )
    Status.check(MaplibreNativeC.mln_map_projection_close(handle))
  }

  actual fun renderFrameBatch(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_render_frame_batch_release",
    )
    MaplibreNativeC.mln_render_frame_batch_release(handle)
  }

  actual fun renderSession(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_render_session_dispose",
    )
    Status.check(MaplibreNativeC.mln_render_session_dispose(handle))
  }

  actual fun resourceRequestHandle(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      handle,
      "mln_resource_request_release",
    )
    MaplibreNativeC.mln_resource_request_release(handle)
  }

  actual fun runtime(handle: Long) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(handle, "mln_runtime_dispose")
    Status.check(MaplibreNativeC.mln_runtime_dispose(handle))
  }
}
