package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.map.MapHandle

/**
 * Event copied from a runtime's native event queue.
 *
 * @property type the event kind; an open domain, so handle unknown values.
 * @property sourceType which handle kind raised the event; an open domain.
 * @property sourceId the identity of the handle that raised the event, carried for every event
 *   including one whose [sourceType] this version does not name. Native `uint64_t` preserved as a
 *   [Long] bit pattern; format through `toULong()`. The value names one object for the life of the
 *   process, so a host may route or correlate on it even after the handle it names is closed. It is
 *   an identity value only: no public API turns it back into a handle.
 * @property runtimeSource the runtime that raised the event, when it can be resolved.
 * @property mapSource the map that raised the event, resolved through a weak reference to the
 *   public wrapper and null once the host drops its own reference to that [MapHandle]. Read
 *   [sourceType] to tell a map-originated event from a runtime-originated one.
 * @property code the secondary event detail carried by the event.
 * @property payload the typed payload, preserved as raw bytes for unknown domains.
 * @property message the copied diagnostic message.
 */
public data class RuntimeEvent(
  public val type: RuntimeEventType,
  public val sourceType: RuntimeEventSourceType,
  public val sourceId: Long,
  public val runtimeSource: RuntimeHandle?,
  public val mapSource: MapHandle?,
  /**
   * Secondary event detail whose meaning [type] selects; types not listed here carry 0:
   * - [RuntimeEventType.MAP_CAMERA_WILL_CHANGE] and [RuntimeEventType.MAP_CAMERA_DID_CHANGE] carry
   *   a [CameraChangeMode], read as `CameraChangeMode(event.code)`.
   * - [RuntimeEventType.MAP_LOADING_FAILED] carries the ordinal of an unnamed internal map load
   *   error kind.
   * - [RuntimeEventType.COMMAND_FINISHED] carries the command's final native status code, read as
   *   `MaplibreStatus(event.code)`; this is where a NOT_FOUND disposition surfaces.
   */
  public val code: Int,
  public val payload: RuntimeEventPayload,
  public val message: String,
)
