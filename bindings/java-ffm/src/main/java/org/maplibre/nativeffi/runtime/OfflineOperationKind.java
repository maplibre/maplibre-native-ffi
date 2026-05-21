package org.maplibre.nativeffi.runtime;

import org.maplibre.nativeffi.internal.c.MapLibreNativeC;

/** Offline database operation kind reported by completion events. */
public enum OfflineOperationKind {
  AMBIENT_CACHE(MapLibreNativeC.MLN_OFFLINE_OPERATION_AMBIENT_CACHE()),
  REGION_CREATE(MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_CREATE()),
  REGION_GET(MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_GET()),
  REGIONS_LIST(MapLibreNativeC.MLN_OFFLINE_OPERATION_REGIONS_LIST()),
  REGIONS_MERGE_DATABASE(MapLibreNativeC.MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE()),
  REGION_UPDATE_METADATA(MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA()),
  REGION_GET_STATUS(MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_GET_STATUS()),
  REGION_SET_OBSERVED(MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_SET_OBSERVED()),
  REGION_SET_DOWNLOAD_STATE(MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_SET_DOWNLOAD_STATE()),
  REGION_INVALIDATE(MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_INVALIDATE()),
  REGION_DELETE(MapLibreNativeC.MLN_OFFLINE_OPERATION_REGION_DELETE()),
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
