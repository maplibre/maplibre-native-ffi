package org.maplibre.nativeffi.internal.lifecycle

import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapProjectionHandle
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.GeoJsonSourceDataHandle

internal object OwnerAdoption {
  fun map(raw: ULong, parent: RuntimeHandle): MapHandle = MapHandle(parent, NativeMap(raw.toLong()))

  fun mapProjection(raw: ULong): MapProjectionHandle =
    MapProjectionHandle(NativeMapProjection(raw.toLong()))

  fun geojsonSourceData(raw: ULong): GeoJsonSourceDataHandle =
    GeoJsonSourceDataHandle(NativeGeoJsonSourceData(raw.toLong()))

  fun acquiredFrame(
    raw: ULong,
    parent: org.maplibre.nativeffi.render.RenderSessionHandle,
  ): org.maplibre.nativeffi.render.AcquiredFrameHandle = parent.adoptAcquiredFrame(raw)

  fun runtime(raw: ULong): org.maplibre.nativeffi.runtime.RuntimeHandle =
    org.maplibre.nativeffi.runtime.RuntimeHandle(NativeRuntime(raw.toLong()))

  fun renderSession(
    raw: ULong,
    parent: MapHandle,
  ): org.maplibre.nativeffi.render.RenderSessionHandle =
    org.maplibre.nativeffi.render.RenderSessionHandle(parent, NativeRenderSession(raw.toLong()))
}
