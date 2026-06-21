package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.geo.TileId
import org.maplibre.nativeffi.map.RenderingStats
import org.maplibre.nativeffi.map.TileOperation
import org.maplibre.nativeffi.offline.OfflineRegionStatus
import org.maplibre.nativeffi.render.RenderMode
import org.maplibre.nativeffi.resource.ResourceErrorReason

/** Copied payload for a runtime event. */
public sealed interface RuntimeEventPayload {
  public data object None : RuntimeEventPayload

  public data class RenderFrame(
    public val mode: RenderMode,
    public val needsRepaint: Boolean,
    public val placementChanged: Boolean,
    public val stats: RenderingStats,
  ) : RuntimeEventPayload

  public data class RenderMap(public val mode: RenderMode) : RuntimeEventPayload

  public data class StyleImageMissing(public val imageId: String) : RuntimeEventPayload

  public data class TileAction(
    public val operation: TileOperation,
    public val tileId: TileId,
    public val sourceId: String,
  ) : RuntimeEventPayload

  public data class OfflineRegionStatusChanged(
    public val regionId: Long,
    public val status: OfflineRegionStatus,
  ) : RuntimeEventPayload

  public data class OfflineRegionResponseError(
    public val regionId: Long,
    public val reason: ResourceErrorReason,
  ) : RuntimeEventPayload

  public data class OfflineRegionTileCountLimit(public val regionId: Long, public val limit: Long) :
    RuntimeEventPayload

  public data class OfflineOperationCompleted(
    /** Native `uint64_t` operation id preserved as a [Long] bit pattern. */
    public val operationId: Long,
    public val operationKind: OfflineOperationKind,
    public val resultKind: OfflineOperationResultKind,
    public val resultStatus: Int,
    public val found: Boolean,
  ) : RuntimeEventPayload

  public class Unknown(
    public val rawPayloadType: Int,
    public val payloadSize: Long,
    payloadBytes: ByteArray,
  ) : RuntimeEventPayload {
    private val copiedPayloadBytes: ByteArray = payloadBytes.copyOf()

    public val payloadBytes: ByteArray
      get() = copiedPayloadBytes.copyOf()

    override fun equals(other: Any?): Boolean =
      other is Unknown &&
        rawPayloadType == other.rawPayloadType &&
        payloadSize == other.payloadSize &&
        copiedPayloadBytes.contentEquals(other.copiedPayloadBytes)

    override fun hashCode(): Int {
      var result = rawPayloadType
      result = 31 * result + payloadSize.hashCode()
      result = 31 * result + copiedPayloadBytes.contentHashCode()
      return result
    }

    override fun toString(): String =
      "Unknown(rawPayloadType=$rawPayloadType, payloadSize=$payloadSize, payloadBytes=${copiedPayloadBytes.contentToString()})"
  }
}
