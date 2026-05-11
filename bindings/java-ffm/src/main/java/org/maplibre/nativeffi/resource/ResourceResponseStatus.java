package org.maplibre.nativeffi.resource;

/** Status for a resource provider response. */
public enum ResourceResponseStatus {
  OK(0),
  ERROR(1),
  NO_CONTENT(2),
  NOT_MODIFIED(3);

  private final int nativeValue;

  ResourceResponseStatus(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }
}
