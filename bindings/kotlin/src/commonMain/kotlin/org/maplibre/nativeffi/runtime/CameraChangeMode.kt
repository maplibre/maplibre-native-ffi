package org.maplibre.nativeffi.runtime

import kotlin.jvm.JvmInline

/**
 * Camera change kind reported by camera will-change and did-change events.
 *
 * [RuntimeEvent.code] carries this value for [RuntimeEventType.MAP_CAMERA_WILL_CHANGE] and
 * [RuntimeEventType.MAP_CAMERA_DID_CHANGE]. Construct one from that code to read it:
 * `CameraChangeMode(event.code)`. A native value this binding does not name yet round-trips through
 * [nativeValue].
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
