package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.map.MapHandle

/**
 * Event copied from a runtime's native event queue.
 *
 * @property type the event kind; an open domain, so handle unknown values.
 * @property sourceType which handle kind raised the event; an open domain.
 * @property runtimeSource the runtime that raised the event, when it can be resolved.
 * @property mapSource the map that raised the event, resolved through a weak reference to the
 *   public wrapper and null once the host drops its own reference to that [MapHandle]. Read
 *   [sourceType] to tell a map-originated event from a runtime-originated one.
 * @property code the native status code carried by the event.
 * @property payload the typed payload, preserved as raw bytes for unknown domains.
 * @property message the copied diagnostic message.
 */
public data class RuntimeEvent(
  public val type: RuntimeEventType,
  public val sourceType: RuntimeEventSourceType,
  public val runtimeSource: RuntimeHandle?,
  public val mapSource: MapHandle?,
  /**
   * Secondary event detail whose meaning [type] selects, 0 for types that carry no detail:
   * - [RuntimeEventType.MAP_CAMERA_WILL_CHANGE] and [RuntimeEventType.MAP_CAMERA_DID_CHANGE] carry
   *   a [CameraChangeMode], read as `CameraChangeMode(event.code)`.
   * - [RuntimeEventType.MAP_LOADING_FAILED] carries the ordinal of an unnamed internal map load
   *   error kind.
   * - [RuntimeEventType.OFFLINE_OPERATION_COMPLETED] carries a native status value.
   */
  public val code: Int,
  public val payload: RuntimeEventPayload,
  public val message: String,
)
