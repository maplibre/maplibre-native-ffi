package org.maplibre.nativejni.error;

/** Thrown when the loaded native library exposes an unsupported C ABI version. */
public final class AbiVersionMismatchException extends MaplibreException {
  private final int actualVersion;
  private final int expectedVersion;

  public AbiVersionMismatchException(int actualVersion, int expectedVersion) {
    super(
        MaplibreStatus.NATIVE_ERROR,
        0,
        "Unsupported Maplibre C ABI version %d; expected %d"
            .formatted(actualVersion, expectedVersion));
    this.actualVersion = actualVersion;
    this.expectedVersion = expectedVersion;
  }

  public int actualVersion() {
    return actualVersion;
  }

  public int expectedVersion() {
    return expectedVersion;
  }
}
