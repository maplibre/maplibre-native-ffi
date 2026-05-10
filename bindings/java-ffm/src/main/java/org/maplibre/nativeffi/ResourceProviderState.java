package org.maplibre.nativeffi;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import org.maplibre.nativeffi.internal.ResourceStructs;
import org.maplibre.nativeffi.internal.c.mln_resource_provider;
import org.maplibre.nativeffi.internal.c.mln_resource_provider_callback;

/** Owns runtime-scoped resource provider callback state. */
final class ResourceProviderState implements AutoCloseable {
  static final int UNKNOWN_DECISION = -1;

  private final Arena arena;
  private final ResourceProviderCallback callback;
  private final MemorySegment stub;
  private final MemorySegment descriptor;

  ResourceProviderState(ResourceProviderCallback callback) {
    this.arena = Arena.ofShared();
    this.callback = callback;
    this.stub = mln_resource_provider_callback.allocate(this::invoke, arena);
    this.descriptor = mln_resource_provider.allocate(arena);
    mln_resource_provider.size(descriptor, (int) mln_resource_provider.sizeof());
    mln_resource_provider.callback(descriptor, stub);
    mln_resource_provider.user_data(descriptor, MemorySegment.NULL);
  }

  MemorySegment descriptor() {
    return descriptor;
  }

  private int invoke(MemorySegment userData, MemorySegment request, MemorySegment handle) {
    ResourceRequestHandle requestHandle = null;
    try {
      requestHandle = new ResourceRequestHandle(handle);
      var decision = callback.handle(ResourceStructs.resourceRequest(request), requestHandle);
      return requestHandle.finishProviderDecision(decision);
    } catch (Throwable ignored) {
      if (requestHandle != null) {
        return requestHandle.finishProviderException();
      }
      return UNKNOWN_DECISION;
    }
  }

  @Override
  public void close() {
    arena.close();
  }
}
