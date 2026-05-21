package org.maplibre.nativeffi.runtime;

import org.maplibre.nativeffi.internal.c.MapLibreNativeC;

/** Offline database operation result kind reported by completion events. */
public enum OfflineOperationResultKind {
  NONE(MapLibreNativeC.MLN_OFFLINE_OPERATION_RESULT_NONE()),
  REGION(MapLibreNativeC.MLN_OFFLINE_OPERATION_RESULT_REGION()),
  OPTIONAL_REGION(MapLibreNativeC.MLN_OFFLINE_OPERATION_RESULT_OPTIONAL_REGION()),
  REGION_LIST(MapLibreNativeC.MLN_OFFLINE_OPERATION_RESULT_REGION_LIST()),
  REGION_STATUS(MapLibreNativeC.MLN_OFFLINE_OPERATION_RESULT_REGION_STATUS()),
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
