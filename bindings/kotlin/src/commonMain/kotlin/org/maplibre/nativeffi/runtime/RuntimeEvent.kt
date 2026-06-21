package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.map.MapHandle

/** Event copied from a runtime's native event queue. */
public data class RuntimeEvent(
  public val type: RuntimeEventType,
  public val sourceType: RuntimeEventSourceType,
  public val runtimeSource: RuntimeHandle?,
  public val mapSource: MapHandle?,
  public val code: Int,
  public val payload: RuntimeEventPayload,
  public val message: String,
)
