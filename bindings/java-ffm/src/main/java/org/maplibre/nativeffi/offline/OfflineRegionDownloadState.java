package org.maplibre.nativeffi.offline;

import org.maplibre.nativeffi.internal.c.MapLibreNativeC;

/** Download state for an offline region status snapshot. */
public final class OfflineRegionDownloadState {
  public static final OfflineRegionDownloadState INACTIVE =
      new OfflineRegionDownloadState(MapLibreNativeC.MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE());
  public static final OfflineRegionDownloadState ACTIVE =
      new OfflineRegionDownloadState(MapLibreNativeC.MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE());

  private final int rawValue;
  private final String name;

  public OfflineRegionDownloadState(int rawValue) {
    this.rawValue = rawValue;
    if (rawValue == MapLibreNativeC.MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE()) {
      name = "INACTIVE";
    } else if (rawValue == MapLibreNativeC.MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE()) {
      name = "ACTIVE";
    } else {
      name = "UNKNOWN(" + rawValue + ")";
    }
  }

  public int rawValue() {
    return rawValue;
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof OfflineRegionDownloadState that && rawValue == that.rawValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(rawValue);
  }

  @Override
  public String toString() {
    return name;
  }
}
