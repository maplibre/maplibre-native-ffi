package org.maplibre.nativeffi.query

/**
 * Mutable options for rendered feature queries.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Assigning [layerIds]
 * snapshots the caller's list and [filter] bytes, so later caller mutation does not change this
 * descriptor. Keep an instance unmodified while it is a key in a hash-based collection.
 */
public class RenderedFeatureQueryOptions {
  public var layerIds: List<String>? = null
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
  public fun copy(block: RenderedFeatureQueryOptions.() -> Unit = {}): RenderedFeatureQueryOptions =
    RenderedFeatureQueryOptions()
      .also {
        it.layerIds = layerIds
        it.filterBytes = filterBytes?.copyOf()
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(layerIds, filterBytes?.contentHashCode())

  override fun equals(other: Any?): Boolean =
    other is RenderedFeatureQueryOptions &&
      layerIds == other.layerIds &&
      renderedQueryBytesEqual(filterBytes, other.filterBytes)

  override fun hashCode(): Int = fields.hashCode()
}

private fun renderedQueryBytesEqual(left: ByteArray?, right: ByteArray?): Boolean =
  when {
    left == null -> right == null
    right == null -> false
    else -> left.contentEquals(right)
  }
