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
    public val rawMode: UInt,
    public val needsRepaint: Boolean,
    public val placementChanged: Boolean,
    public val stats: RenderingStats,
  ) : RuntimeEventPayload

  public data class RenderMap(public val mode: RenderMode, public val rawMode: UInt) :
    RuntimeEventPayload

  public data class StyleImageMissing(public val imageId: String) : RuntimeEventPayload

  public data class TileAction(
    public val operation: TileOperation,
    public val rawOperation: UInt,
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
    public val rawReason: UInt,
  ) : RuntimeEventPayload

  public data class OfflineRegionTileCountLimit(
    public val regionId: Long,
    public val limit: ULong,
  ) : RuntimeEventPayload

  public data class OfflineOperationCompleted(
    public val operationId: ULong,
    public val operationKind: OfflineOperationKind,
    public val rawOperationKind: UInt,
    public val resultKind: OfflineOperationResultKind,
    public val rawResultKind: UInt,
    public val resultStatus: Int,
    public val found: Boolean,
  ) : RuntimeEventPayload

  public data class Unknown(public val rawPayloadType: UInt, public val payloadSize: ULong) :
    RuntimeEventPayload
}
