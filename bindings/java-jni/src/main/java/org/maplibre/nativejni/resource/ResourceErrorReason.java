package org.maplibre.nativejni.resource;

/** Native resource error reason copied from events or resource responses. */
public enum ResourceErrorReason {
  NONE(0),
  NOT_FOUND(1),
  SERVER(2),
  CONNECTION(3),
  RATE_LIMIT(4),
  OTHER(5),
  UNKNOWN(-1);

  private final int nativeValue;

  ResourceErrorReason(int nativeValue) {
    this.nativeValue = nativeValue;
  }

  public int nativeValue() {
    return nativeValue;
  }

  public static ResourceErrorReason fromNative(int nativeValue) {
    for (var reason : values()) {
      if (reason.nativeValue == nativeValue) {
        return reason;
      }
    }
    return UNKNOWN;
  }
}
