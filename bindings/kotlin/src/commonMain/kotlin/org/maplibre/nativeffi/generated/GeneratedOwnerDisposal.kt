// Generated from handle disposal relationships by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.callback.CallbackAdmission
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

internal object GeneratedOwnerDisposal {
  fun acquiredFrame(handle: Long) {
    CallbackAdmission.check(handle, "mln_acquired_frame_dispose")
    NativeDiagnostics.check { diagnostic -> C.mln_acquired_frame_dispose(handle, diagnostic) }
  }

  fun buffer(handle: Long) {
    CallbackAdmission.check(handle, "mln_buffer_destroy")
    C.mln_buffer_destroy(handle)
  }

  fun eventBatch(handle: Long) {
    CallbackAdmission.check(handle, "mln_event_batch_release")
    C.mln_event_batch_release(handle)
  }

  fun geojsonSourceData(handle: Long) {
    CallbackAdmission.check(handle, "mln_geojson_source_data_destroy")
    C.mln_geojson_source_data_destroy(handle)
  }

  fun map(handle: Long) {
    CallbackAdmission.check(handle, "mln_map_dispose")
    NativeDiagnostics.check { diagnostic -> C.mln_map_dispose(handle, diagnostic) }
  }

  fun mapProjection(handle: Long) {
    CallbackAdmission.check(handle, "mln_map_projection_close")
    NativeDiagnostics.check { diagnostic -> C.mln_map_projection_close(handle, diagnostic) }
  }

  fun renderFrameBatch(handle: Long) {
    CallbackAdmission.check(handle, "mln_render_frame_batch_release")
    C.mln_render_frame_batch_release(handle)
  }

  fun renderSession(handle: Long) {
    CallbackAdmission.check(handle, "mln_render_session_dispose")
    NativeDiagnostics.check { diagnostic -> C.mln_render_session_dispose(handle, diagnostic) }
  }

  fun resourceRequestHandle(handle: Long) {
    CallbackAdmission.check(handle, "mln_resource_request_release")
    C.mln_resource_request_release(handle)
  }

  fun runtime(handle: Long) {
    CallbackAdmission.check(handle, "mln_runtime_dispose")
    NativeDiagnostics.check { diagnostic -> C.mln_runtime_dispose(handle, diagnostic) }
  }
}
