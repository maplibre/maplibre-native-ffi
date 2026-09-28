package org.maplibre.nativeffi.internal.lifecycle

import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapProjectionHandle
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.GeoJsonSourceDataHandle

internal object OwnerAdoption {
  fun map(raw: Long, parent: RuntimeHandle): MapHandle = MapHandle(parent, NativeMap(raw))

  fun mapProjection(raw: Long): MapProjectionHandle = MapProjectionHandle(NativeMapProjection(raw))

  fun geojsonSourceData(raw: Long): GeoJsonSourceDataHandle =
    GeoJsonSourceDataHandle(NativeGeoJsonSourceData(raw))

  fun acquiredFrame(
    raw: Long,
    parent: org.maplibre.nativeffi.render.RenderSessionHandle,
  ): org.maplibre.nativeffi.render.AcquiredFrameHandle = parent.adoptAcquiredFrame(raw)

  fun runtime(raw: Long): org.maplibre.nativeffi.runtime.RuntimeHandle =
    org.maplibre.nativeffi.runtime.RuntimeHandle(NativeRuntime(raw))

  fun renderSession(
    raw: Long,
    parent: MapHandle,
  ): org.maplibre.nativeffi.render.RenderSessionHandle =
    org.maplibre.nativeffi.render.RenderSessionHandle(parent, NativeRenderSession(raw))
}
