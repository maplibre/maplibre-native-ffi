// Generated from handle disposal relationships by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

internal object GeneratedOwnerDisposal {
  fun acquiredFrame(handle: Long) {
    NativeDiagnostics.check { diagnostic -> C.mln_acquired_frame_dispose(handle, diagnostic) }
  }

  fun geojsonSourceData(handle: Long) {
    C.mln_geojson_source_data_release(handle)
  }

  fun map(handle: Long) {
    NativeDiagnostics.check { diagnostic -> C.mln_map_dispose(handle, diagnostic) }
  }

  fun mapProjection(handle: Long) {
    NativeDiagnostics.check { diagnostic -> C.mln_map_projection_release(handle, diagnostic) }
  }

  fun renderFrameBatch(handle: Long) {
    C.mln_render_frame_batch_release(handle)
  }

  fun renderSession(handle: Long) {
    NativeDiagnostics.check { diagnostic -> C.mln_render_session_dispose(handle, diagnostic) }
  }

  fun resourceRequest(handle: Long) {
    C.mln_resource_request_release(handle)
  }

  fun resourceTransformRequest(handle: Long) {
    C.mln_resource_transform_request_release(handle)
  }

  fun runtime(handle: Long) {
    NativeDiagnostics.check { diagnostic -> C.mln_runtime_dispose(handle, diagnostic) }
  }
}
