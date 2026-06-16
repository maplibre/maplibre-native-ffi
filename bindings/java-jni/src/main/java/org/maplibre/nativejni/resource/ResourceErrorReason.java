package org.maplibre.nativejni.resource;

/** Native resource error reason copied from events or resource responses. */
public final class ResourceErrorReason {
  public static final ResourceErrorReason NONE = new ResourceErrorReason(0, "NONE");
  public static final ResourceErrorReason NOT_FOUND = new ResourceErrorReason(1, "NOT_FOUND");
  public static final ResourceErrorReason SERVER = new ResourceErrorReason(2, "SERVER");
  public static final ResourceErrorReason CONNECTION = new ResourceErrorReason(3, "CONNECTION");
  public static final ResourceErrorReason RATE_LIMIT = new ResourceErrorReason(4, "RATE_LIMIT");
  public static final ResourceErrorReason OTHER = new ResourceErrorReason(5, "OTHER");

  private final int nativeValue;
  private final String name;

  private ResourceErrorReason(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int nativeValue() {
    if (name == null) {
      throw new IllegalArgumentException(
          "Unknown resource error reason cannot be used as an input");
    }
    return nativeValue;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static ResourceErrorReason fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> NONE;
      case 1 -> NOT_FOUND;
      case 2 -> SERVER;
      case 3 -> CONNECTION;
      case 4 -> RATE_LIMIT;
      case 5 -> OTHER;
      default -> new ResourceErrorReason(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof ResourceErrorReason value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "ResourceErrorReason(" + nativeValue + ")";
  }
}
