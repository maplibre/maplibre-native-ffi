package org.maplibre.nativejni.runtime;

/** Offline database operation kind reported by completion events. */
public enum OfflineOperationKind {
  AMBIENT_CACHE(1),
  REGION_CREATE(2),
  REGION_GET(3),
  REGIONS_LIST(4),
  REGIONS_MERGE_DATABASE(5),
  REGION_UPDATE_METADATA(6),
  REGION_GET_STATUS(7),
  REGION_SET_OBSERVED(8),
  REGION_SET_DOWNLOAD_STATE(9),
  REGION_INVALIDATE(10),
  REGION_DELETE(11),
  UNKNOWN(-1);

  private final int nativeValue;

  OfflineOperationKind(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static OfflineOperationKind fromNative(int nativeValue) {
    for (var kind : values()) {
      if (kind.nativeValue == nativeValue) {
        return kind;
      }
    }
    return UNKNOWN;
  }
}
