package org.maplibre.nativejni.runtime;

/** Offline database operation kind reported by completion events. */
public final class OfflineOperationKind {
  public static final OfflineOperationKind AMBIENT_CACHE =
      new OfflineOperationKind(1, "AMBIENT_CACHE");
  public static final OfflineOperationKind REGION_CREATE =
      new OfflineOperationKind(2, "REGION_CREATE");
  public static final OfflineOperationKind REGION_GET = new OfflineOperationKind(3, "REGION_GET");
  public static final OfflineOperationKind REGIONS_LIST =
      new OfflineOperationKind(4, "REGIONS_LIST");
  public static final OfflineOperationKind REGIONS_MERGE_DATABASE =
      new OfflineOperationKind(5, "REGIONS_MERGE_DATABASE");
  public static final OfflineOperationKind REGION_UPDATE_METADATA =
      new OfflineOperationKind(6, "REGION_UPDATE_METADATA");
  public static final OfflineOperationKind REGION_GET_STATUS =
      new OfflineOperationKind(7, "REGION_GET_STATUS");
  public static final OfflineOperationKind REGION_SET_OBSERVED =
      new OfflineOperationKind(8, "REGION_SET_OBSERVED");
  public static final OfflineOperationKind REGION_SET_DOWNLOAD_STATE =
      new OfflineOperationKind(9, "REGION_SET_DOWNLOAD_STATE");
  public static final OfflineOperationKind REGION_INVALIDATE =
      new OfflineOperationKind(10, "REGION_INVALIDATE");
  public static final OfflineOperationKind REGION_DELETE =
      new OfflineOperationKind(11, "REGION_DELETE");

  private final int nativeValue;
  private final String name;

  private OfflineOperationKind(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static OfflineOperationKind fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 1 -> AMBIENT_CACHE;
      case 2 -> REGION_CREATE;
      case 3 -> REGION_GET;
      case 4 -> REGIONS_LIST;
      case 5 -> REGIONS_MERGE_DATABASE;
      case 6 -> REGION_UPDATE_METADATA;
      case 7 -> REGION_GET_STATUS;
      case 8 -> REGION_SET_OBSERVED;
      case 9 -> REGION_SET_DOWNLOAD_STATE;
      case 10 -> REGION_INVALIDATE;
      case 11 -> REGION_DELETE;
      default -> new OfflineOperationKind(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof OfflineOperationKind value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "OfflineOperationKind(" + nativeValue + ")";
  }
}
