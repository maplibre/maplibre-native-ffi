package org.maplibre.nativejni.runtime;

/** Ambient cache maintenance operation for a runtime. */
public enum AmbientCacheOperation {
  RESET_DATABASE(1),
  PACK_DATABASE(2),
  INVALIDATE(3),
  CLEAR(4);

  private final int nativeValue;

  AmbientCacheOperation(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }
}
