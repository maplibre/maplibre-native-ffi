package org.maplibre.nativeffi.offline;

/** Download state for an offline region status snapshot. */
public final class OfflineRegionDownloadState {
  public static final OfflineRegionDownloadState INACTIVE = new OfflineRegionDownloadState(0);
  public static final OfflineRegionDownloadState ACTIVE = new OfflineRegionDownloadState(1);

  private final int rawValue;
  private final String name;

  public OfflineRegionDownloadState(int rawValue) {
    this.rawValue = rawValue;
    this.name =
        switch (rawValue) {
          case 0 -> "INACTIVE";
          case 1 -> "ACTIVE";
          default -> "UNKNOWN(" + rawValue + ")";
        };
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
