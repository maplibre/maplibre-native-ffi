package org.maplibre.nativeffi.runtime

import kotlin.jvm.JvmInline

/**
 * Set of runtime event types that a map or a runtime queues.
 *
 * One bit selects one [RuntimeEventType]: the bit for a type is 1 shifted left by the type's
 * [RuntimeEventType.nativeValue], so [of] derives the bit for a type this version does not name. An
 * event type that a mask leaves out is never built or queued, so it neither reaches a batch nor
 * wakes the notification receiver.
 *
 * [org.maplibre.nativeffi.map.MapHandle.eventMask] reads the [ALL_MAP_EVENTS] bits and
 * [RuntimeHandle.eventMask] reads the [ALL_RUNTIME_EVENTS] bits, so both accept [ALL]. A bit
 * outside [ALL] fails both setters with [org.maplibre.nativeffi.error.InvalidArgumentException].
 */
@JvmInline
public value class RuntimeEventMask(public val nativeValue: Long) {
  /** Returns the mask that selects every type this mask or [other] selects. */
  public operator fun plus(other: RuntimeEventMask): RuntimeEventMask =
    RuntimeEventMask(nativeValue or other.nativeValue)

  /** Returns the mask that selects every type this mask selects and [other] leaves out. */
  public operator fun minus(other: RuntimeEventMask): RuntimeEventMask =
    RuntimeEventMask(nativeValue and other.nativeValue.inv())

  /** Reports whether this mask selects [type]. */
  public operator fun contains(type: RuntimeEventType): Boolean =
    nativeValue and of(type).nativeValue != 0L

  /** Reports whether this mask selects no event type. */
  public fun isEmpty(): Boolean = nativeValue == 0L

  public companion object {
    /** Selects no event type. */
    public val NONE: RuntimeEventMask = RuntimeEventMask(0)

    public val MAP_CAMERA_WILL_CHANGE: RuntimeEventMask =
      of(RuntimeEventType.MAP_CAMERA_WILL_CHANGE)
    public val MAP_CAMERA_IS_CHANGING: RuntimeEventMask =
      of(RuntimeEventType.MAP_CAMERA_IS_CHANGING)
    public val MAP_CAMERA_DID_CHANGE: RuntimeEventMask = of(RuntimeEventType.MAP_CAMERA_DID_CHANGE)
    public val MAP_STYLE_LOADED: RuntimeEventMask = of(RuntimeEventType.MAP_STYLE_LOADED)
    public val MAP_LOADING_STARTED: RuntimeEventMask = of(RuntimeEventType.MAP_LOADING_STARTED)
    public val MAP_LOADING_FINISHED: RuntimeEventMask = of(RuntimeEventType.MAP_LOADING_FINISHED)
    public val MAP_LOADING_FAILED: RuntimeEventMask = of(RuntimeEventType.MAP_LOADING_FAILED)
    public val MAP_IDLE: RuntimeEventMask = of(RuntimeEventType.MAP_IDLE)
    public val MAP_RENDER_UPDATE_AVAILABLE: RuntimeEventMask =
      of(RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE)
    public val MAP_RENDER_ERROR: RuntimeEventMask = of(RuntimeEventType.MAP_RENDER_ERROR)
    public val MAP_STILL_IMAGE_FINISHED: RuntimeEventMask =
      of(RuntimeEventType.MAP_STILL_IMAGE_FINISHED)
    public val MAP_STILL_IMAGE_FAILED: RuntimeEventMask =
      of(RuntimeEventType.MAP_STILL_IMAGE_FAILED)
    public val MAP_RENDER_FRAME_STARTED: RuntimeEventMask =
      of(RuntimeEventType.MAP_RENDER_FRAME_STARTED)
    public val MAP_RENDER_FRAME_FINISHED: RuntimeEventMask =
      of(RuntimeEventType.MAP_RENDER_FRAME_FINISHED)
    public val MAP_RENDER_MAP_STARTED: RuntimeEventMask =
      of(RuntimeEventType.MAP_RENDER_MAP_STARTED)
    public val MAP_RENDER_MAP_FINISHED: RuntimeEventMask =
      of(RuntimeEventType.MAP_RENDER_MAP_FINISHED)
    public val MAP_STYLE_IMAGE_MISSING: RuntimeEventMask =
      of(RuntimeEventType.MAP_STYLE_IMAGE_MISSING)
    public val MAP_TILE_ACTION: RuntimeEventMask = of(RuntimeEventType.MAP_TILE_ACTION)
    public val MAP_CAMERA_TRANSITION_FINISHED: RuntimeEventMask =
      of(RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED)
    public val COMMAND_FINISHED: RuntimeEventMask = of(RuntimeEventType.COMMAND_FINISHED)
    public val OFFLINE_REGION_STATUS_CHANGED: RuntimeEventMask =
      of(RuntimeEventType.OFFLINE_REGION_STATUS_CHANGED)
    public val OFFLINE_REGION_RESPONSE_ERROR: RuntimeEventMask =
      of(RuntimeEventType.OFFLINE_REGION_RESPONSE_ERROR)
    public val OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED: RuntimeEventMask =
      of(RuntimeEventType.OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED)

    /** Selects every map-originated event type this version defines. */
    public val ALL_MAP_EVENTS: RuntimeEventMask =
      MAP_CAMERA_WILL_CHANGE +
        MAP_CAMERA_IS_CHANGING +
        MAP_CAMERA_DID_CHANGE +
        MAP_STYLE_LOADED +
        MAP_LOADING_STARTED +
        MAP_LOADING_FINISHED +
        MAP_LOADING_FAILED +
        MAP_IDLE +
        MAP_RENDER_UPDATE_AVAILABLE +
        MAP_RENDER_ERROR +
        MAP_STILL_IMAGE_FINISHED +
        MAP_STILL_IMAGE_FAILED +
        MAP_RENDER_FRAME_STARTED +
        MAP_RENDER_FRAME_FINISHED +
        MAP_RENDER_MAP_STARTED +
        MAP_RENDER_MAP_FINISHED +
        MAP_STYLE_IMAGE_MISSING +
        MAP_TILE_ACTION +
        MAP_CAMERA_TRANSITION_FINISHED +
        COMMAND_FINISHED

    /** Selects every runtime-originated event type this version defines. */
    public val ALL_RUNTIME_EVENTS: RuntimeEventMask =
      OFFLINE_REGION_STATUS_CHANGED +
        OFFLINE_REGION_RESPONSE_ERROR +
        OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED +
        COMMAND_FINISHED

    /** Selects every event type this version defines. */
    public val ALL: RuntimeEventMask = ALL_MAP_EVENTS + ALL_RUNTIME_EVENTS

    /**
     * Returns the mask that selects [type] alone.
     *
     * A mask holds one bit per event type, so a type whose [RuntimeEventType.nativeValue] is
     * outside 0 through 63 has no bit and yields [NONE].
     */
    public fun of(type: RuntimeEventType): RuntimeEventMask =
      if (type.nativeValue in 0..63) RuntimeEventMask(1L shl type.nativeValue) else NONE
  }
}
