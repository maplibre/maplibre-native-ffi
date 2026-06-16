package org.maplibre.nativejni.resource;

/** Resource loading method copied from a native resource request. */
public final class ResourceLoadingMethod {
  public static final ResourceLoadingMethod ALL = new ResourceLoadingMethod(0, "ALL");
  public static final ResourceLoadingMethod CACHE_ONLY = new ResourceLoadingMethod(1, "CACHE_ONLY");
  public static final ResourceLoadingMethod NETWORK_ONLY =
      new ResourceLoadingMethod(2, "NETWORK_ONLY");

  private final int nativeValue;
  private final String name;

  private ResourceLoadingMethod(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static ResourceLoadingMethod fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> ALL;
      case 1 -> CACHE_ONLY;
      case 2 -> NETWORK_ONLY;
      default -> new ResourceLoadingMethod(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof ResourceLoadingMethod value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "ResourceLoadingMethod(" + nativeValue + ")";
  }
}
