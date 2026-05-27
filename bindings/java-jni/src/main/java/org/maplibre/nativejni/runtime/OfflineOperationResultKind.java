package org.maplibre.nativejni.runtime;

/** Offline database operation result kind reported by completion events. */
public enum OfflineOperationResultKind {
  NONE(0),
  REGION(1),
  OPTIONAL_REGION(2),
  REGION_LIST(3),
  REGION_STATUS(4),
  UNKNOWN(-1);

  private final int nativeValue;

  OfflineOperationResultKind(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static OfflineOperationResultKind fromNative(int nativeValue) {
    for (var kind : values()) {
      if (kind.nativeValue == nativeValue) {
        return kind;
      }
    }
    return UNKNOWN;
  }
}
