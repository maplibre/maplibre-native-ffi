package org.maplibre.nativeffi.camera

/** Mutable camera fitting descriptor. */
public class CameraFitOptions {
  public var padding: EdgeInsets? = null
    private set

  public var bearing: Double? = null
    private set

  public var pitch: Double? = null
    private set

  public fun hasPadding(): Boolean = padding != null

  public fun padding(padding: EdgeInsets): CameraFitOptions = apply { this.padding = padding }

  public fun clearPadding(): CameraFitOptions = apply { padding = null }

  public fun hasBearing(): Boolean = bearing != null

  public fun bearing(bearing: Double): CameraFitOptions = apply { this.bearing = bearing }

  public fun clearBearing(): CameraFitOptions = apply { bearing = null }

  public fun hasPitch(): Boolean = pitch != null

  public fun pitch(pitch: Double): CameraFitOptions = apply { this.pitch = pitch }

  public fun clearPitch(): CameraFitOptions = apply { pitch = null }
}
