package org.maplibre.nativejni.runtime;

import org.maplibre.nativejni.internal.bridge.RuntimeNative;

/** Owns runtime-scoped resource provider callback state. */
final class ResourceProviderState implements AutoCloseable {
  private final long address;
  private boolean closed;

  ResourceProviderState(long address) {
    if (address == 0) {
      throw new IllegalArgumentException("address must be non-zero");
    }
    this.address = address;
  }

  long address() {
    return address;
  }

  @Override
  public synchronized void close() {
    if (!closed) {
      closed = true;
      RuntimeNative.mln_resource_provider_state_destroy(address);
    }
  }
}
