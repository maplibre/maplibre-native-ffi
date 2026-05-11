package org.maplibre.nativeffi.resource;

/** Decision returned by a resource provider callback. */
public enum ResourceProviderDecision {
  PASS_THROUGH(0),
  HANDLE(1);

  private final int nativeValue;

  ResourceProviderDecision(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }
}
