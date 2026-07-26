package org.maplibre.nativeffi.camera

import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.Vec3

/**
 * Mutable free-camera descriptor.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class FreeCameraOptions {
  public var position: Vec3? = null

  public var orientation: Quaternion? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: FreeCameraOptions.() -> Unit = {}): FreeCameraOptions =
    FreeCameraOptions()
      .also {
        it.position = position
        it.orientation = orientation
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(position, orientation)

  override fun equals(other: Any?): Boolean = other is FreeCameraOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
