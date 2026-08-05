package org.maplibre.nativeffi.runtime

import kotlin.jvm.JvmInline

/**
 * Camera change kind carried in [RuntimeEvent.code] for [RuntimeEventType.MAP_CAMERA_WILL_CHANGE]
 * and [RuntimeEventType.MAP_CAMERA_DID_CHANGE]. Read it as `CameraChangeMode(event.code)`. This is
 * an open domain; unnamed values keep their raw [nativeValue].
 */
@JvmInline
public value class CameraChangeMode(public val nativeValue: Int) {
  public companion object {
    /** The camera reached its new value without an animated transition. */
    public val IMMEDIATE: CameraChangeMode = CameraChangeMode(0)

    /** The camera moved as part of an animated transition. */
    public val ANIMATED: CameraChangeMode = CameraChangeMode(1)
  }
}
