package org.maplibre.nativeffi.style

import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.json.JsonValue

/**
 * Mutable descriptor for GeoJSON style sources. These options are fixed when the source is created;
 * the data update APIs keep the options the source was added with.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class GeoJsonSourceOptions {
  public var minZoom: Double? = null

  public var maxZoom: Double? = null

  public var tolerance: Double? = null

  public var clusterMaxZoom: Double? = null

  /**
   * Cluster aggregation expressions keyed by property name, as a JSON object whose members follow
   * the MapLibre Style Spec `clusterProperties` form.
   */
  public var clusterProperties: JsonValue? = null

  public var tileSize: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "tileSize must be non-negative" } }
      field = value
    }

  public var buffer: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "buffer must be non-negative" } }
      field = value
    }

  public var clusterRadius: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "clusterRadius must be non-negative" } }
      field = value
    }

  public var clusterMinPoints: Int? = null
    set(value) {
      value?.let { Status.requireArgument(it >= 0) { "clusterMinPoints must be non-negative" } }
      field = value
    }

  public var lineMetrics: Boolean? = null

  public var cluster: Boolean? = null

  /** Tiles data updates inline, so a data update reaches the next rendered frame. */
  public var synchronousUpdate: Boolean? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: GeoJsonSourceOptions.() -> Unit = {}): GeoJsonSourceOptions =
    GeoJsonSourceOptions()
      .also {
        it.minZoom = minZoom
        it.maxZoom = maxZoom
        it.tolerance = tolerance
        it.clusterMaxZoom = clusterMaxZoom
        it.clusterProperties = clusterProperties
        it.tileSize = tileSize
        it.buffer = buffer
        it.clusterRadius = clusterRadius
        it.clusterMinPoints = clusterMinPoints
        it.lineMetrics = lineMetrics
        it.cluster = cluster
        it.synchronousUpdate = synchronousUpdate
      }
      .apply(block)

  private val fields: List<Any?>
    get() =
      listOf(
        minZoom,
        maxZoom,
        tolerance,
        clusterMaxZoom,
        clusterProperties,
        tileSize,
        buffer,
        clusterRadius,
        clusterMinPoints,
        lineMetrics,
        cluster,
        synchronousUpdate,
      )

  override fun equals(other: Any?): Boolean =
    other is GeoJsonSourceOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
