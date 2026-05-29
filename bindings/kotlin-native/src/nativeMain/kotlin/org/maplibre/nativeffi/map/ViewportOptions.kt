package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.EdgeInsets

/** Mutable descriptor for live map viewport and render-transform controls. */
public class ViewportOptions {
  public var northOrientation: NorthOrientation? = null
    private set

  public var constrainMode: ConstrainMode? = null
    private set

  public var viewportMode: ViewportMode? = null
    private set

  public var frustumOffset: EdgeInsets? = null
    private set

  public fun hasNorthOrientation(): Boolean = northOrientation != null

  public fun northOrientation(northOrientation: NorthOrientation): ViewportOptions = apply {
    this.northOrientation = northOrientation
  }

  public fun clearNorthOrientation(): ViewportOptions = apply { northOrientation = null }

  public fun hasConstrainMode(): Boolean = constrainMode != null

  public fun constrainMode(constrainMode: ConstrainMode): ViewportOptions = apply {
    this.constrainMode = constrainMode
  }

  public fun clearConstrainMode(): ViewportOptions = apply { constrainMode = null }

  public fun hasViewportMode(): Boolean = viewportMode != null

  public fun viewportMode(viewportMode: ViewportMode): ViewportOptions = apply {
    this.viewportMode = viewportMode
  }

  public fun clearViewportMode(): ViewportOptions = apply { viewportMode = null }

  public fun hasFrustumOffset(): Boolean = frustumOffset != null

  public fun frustumOffset(frustumOffset: EdgeInsets): ViewportOptions = apply {
    this.frustumOffset = frustumOffset
  }

  public fun clearFrustumOffset(): ViewportOptions = apply { frustumOffset = null }
}
