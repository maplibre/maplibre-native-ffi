package org.maplibre.nativejni.runtime;

/** Source kind for a copied runtime event. */
public final class RuntimeEventSourceType {
  public static final RuntimeEventSourceType RUNTIME = new RuntimeEventSourceType(0, "RUNTIME");
  public static final RuntimeEventSourceType MAP = new RuntimeEventSourceType(1, "MAP");

  private final int nativeValue;
  private final String name;

  private RuntimeEventSourceType(int nativeValue, String name) {
    this.nativeValue = nativeValue;
    this.name = name;
  }

  public int rawValue() {
    return nativeValue;
  }

  public static RuntimeEventSourceType fromNative(int nativeValue) {
    return switch (nativeValue) {
      case 0 -> RUNTIME;
      case 1 -> MAP;
      default -> new RuntimeEventSourceType(nativeValue, null);
    };
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof RuntimeEventSourceType value && nativeValue == value.nativeValue;
  }

  @Override
  public int hashCode() {
    return Integer.hashCode(nativeValue);
  }

  @Override
  public String toString() {
    return name != null ? name : "RuntimeEventSourceType(" + nativeValue + ")";
  }
}
