package org.maplibre.nativejni.map;

/** Mutable descriptor for axonometric map projection mode options. */
public final class ProjectionModeOptions {
  private Boolean axonometric;
  private Double xSkew;
  private Double ySkew;

  public boolean hasAxonometric() {
    return axonometric != null;
  }

  public Boolean axonometric() {
    return axonometric;
  }

  public ProjectionModeOptions axonometric(boolean axonometric) {
    this.axonometric = axonometric;
    return this;
  }

  public ProjectionModeOptions clearAxonometric() {
    axonometric = null;
    return this;
  }

  public boolean hasXSkew() {
    return xSkew != null;
  }

  public Double xSkew() {
    return xSkew;
  }

  public ProjectionModeOptions xSkew(double xSkew) {
    this.xSkew = xSkew;
    return this;
  }

  public ProjectionModeOptions clearXSkew() {
    xSkew = null;
    return this;
  }

  public boolean hasYSkew() {
    return ySkew != null;
  }

  public Double ySkew() {
    return ySkew;
  }

  public ProjectionModeOptions ySkew(double ySkew) {
    this.ySkew = ySkew;
    return this;
  }

  public ProjectionModeOptions clearYSkew() {
    ySkew = null;
    return this;
  }
}
