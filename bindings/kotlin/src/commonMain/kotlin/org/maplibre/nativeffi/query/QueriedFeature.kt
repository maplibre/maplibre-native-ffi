package org.maplibre.nativeffi.query

/**
 * One query hit copied from a queried-feature list.
 *
 * [feature] is one UTF-8 GeoJSON Feature. [sourceId], [sourceLayerId], and [state] are present when
 * the native hit carried them. [state] is a UTF-8 JSON object. Constructor bytes are snapshotted,
 * so later caller mutation does not change this value.
 */
public class QueriedFeature(
  feature: ByteArray,
  public val sourceId: String?,
  public val sourceLayerId: String?,
  state: ByteArray?,
) {
  private val featureBytes: ByteArray = feature.copyOf()
  private val stateBytes: ByteArray? = state?.copyOf()

  public val feature: ByteArray
    get() = featureBytes.copyOf()

  public val state: ByteArray?
    get() = stateBytes?.copyOf()

  override fun equals(other: Any?): Boolean =
    other is QueriedFeature &&
      featureBytes.contentEquals(other.featureBytes) &&
      sourceId == other.sourceId &&
      sourceLayerId == other.sourceLayerId &&
      stateBytes.contentEquals(other.stateBytes)

  override fun hashCode(): Int {
    var result = featureBytes.contentHashCode()
    result = 31 * result + sourceId.hashCode()
    result = 31 * result + sourceLayerId.hashCode()
    result = 31 * result + (stateBytes?.contentHashCode() ?: 0)
    return result
  }

  override fun toString(): String =
    "QueriedFeature(feature=${featureBytes.size} bytes, sourceId=$sourceId, sourceLayerId=$sourceLayerId, state=${stateBytes?.size} bytes)"
}
