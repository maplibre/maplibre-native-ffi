package org.maplibre.nativejni.runtime;

import java.util.Optional;
import org.maplibre.nativejni.map.MapHandle;

/** Event copied from a runtime's native event queue. */
public record RuntimeEvent(
    RuntimeEventType type,
    RuntimeEventSourceType sourceType,
    Optional<RuntimeHandle> runtimeSource,
    Optional<MapHandle> mapSource,
    int code,
    RuntimeEventPayload payload,
    String message) {
  public RuntimeEvent {
    runtimeSource = runtimeSource == null ? Optional.empty() : runtimeSource;
    mapSource = mapSource == null ? Optional.empty() : mapSource;
  }
}
