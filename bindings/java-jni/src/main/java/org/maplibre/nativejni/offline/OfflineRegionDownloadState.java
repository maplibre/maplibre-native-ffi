package org.maplibre.nativejni.offline;

/** Download state for an offline region status snapshot. */
public enum OfflineRegionDownloadState {
  INACTIVE(0),
  ACTIVE(1),
  UNKNOWN(-1);

  private final int nativeValue;

  OfflineRegionDownloadState(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static OfflineRegionDownloadState fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> INACTIVE;
      case 1 -> ACTIVE;
      default -> UNKNOWN;
    };
  }
}
