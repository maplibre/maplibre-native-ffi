package org.maplibre.nativeffi.runtime

import kotlin.jvm.JvmInline

/**
 * Runtime event type copied from the native event queue. It selects the meaning of
 * [RuntimeEvent.code] and the [RuntimeEventPayload] variant behind [RuntimeEvent.payload].
 *
 * This is an open domain: a value may have no named constant here, so a `when` over this type needs
 * an `else` branch.
 */
@JvmInline
public value class RuntimeEventType(public val nativeValue: Int) {
  public companion object {
    /** [RuntimeEvent.code] carries a [CameraChangeMode]. */
    public val MAP_CAMERA_WILL_CHANGE: RuntimeEventType = RuntimeEventType(1)

    public val MAP_CAMERA_IS_CHANGING: RuntimeEventType = RuntimeEventType(2)

    /** [RuntimeEvent.code] carries a [CameraChangeMode]. */
    public val MAP_CAMERA_DID_CHANGE: RuntimeEventType = RuntimeEventType(3)

    public val MAP_STYLE_LOADED: RuntimeEventType = RuntimeEventType(4)
    public val MAP_LOADING_STARTED: RuntimeEventType = RuntimeEventType(5)
    public val MAP_LOADING_FINISHED: RuntimeEventType = RuntimeEventType(6)

    /**
     * [RuntimeEvent.code] carries the ordinal of MapLibre Native's internal map load error kind,
     * which the C API leaves unnamed. [RuntimeEvent.message] carries the failure text.
     */
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

    /** [RuntimeEvent.message] carries the missing image id. */
    public val MAP_STYLE_IMAGE_MISSING: RuntimeEventType = RuntimeEventType(17)

    /** [RuntimeEvent.message] carries the source id. */
    public val MAP_TILE_ACTION: RuntimeEventType = RuntimeEventType(18)

    public val OFFLINE_REGION_STATUS_CHANGED: RuntimeEventType = RuntimeEventType(19)
    public val OFFLINE_REGION_RESPONSE_ERROR: RuntimeEventType = RuntimeEventType(20)
    public val OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED: RuntimeEventType = RuntimeEventType(21)

    /**
     * [RuntimeEvent.code] carries the operation result as a native status value, the same value
     * [RuntimeEventPayload.OfflineOperationCompleted.resultStatus] reports.
     */
    public val OFFLINE_OPERATION_COMPLETED: RuntimeEventType = RuntimeEventType(22)

    /** [RuntimeEvent.payload] carries [RuntimeEventPayload.CameraTransitionFinished]. */
    public val MAP_CAMERA_TRANSITION_FINISHED: RuntimeEventType = RuntimeEventType(23)

    internal fun fromNative(nativeValue: UInt): RuntimeEventType = fromNative(nativeValue.toInt())

    internal fun fromNative(nativeValue: Int): RuntimeEventType = RuntimeEventType(nativeValue)
  }
}
