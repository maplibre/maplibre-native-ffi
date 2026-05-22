package org.maplibre.nativejni.map;

import java.lang.foreign.MemorySegment;
import org.maplibre.nativejni.style.CustomGeometrySourceOptions;

/** API-parity scaffold for the Java JNI binding. */
final class CustomGeometrySourceState implements AutoCloseable {
  private static UnsupportedOperationException unsupported() {
    return new UnsupportedOperationException(
        "CustomGeometrySourceState is not implemented by the JNI bridge yet");
  }

  CustomGeometrySourceState(CustomGeometrySourceOptions options) {
    throw unsupported();
  }

  MemorySegment descriptor() {
    throw unsupported();
  }

  public void close() {
    throw unsupported();
  }
}
