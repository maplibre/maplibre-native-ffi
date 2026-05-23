package org.maplibre.nativeffi.map

/** Mutable descriptor for axonometric map projection mode options. */
public class ProjectionModeOptions {
  public var axonometric: Boolean? = null
    private set

  public var xSkew: Double? = null
    private set

  public var ySkew: Double? = null
    private set

  public fun hasAxonometric(): Boolean = axonometric != null

  public fun axonometric(axonometric: Boolean): ProjectionModeOptions = apply {
    this.axonometric = axonometric
  }

  public fun clearAxonometric(): ProjectionModeOptions = apply { axonometric = null }

  public fun hasXSkew(): Boolean = xSkew != null

  public fun xSkew(xSkew: Double): ProjectionModeOptions = apply { this.xSkew = xSkew }

  public fun clearXSkew(): ProjectionModeOptions = apply { xSkew = null }

  public fun hasYSkew(): Boolean = ySkew != null

  public fun ySkew(ySkew: Double): ProjectionModeOptions = apply { this.ySkew = ySkew }

  public fun clearYSkew(): ProjectionModeOptions = apply { ySkew = null }
}
