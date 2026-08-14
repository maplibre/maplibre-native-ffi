package org.maplibre.nativeffi.style

/**
 * Owned prepared GeoJSON source data.
 *
 * [create] parses one complete UTF-8 GeoJSON document and tiles or clusters it into the index a
 * GeoJSON source consumes, which is the expensive part of a data update. The options are baked into
 * the prepared data: a source added with
 * [org.maplibre.nativeffi.map.MapHandle.addGeoJsonSourceData] adopts them, and
 * [org.maplibre.nativeffi.map.MapHandle.setGeoJsonSourceData] rejects data whose baked-in options
 * differ from the source's.
 *
 * Creation touches no runtime or map and is callable from any thread, so a host prepares data on a
 * worker thread or coroutine dispatcher and installs it on the map owner thread. The prepared data
 * is immutable and safe to share across threads. Install calls borrow the handle, so one prepared
 * value may be installed on any number of sources and closed at any time afterward; closing never
 * invalidates a source the data was installed on.
 */
public expect class GeoJsonSourceDataHandle : AutoCloseable {
  public val isClosed: Boolean

  override fun close()

  public companion object {
    /**
     * Prepares [data] with [options] for installation on GeoJSON sources.
     *
     * When [options] enable clustering, [data] must be a feature collection whose every feature
     * carries point geometry.
     */
    public fun create(
      data: ByteArray,
      options: GeoJsonSourceOptions? = null,
    ): GeoJsonSourceDataHandle
  }
}
