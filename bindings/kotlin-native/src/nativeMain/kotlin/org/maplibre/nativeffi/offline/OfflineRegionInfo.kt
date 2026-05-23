package org.maplibre.nativeffi.offline

/** Offline region snapshot copied from native snapshot or list handles. */
public class OfflineRegionInfo(
  public val id: Long,
  public val definition: OfflineRegionDefinition,
  metadata: ByteArray,
) {
  public val metadata: ByteArray = metadata.copyOf()

  override fun equals(other: Any?): Boolean =
    other is OfflineRegionInfo &&
      id == other.id &&
      definition == other.definition &&
      metadata.contentEquals(other.metadata)

  override fun hashCode(): Int {
    var result = id.hashCode()
    result = 31 * result + definition.hashCode()
    result = 31 * result + metadata.contentHashCode()
    return result
  }

  override fun toString(): String =
    "OfflineRegionInfo(id=$id, definition=$definition, metadata=${metadata.size} bytes)"
}
