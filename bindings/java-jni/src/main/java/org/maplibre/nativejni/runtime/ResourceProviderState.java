package org.maplibre.nativejni.runtime;

import java.lang.foreign.MemorySegment;
import org.maplibre.nativejni.resource.ResourceProviderCallback;

/** API-parity scaffold for the Java JNI binding. */
final class ResourceProviderState implements AutoCloseable {
  private static UnsupportedOperationException unsupported() {
    return new UnsupportedOperationException(
        "ResourceProviderState is not implemented by the JNI bridge yet");
  }

  ResourceProviderState(ResourceProviderCallback callback) {
    throw unsupported();
  }

  MemorySegment descriptor() {
    throw unsupported();
  }

  public void close() {
    throw unsupported();
  }
}
