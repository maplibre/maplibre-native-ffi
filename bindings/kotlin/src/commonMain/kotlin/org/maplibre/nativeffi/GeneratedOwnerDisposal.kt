// Generated from handle disposal relationships by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.JvmSynthetic
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

internal object GeneratedOwnerDisposal {
  @JvmSynthetic
  fun acquiredFrame(handle: Long) {
    NativeDiagnostics.check { diagnostic -> C.mln_acquired_frame_discard(handle, diagnostic) }
  }

  @JvmSynthetic
  fun framePacer(handle: Long) {
    C.mln_frame_pacer_release(handle)
  }

  @JvmSynthetic
  fun geoJsonSourceData(handle: Long) {
    C.mln_geo_json_source_data_release(handle)
  }

  @JvmSynthetic
  fun map(handle: Long) {
    NativeDiagnostics.check { diagnostic -> C.mln_map_dispose(handle, diagnostic) }
  }

  @JvmSynthetic
  fun mapProjection(handle: Long) {
    C.mln_map_projection_release(handle)
  }

  @JvmSynthetic
  fun offscreenDevice(handle: Long) {
    C.mln_offscreen_device_release(handle)
  }

  @JvmSynthetic
  fun renderSession(handle: Long) {
    NativeDiagnostics.check { diagnostic -> C.mln_render_session_dispose(handle, diagnostic) }
  }

  @JvmSynthetic
  fun resourceRequest(handle: Long) {
    C.mln_resource_request_release(handle)
  }

  @JvmSynthetic
  fun resourceTransformRequest(handle: Long) {
    C.mln_resource_transform_request_release(handle)
  }

  @JvmSynthetic
  fun runtime(handle: Long) {
    NativeDiagnostics.check { diagnostic -> C.mln_runtime_dispose(handle, diagnostic) }
  }

  @JvmSynthetic
  fun styleExpression(handle: Long) {
    C.mln_style_expression_release(handle)
  }

  @JvmSynthetic
  fun vulkanDevice(handle: Long) {
    C.mln_vulkan_device_release(handle)
  }
}
