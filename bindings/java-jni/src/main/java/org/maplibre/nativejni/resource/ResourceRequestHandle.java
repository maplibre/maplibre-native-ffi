package org.maplibre.nativejni.resource;

import java.lang.foreign.MemorySegment;
import java.util.function.Consumer;
import org.maplibre.nativejni.internal.access.InternalAccess;

/** API-parity scaffold for the Java JNI binding. */
public final class ResourceRequestHandle implements AutoCloseable {
  private static UnsupportedOperationException unsupported() {
    return new UnsupportedOperationException(
        "ResourceRequestHandle is not implemented by the JNI bridge yet");
  }

  public ResourceRequestHandle(InternalAccess access, MemorySegment handle) {
    throw unsupported();
  }

  ResourceRequestHandle(MemorySegment handle) {
    throw unsupported();
  }

  ResourceRequestHandle(MemorySegment handle, Consumer<MemorySegment> releaser) {
    throw unsupported();
  }

  public synchronized void complete(ResourceResponse response) {
    throw unsupported();
  }

  public synchronized boolean isCancelled() {
    throw unsupported();
  }

  public synchronized void close() {
    throw unsupported();
  }

  public synchronized int finishProviderDecision(
      InternalAccess access, ResourceProviderDecision decision) {
    throw unsupported();
  }

  synchronized int finishProviderDecision(ResourceProviderDecision decision) {
    throw unsupported();
  }

  public synchronized int finishProviderException(InternalAccess access) {
    throw unsupported();
  }

  synchronized int finishProviderException() {
    throw unsupported();
  }
}
