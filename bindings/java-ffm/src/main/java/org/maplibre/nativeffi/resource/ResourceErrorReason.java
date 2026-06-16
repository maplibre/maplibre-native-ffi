package org.maplibre.nativeffi.resource;

/** Native resource error reason copied from events or resource responses. */
public final class ResourceErrorReason {
  public static final ResourceErrorReason NONE = new ResourceErrorReason(0);
  public static final ResourceErrorReason NOT_FOUND = new ResourceErrorReason(1);
  public static final ResourceErrorReason SERVER = new ResourceErrorReason(2);
  public static final ResourceErrorReason CONNECTION = new ResourceErrorReason(3);
  public static final ResourceErrorReason RATE_LIMIT = new ResourceErrorReason(4);
  public static final ResourceErrorReason OTHER = new ResourceErrorReason(5);

  private final int rawValue;
  private final String name;

  public ResourceErrorReason(int rawValue) {
    this.rawValue = rawValue;
    this.name =
        switch (rawValue) {
          case 0 -> "NONE";
          case 1 -> "NOT_FOUND";
          case 2 -> "SERVER";
          case 3 -> "CONNECTION";
          case 4 -> "RATE_LIMIT";
          case 5 -> "OTHER";
          default -> "UNKNOWN(" + rawValue + ")";
        };
  }

  public int rawValue() {
    return rawValue;
  }

  @Override
  public boolean equals(Object other) {
    return other instanceof ResourceErrorReason that && rawValue == that.rawValue;
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
