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

  /** The event message carries the source id. */
  public data class TileAction(public val operation: TileOperation, public val tileId: TileId) :
    RuntimeEventPayload

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

  /**
   * Reports that a camera transition released the camera.
   *
   * @see org.maplibre.nativeffi.camera.AnimationOptions.transitionId
   */
  public data class CameraTransitionFinished(
    /**
     * The transition id the caller set on the animation options that started this transition.
     * Native `uint64_t` preserved as a [Long] bit pattern; format through `toULong()`.
     */
    public val transitionId: Long
  ) : RuntimeEventPayload

  /** Terminal outcome for one accepted runtime command. */
  public data class CommandFinished(
    public val commandId: ULong,
    public val disposition: CommandDisposition,
    public val generation: ULong,
  ) : RuntimeEventPayload

  /**
   * Payload of a kind this version does not name.
   *
   * [payloadBytes] holds the whole payload window that the drained batch reported, which is the
   * batch stride minus the offset of the payload inside one event record.
   */
  public class Unknown(public val rawPayloadType: Int, payloadBytes: ByteArray) :
    RuntimeEventPayload {
    private val copiedPayloadBytes: ByteArray = payloadBytes.copyOf()

    public val payloadBytes: ByteArray
      get() = copiedPayloadBytes.copyOf()

    override fun equals(other: Any?): Boolean =
      other is Unknown &&
        rawPayloadType == other.rawPayloadType &&
        copiedPayloadBytes.contentEquals(other.copiedPayloadBytes)

    override fun hashCode(): Int {
      var result = rawPayloadType
      result = 31 * result + copiedPayloadBytes.contentHashCode()
      return result
    }

    override fun toString(): String =
      "Unknown(rawPayloadType=$rawPayloadType, payloadBytes=${copiedPayloadBytes.contentToString()})"
  }
}
