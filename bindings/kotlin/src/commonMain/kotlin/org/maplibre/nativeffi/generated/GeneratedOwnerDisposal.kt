// Generated from handle disposal relationships by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

internal object GeneratedOwnerDisposal {
  fun acquiredFrame(handle: Long) {
    NativeDiagnostics.check { diagnostic -> C.mln_acquired_frame_discard(handle, diagnostic) }
  }

  fun geoJsonSourceData(handle: Long) {
    C.mln_geo_json_source_data_release(handle)
  }

  fun map(handle: Long) {
    NativeDiagnostics.check { diagnostic -> C.mln_map_dispose(handle, diagnostic) }
  }

  fun mapProjection(handle: Long) {
    C.mln_map_projection_release(handle)
  }

  fun renderSession(handle: Long) {
    NativeDiagnostics.check { diagnostic -> C.mln_render_session_dispose(handle, 0L, diagnostic) }
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

  fun styleExpression(handle: Long) {
    C.mln_style_expression_release(handle)
  }
}
