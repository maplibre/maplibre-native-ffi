package org.maplibre.nativeffi.resource;

/** Resource usage copied from a native resource request. */
public final class ResourceUsage {
  public static final ResourceUsage ONLINE = new ResourceUsage(0, "ONLINE");
  public static final ResourceUsage OFFLINE = new ResourceUsage(1, "OFFLINE");

  private final int nativeValue;
  private final String name;

  private ResourceUsage(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static ResourceUsage fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> ONLINE;
      case 1 -> OFFLINE;
      default -> new ResourceUsage(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof ResourceUsage value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "ResourceUsage(" + nativeValue + ")";
  }
}
