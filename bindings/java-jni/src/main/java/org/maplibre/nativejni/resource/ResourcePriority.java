package org.maplibre.nativejni.resource;

/** Resource request priority copied from a native resource request. */
public final class ResourcePriority {
  public static final ResourcePriority REGULAR = new ResourcePriority(0, "REGULAR");
  public static final ResourcePriority LOW = new ResourcePriority(1, "LOW");

  private final int nativeValue;
  private final String name;

  private ResourcePriority(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static ResourcePriority fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> REGULAR;
      case 1 -> LOW;
      default -> new ResourcePriority(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof ResourcePriority value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "ResourcePriority(" + nativeValue + ")";
  }
}
