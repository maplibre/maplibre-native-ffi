package org.maplibre.nativejni.internal.callback;

import org.maplibre.nativejni.internal.bridge.RuntimeNative;

/** Owns runtime-scoped resource transform callback state. */
public final class ResourceTransformState implements AutoCloseable {
  private final long address;
  private boolean closed;

  public ResourceTransformState(long address) {
    if (address == 0) {
      throw new IllegalArgumentException("address must be non-zero");
    }
    this.address = address;
  }

  public long address() {
    return address;
  }

  @Override
  public synchronized void close() {
    if (!closed) {
      closed = true;
      RuntimeNative.mln_resource_transform_state_destroy(address);
    }
  }
}
