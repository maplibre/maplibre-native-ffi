package org.maplibre.nativeffi.runtime;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import org.maplibre.nativeffi.internal.access.InternalAccess;
import org.maplibre.nativeffi.internal.c.mln_resource_provider;
import org.maplibre.nativeffi.internal.c.mln_resource_provider_callback;
import org.maplibre.nativeffi.internal.callback.CallbackLifecycle;
import org.maplibre.nativeffi.internal.struct.ResourceStructs;
import org.maplibre.nativeffi.resource.ResourceProviderCallback;
import org.maplibre.nativeffi.resource.ResourceRequestHandle;

/** Owns runtime-scoped resource provider callback state. */
final class ResourceProviderState implements AutoCloseable {
  static final int UNKNOWN_DECISION = -1;

  private final Arena arena;
  private final CallbackLifecycle lifecycle = new CallbackLifecycle();
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

  boolean isCurrentThreadInCallback() {
    return lifecycle.isCurrentThreadInCallback();
  }

  private int invoke(MemorySegment userData, MemorySegment request, MemorySegment handle) {
    ResourceRequestHandle requestHandle = null;
    try {
      requestHandle = new ResourceRequestHandle(InternalAccess.INSTANCE, handle);
      var lease = lifecycle.enter();
      if (lease.isEmpty()) {
        return requestHandle.finishProviderException(InternalAccess.INSTANCE);
      }
      try (var ignored = lease.get()) {
        var decision = callback.handle(ResourceStructs.resourceRequest(request), requestHandle);
        return requestHandle.finishProviderDecision(InternalAccess.INSTANCE, decision);
      }
    } catch (Throwable ignored) {
      if (requestHandle != null) {
        return requestHandle.finishProviderException(InternalAccess.INSTANCE);
      }
      return UNKNOWN_DECISION;
    }
  }

  @Override
  public void close() {
    lifecycle.close("Resource provider", arena::close);
  }
}
