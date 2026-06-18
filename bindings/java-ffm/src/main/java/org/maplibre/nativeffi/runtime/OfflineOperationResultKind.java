package org.maplibre.nativeffi.runtime;

/** Offline database operation result kind reported by completion events. */
public final class OfflineOperationResultKind {
  public static final OfflineOperationResultKind NONE = new OfflineOperationResultKind(0, "NONE");
  public static final OfflineOperationResultKind REGION =
      new OfflineOperationResultKind(1, "REGION");
  public static final OfflineOperationResultKind OPTIONAL_REGION =
      new OfflineOperationResultKind(2, "OPTIONAL_REGION");
  public static final OfflineOperationResultKind REGION_LIST =
      new OfflineOperationResultKind(3, "REGION_LIST");
  public static final OfflineOperationResultKind REGION_STATUS =
      new OfflineOperationResultKind(4, "REGION_STATUS");

  private final int nativeValue;
  private final String name;

  private OfflineOperationResultKind(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static OfflineOperationResultKind fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> NONE;
      case 1 -> REGION;
      case 2 -> OPTIONAL_REGION;
      case 3 -> REGION_LIST;
      case 4 -> REGION_STATUS;
      default -> new OfflineOperationResultKind(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof OfflineOperationResultKind value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "OfflineOperationResultKind(" + nativeValue + ")";
  }
}
