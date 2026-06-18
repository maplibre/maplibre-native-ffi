package org.maplibre.nativeffi.runtime

import kotlin.jvm.JvmInline

/** Runtime event type copied from the native event queue. */
@JvmInline
public value class RuntimeEventType(public val nativeValue: Int) {
  public companion object {
    public val MAP_CAMERA_WILL_CHANGE: RuntimeEventType = RuntimeEventType(1)
    public val MAP_CAMERA_IS_CHANGING: RuntimeEventType = RuntimeEventType(2)
    public val MAP_CAMERA_DID_CHANGE: RuntimeEventType = RuntimeEventType(3)
    public val MAP_STYLE_LOADED: RuntimeEventType = RuntimeEventType(4)
    public val MAP_LOADING_STARTED: RuntimeEventType = RuntimeEventType(5)
    public val MAP_LOADING_FINISHED: RuntimeEventType = RuntimeEventType(6)
    public val MAP_LOADING_FAILED: RuntimeEventType = RuntimeEventType(7)
    public val MAP_IDLE: RuntimeEventType = RuntimeEventType(8)
    public val MAP_RENDER_UPDATE_AVAILABLE: RuntimeEventType = RuntimeEventType(9)
    public val MAP_RENDER_ERROR: RuntimeEventType = RuntimeEventType(10)
    public val MAP_STILL_IMAGE_FINISHED: RuntimeEventType = RuntimeEventType(11)
    public val MAP_STILL_IMAGE_FAILED: RuntimeEventType = RuntimeEventType(12)
    public val MAP_RENDER_FRAME_STARTED: RuntimeEventType = RuntimeEventType(13)
    public val MAP_RENDER_FRAME_FINISHED: RuntimeEventType = RuntimeEventType(14)
    public val MAP_RENDER_MAP_STARTED: RuntimeEventType = RuntimeEventType(15)
    public val MAP_RENDER_MAP_FINISHED: RuntimeEventType = RuntimeEventType(16)
    public val MAP_STYLE_IMAGE_MISSING: RuntimeEventType = RuntimeEventType(17)
    public val MAP_TILE_ACTION: RuntimeEventType = RuntimeEventType(18)
    public val OFFLINE_REGION_STATUS_CHANGED: RuntimeEventType = RuntimeEventType(19)
    public val OFFLINE_REGION_RESPONSE_ERROR: RuntimeEventType = RuntimeEventType(20)
    public val OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED: RuntimeEventType = RuntimeEventType(21)
    public val OFFLINE_OPERATION_COMPLETED: RuntimeEventType = RuntimeEventType(22)

    internal fun fromNative(nativeValue: UInt): RuntimeEventType = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): RuntimeEventType = RuntimeEventType(nativeValue)
  }
}
