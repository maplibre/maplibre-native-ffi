package org.maplibre.nativeffi.runtime;

import org.maplibre.nativeffi.geo.TileId;
import org.maplibre.nativeffi.map.RenderingStats;
import org.maplibre.nativeffi.map.TileOperation;
import org.maplibre.nativeffi.offline.OfflineRegionStatus;
import org.maplibre.nativeffi.render.RenderMode;
import org.maplibre.nativeffi.resource.ResourceErrorReason;

/** Copied payload for a runtime event. */
public sealed interface RuntimeEventPayload
    permits RuntimeEventPayload.None,
        RuntimeEventPayload.RenderFrame,
        RuntimeEventPayload.RenderMap,
        RuntimeEventPayload.StyleImageMissing,
        RuntimeEventPayload.TileAction,
        RuntimeEventPayload.OfflineRegionStatusChanged,
        RuntimeEventPayload.OfflineRegionResponseError,
        RuntimeEventPayload.OfflineRegionTileCountLimit,
        RuntimeEventPayload.OfflineOperationCompleted,
        RuntimeEventPayload.Unknown {
  None NONE = new None();

  record None() implements RuntimeEventPayload {}

  record RenderFrame(
      RenderMode mode,
      int rawMode,
      boolean needsRepaint,
      boolean placementChanged,
      RenderingStats stats)
      implements RuntimeEventPayload {}

  record RenderMap(RenderMode mode, int rawMode) implements RuntimeEventPayload {}

  record StyleImageMissing(String imageId) implements RuntimeEventPayload {}

  record TileAction(TileOperation operation, int rawOperation, TileId tileId, String sourceId)
      implements RuntimeEventPayload {}

  record OfflineRegionStatusChanged(long regionId, OfflineRegionStatus status)
      implements RuntimeEventPayload {}

  record OfflineRegionResponseError(long regionId, ResourceErrorReason reason, int rawReason)
      implements RuntimeEventPayload {}

  record OfflineRegionTileCountLimit(long regionId, long limit) implements RuntimeEventPayload {}

  record OfflineOperationCompleted(
      long operationId,
      OfflineOperationKind operationKind,
      int rawOperationKind,
      OfflineOperationResultKind resultKind,
      int rawResultKind,
      int resultStatus,
      boolean found)
      implements RuntimeEventPayload {}

  record Unknown(int rawPayloadType, long payloadSize) implements RuntimeEventPayload {}
}
