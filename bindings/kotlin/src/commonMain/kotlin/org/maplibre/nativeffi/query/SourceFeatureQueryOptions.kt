package org.maplibre.nativeffi.query

/**
 * Mutable options for source feature queries.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Assigning
 * [sourceLayerIds] and [filter] snapshot caller-owned storage, so later caller mutation does not
 * change this descriptor. Keep an instance unmodified while it is a key in a hash-based collection.
 */
public class SourceFeatureQueryOptions {
  public var sourceLayerIds: List<String>? = null
    set(value) {
      field = value?.toList()
    }

  private var filterBytes: ByteArray? = null

  public var filter: ByteArray?
    get() = filterBytes?.copyOf()
    set(value) {
      filterBytes = value?.copyOf()
    }

  internal val filterTransit: ByteArray?
    get() = filterBytes

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: SourceFeatureQueryOptions.() -> Unit = {}): SourceFeatureQueryOptions =
    SourceFeatureQueryOptions()
      .also {
        it.sourceLayerIds = sourceLayerIds
        it.filterBytes = filterBytes?.copyOf()
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(sourceLayerIds, filterBytes?.contentHashCode())

  override fun equals(other: Any?): Boolean =
    other is SourceFeatureQueryOptions &&
      sourceLayerIds == other.sourceLayerIds &&
      filterBytes.contentEquals(other.filterBytes)

  override fun hashCode(): Int = fields.hashCode()
}
