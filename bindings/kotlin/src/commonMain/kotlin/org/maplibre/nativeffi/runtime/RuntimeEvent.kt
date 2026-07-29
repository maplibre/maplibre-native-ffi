package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.map.MapHandle

/**
 * Event copied from a runtime's native event queue.
 *
 * @property type the event kind; an open domain, so handle unknown values.
 * @property sourceType which handle kind raised the event; an open domain.
 * @property runtimeSource the runtime that raised the event, when it can be resolved.
 * @property mapSource the map that raised the event, resolved through a weak reference to the
 *   public wrapper. This is null when the host no longer holds a strong reference to that
 *   [MapHandle], even though the event did originate from a map. Read [sourceType] to tell a
 *   map-originated event from a runtime-originated one, and keep your own strong reference to any
 *   map whose events you plan to attribute.
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
   * Secondary event detail whose meaning [type] selects, and which is 0 for the types that carry no
   * detail:
   * - [RuntimeEventType.MAP_CAMERA_WILL_CHANGE] and [RuntimeEventType.MAP_CAMERA_DID_CHANGE] carry
   *   a [CameraChangeMode], read as `CameraChangeMode(event.code)`.
   * - [RuntimeEventType.MAP_LOADING_FAILED] carries the ordinal of MapLibre Native's internal map
   *   load error kind, which the C API leaves unnamed.
   * - [RuntimeEventType.OFFLINE_OPERATION_COMPLETED] carries the operation result as a native
   *   status value.
   */
  public val code: Int,
  public val payload: RuntimeEventPayload,
  public val message: String,
)
