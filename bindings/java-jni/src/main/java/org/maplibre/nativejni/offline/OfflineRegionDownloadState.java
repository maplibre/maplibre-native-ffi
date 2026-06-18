package org.maplibre.nativejni.offline;

/** Download state for an offline region status snapshot. */
public final class OfflineRegionDownloadState {
  public static final OfflineRegionDownloadState INACTIVE =
      new OfflineRegionDownloadState(0, "INACTIVE");
  public static final OfflineRegionDownloadState ACTIVE =
      new OfflineRegionDownloadState(1, "ACTIVE");

  private final int nativeValue;
  private final String name;

  private OfflineRegionDownloadState(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    if (name == null) {
      throw new IllegalArgumentException(
          "Unknown offline region download state cannot be used as an input");
    }
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static OfflineRegionDownloadState fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> INACTIVE;
      case 1 -> ACTIVE;
      default -> new OfflineRegionDownloadState(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof OfflineRegionDownloadState value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "OfflineRegionDownloadState(" + nativeValue + ")";
  }
}
