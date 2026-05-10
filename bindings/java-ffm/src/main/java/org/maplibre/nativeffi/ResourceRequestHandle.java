package org.maplibre.nativeffi;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.util.Objects;
import org.maplibre.nativeffi.internal.MemoryUtil;
import org.maplibre.nativeffi.internal.NativeAccess;
import org.maplibre.nativeffi.internal.Status;
import org.maplibre.nativeffi.internal.Structs;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;

/**
 * Owned handle for a resource provider request that Java chose to handle.
 *
 * <p>Call {@link #complete(ResourceResponse)} to send a response, or {@link #close()} when a
 * handled request will not receive one. Successful completion releases the native provider
 * reference, so a completed handle rejects further use. Closing is harmless after completion.
 */
public final class ResourceRequestHandle implements AutoCloseable {
  private final MemorySegment handle;
  private boolean decisionFinalized;
  private boolean nativeReferenceConsumed;
  private boolean closed;
  private boolean completed;
  private boolean closeRequested;

  ResourceRequestHandle(MemorySegment handle) {
    this.handle = Objects.requireNonNull(handle, "handle");
    if (MemoryUtil.isNull(handle)) {
      throw new IllegalArgumentException("Resource request handle is null");
    }
  }

  public synchronized void complete(ResourceResponse response) {
    NativeAccess.ensureLoaded();
    if (completed) {
      throw new InvalidStateException(
          MapLibreStatus.INVALID_STATE.nativeCode(), "ResourceRequestHandle is already completed");
    }
    requireLive();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_resource_request_complete(
              handle, Structs.resourceResponse(Objects.requireNonNull(response), arena)));
      completed = true;
      closed = true;
      if (decisionFinalized) {
        releaseNative();
      } else {
        closeRequested = true;
      }
    }
  }

  public synchronized boolean isCancelled() {
    NativeAccess.ensureLoaded();
    requireLive();
    try (var arena = Arena.ofConfined()) {
      var outCancelled = arena.allocate(ValueLayout.JAVA_BOOLEAN);
      Status.check(MapLibreNativeC.mln_resource_request_cancelled(handle, outCancelled));
      return outCancelled.get(ValueLayout.JAVA_BOOLEAN, 0);
    }
  }

  @Override
  public synchronized void close() {
    if (closed) {
      return;
    }
    closeRequested = true;
    closed = true;
    if (decisionFinalized) {
      releaseNative();
    }
  }

  synchronized int finishProviderDecision(ResourceProviderDecision decision) {
    if (completed || decision == ResourceProviderDecision.HANDLE) {
      decisionFinalized = true;
      if (completed || closeRequested) {
        releaseNative();
      }
      return ResourceProviderDecision.HANDLE.nativeValue();
    }
    markNativeWillRelease();
    return ResourceProviderDecision.PASS_THROUGH.nativeValue();
  }

  synchronized int finishProviderException() {
    if (completed) {
      return finishProviderDecision(ResourceProviderDecision.HANDLE);
    }
    markNativeWillRelease();
    return ResourceProviderState.UNKNOWN_DECISION;
  }

  private void markNativeWillRelease() {
    decisionFinalized = true;
    nativeReferenceConsumed = true;
    closed = true;
  }

  private void releaseNative() {
    if (!nativeReferenceConsumed) {
      MapLibreNativeC.mln_resource_request_release(handle);
      nativeReferenceConsumed = true;
    }
    closed = true;
  }

  private void requireLive() {
    if (closed) {
      throw Status.released("ResourceRequestHandle");
    }
  }
}
