package org.maplibre.nativeffi.style

/** Owned immutable prepared GeoJSON data. */
public expect class GeoJsonSourceDataHandle :
  org.maplibre.nativeffi.generated.GeneratedGeojsonSourceDataOperations, AutoCloseable {
  public val isClosed: Boolean

  override fun close()
}
