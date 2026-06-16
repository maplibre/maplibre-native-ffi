package org.maplibre.nativeffi.runtime;

import java.util.Arrays;
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

  /**
   * Native event payload that this binding could not decode.
   *
   * @param payloadSize byte count reported by native
   * @param rawPayload copied payload bytes, which can be shorter than {@code payloadSize} when
   *     native supplied a null payload pointer
   */
  record Unknown(int rawPayloadType, long payloadSize, byte[] rawPayload)
      implements RuntimeEventPayload {
    public Unknown {
      rawPayload = rawPayload == null ? new byte[0] : rawPayload.clone();
    }

    @Override
    public byte[] rawPayload() {
      return rawPayload.clone();
    }

    @Override
    public boolean equals(Object other) {
      return other instanceof Unknown that
          && rawPayloadType == that.rawPayloadType
          && payloadSize == that.payloadSize
          && Arrays.equals(rawPayload, that.rawPayload);
    }

    @Override
    public int hashCode() {
      var result = Integer.hashCode(rawPayloadType);
      result = 31 * result + Long.hashCode(payloadSize);
      result = 31 * result + Arrays.hashCode(rawPayload);
      return result;
    }

    @Override
    public String toString() {
      return "Unknown[rawPayloadType="
          + rawPayloadType
          + ", payloadSize="
          + payloadSize
          + ", rawPayload="
          + rawPayload.length
          + " bytes]";
    }
  }
}
