package org.maplibre.nativeffi;

import java.util.Optional;

/** Event copied from a runtime's native event queue. */
public record RuntimeEvent(
    RuntimeEventType type,
    int rawType,
    RuntimeEventSourceType sourceType,
    int rawSourceType,
    Optional<RuntimeHandle> runtimeSource,
    Optional<MapHandle> mapSource,
    int code,
    int rawPayloadType,
    RuntimeEventPayload payload,
    String message) {
  public RuntimeEvent {
    runtimeSource = runtimeSource == null ? Optional.empty() : runtimeSource;
    mapSource = mapSource == null ? Optional.empty() : mapSource;
  }
}
