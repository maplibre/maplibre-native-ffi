package org.maplibre.nativeffi.resource;

/** Resource storage policy copied from a native resource request. */
public final class ResourceStoragePolicy {
  public static final ResourceStoragePolicy PERMANENT = new ResourceStoragePolicy(0, "PERMANENT");
  public static final ResourceStoragePolicy VOLATILE = new ResourceStoragePolicy(1, "VOLATILE");

  private final int nativeValue;
  private final String name;

  private ResourceStoragePolicy(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static ResourceStoragePolicy fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> PERMANENT;
      case 1 -> VOLATILE;
      default -> new ResourceStoragePolicy(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof ResourceStoragePolicy value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "ResourceStoragePolicy(" + nativeValue + ")";
  }
}
