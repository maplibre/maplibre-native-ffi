package org.maplibre.nativejni.map;

import org.maplibre.nativejni.internal.bridge.StyleNative;

/** Owns map/style-scoped custom geometry source callback state. */
final class CustomGeometrySourceState implements AutoCloseable {
  private final long address;
  private boolean closed;

  CustomGeometrySourceState(long address) {
    if (address == 0) {
      throw new IllegalArgumentException("address must be non-zero");
    }
    this.address = address;
  }

  long address() {
    return address;
  }

  @Override
  public void close() {
    if (!closed) {
      closed = true;
      StyleNative.mln_custom_geometry_source_state_destroy(address);
    }
  }
}
