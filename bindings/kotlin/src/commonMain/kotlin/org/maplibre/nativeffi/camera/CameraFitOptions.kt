package org.maplibre.nativeffi.camera

/**
 * Mutable camera fitting descriptor.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class CameraFitOptions {
  public var padding: EdgeInsets? = null

  public var bearing: Double? = null

  public var pitch: Double? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: CameraFitOptions.() -> Unit = {}): CameraFitOptions =
    CameraFitOptions()
      .also {
        it.padding = padding
        it.bearing = bearing
        it.pitch = pitch
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(padding, bearing, pitch)

  override fun equals(other: Any?): Boolean = other is CameraFitOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
