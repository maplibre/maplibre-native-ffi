package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.EdgeInsets

/** Mutable descriptor for live map viewport and render-transform controls. */
public class ViewportOptions {
  public var northOrientation: NorthOrientation? = null

  public var constrainMode: ConstrainMode? = null

  public var viewportMode: ViewportMode? = null

  public var frustumOffset: EdgeInsets? = null
}
