package org.maplibre.nativeffi.map

/** Mutable descriptor used when creating a [MapHandle]. */
public class MapOptions {
  public var width: UInt? = null
    private set

  public var height: UInt? = null
    private set

  public var scaleFactor: Double? = null
    private set

  public var mapMode: MapMode? = null
    private set

  public fun size(width: UInt, height: UInt): MapOptions = apply {
    this.width = width
    this.height = height
  }

  public fun size(width: Int, height: Int): MapOptions = size(width.toUInt(), height.toUInt())

  public fun scaleFactor(scaleFactor: Double): MapOptions = apply { this.scaleFactor = scaleFactor }

  public fun clearScaleFactor(): MapOptions = apply { scaleFactor = null }

  public fun mapMode(mapMode: MapMode): MapOptions = apply { this.mapMode = mapMode }

  public fun clearMapMode(): MapOptions = apply { mapMode = null }
}
