package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.EdgeInsets

/**
 * Mutable descriptor for live map viewport and render-transform controls.
 *
 * Compares and hashes by field value; [copy] returns an independent instance. Keep an instance
 * unmodified while it is a key in a hash-based collection.
 */
public class ViewportOptions {
  public var northOrientation: NorthOrientation? = null

  public var constrainMode: ConstrainMode? = null

  public var viewportMode: ViewportMode? = null

  public var frustumOffset: EdgeInsets? = null

  /** Returns an independent copy of this descriptor with [block] applied to the copy. */
  public fun copy(block: ViewportOptions.() -> Unit = {}): ViewportOptions =
    ViewportOptions()
      .also {
        it.northOrientation = northOrientation
        it.constrainMode = constrainMode
        it.viewportMode = viewportMode
        it.frustumOffset = frustumOffset
      }
      .apply(block)

  private val fields: List<Any?>
    get() = listOf(northOrientation, constrainMode, viewportMode, frustumOffset)

  override fun equals(other: Any?): Boolean = other is ViewportOptions && fields == other.fields

  override fun hashCode(): Int = fields.hashCode()
}
