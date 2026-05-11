package org.maplibre.nativeffi.resource;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.lang.ref.Cleaner;
import java.util.Objects;
import java.util.function.Consumer;
import org.maplibre.nativeffi.error.InvalidStateException;
import org.maplibre.nativeffi.error.MaplibreStatus;
import org.maplibre.nativeffi.internal.access.InternalAccess;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.loader.NativeAccess;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.internal.status.Status;
import org.maplibre.nativeffi.internal.struct.ResourceStructs;

/**
 * Owned handle for a resource provider request that Java chose to handle.
 *
 * <p>Call {@link #complete(ResourceResponse)} to send a response, or {@link #close()} when a
 * handled request will not receive one. Successful completion releases the native provider
 * reference, so a completed handle rejects further use. Closing is harmless after completion.
 */
public final class ResourceRequestHandle implements AutoCloseable {
  private static final Cleaner CLEANER = Cleaner.create();

  private final MemorySegment handle;
  private final NativeReference nativeReference;
  private final Cleaner.Cleanable cleanable;
  private boolean decisionFinalized;
  private boolean closed;
  private boolean completed;

  public ResourceRequestHandle(InternalAccess access, MemorySegment handle) {
    this(handle);
    Objects.requireNonNull(access, "access");
  }

  ResourceRequestHandle(MemorySegment handle) {
    this(handle, MapLibreNativeC::mln_resource_request_release);
  }

  ResourceRequestHandle(MemorySegment handle, Consumer<MemorySegment> releaser) {
    this.handle = Objects.requireNonNull(handle, "handle");
    if (MemoryUtil.isNull(handle)) {
      throw new IllegalArgumentException("Resource request handle is null");
    }
    nativeReference = new NativeReference(handle, releaser);
    cleanable = CLEANER.register(this, nativeReference);
  }

  public synchronized void complete(ResourceResponse response) {
    NativeAccess.ensureLoaded();
    if (completed) {
      throw new InvalidStateException(
          MaplibreStatus.INVALID_STATE.nativeCode(), "ResourceRequestHandle is already completed");
    }
    requireLive();
    try (var arena = Arena.ofConfined()) {
      Status.check(
          MapLibreNativeC.mln_resource_request_complete(
              handle, ResourceStructs.resourceResponse(Objects.requireNonNull(response), arena)));
      completed = true;
      closed = true;
      if (decisionFinalized) {
        releaseNative();
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
    closed = true;
    if (decisionFinalized) {
      releaseNative();
    }
  }

  public synchronized int finishProviderDecision(
      InternalAccess access, ResourceProviderDecision decision) {
    Objects.requireNonNull(access, "access");
    return finishProviderDecision(decision);
  }

  synchronized int finishProviderDecision(ResourceProviderDecision decision) {
    // Completion before the callback returns makes Java the provider owner even if the callback
    // returns PASS_THROUGH. The native side must see HANDLE so Java can release its reference.
    if (completed
        || Objects.requireNonNull(decision, "decision") == ResourceProviderDecision.HANDLE) {
      decisionFinalized = true;
      nativeReference.markProviderOwned();
      if (closed) {
        releaseNative();
      }
      return ResourceProviderDecision.HANDLE.nativeValue();
    }
    markNativeWillRelease();
    return ResourceProviderDecision.PASS_THROUGH.nativeValue();
  }

  public synchronized int finishProviderException(InternalAccess access) {
    Objects.requireNonNull(access, "access");
    return finishProviderException();
  }

  synchronized int finishProviderException() {
    if (completed) {
      return finishProviderDecision(ResourceProviderDecision.HANDLE);
    }
    markNativeWillRelease();
    return -1;
  }

  private void markNativeWillRelease() {
    decisionFinalized = true;
    nativeReference.markNativeWillRelease();
    cleanable.clean();
    closed = true;
  }

  private void releaseNative() {
    nativeReference.releaseIfOwned();
    cleanable.clean();
    closed = true;
  }

  private void requireLive() {
    if (closed) {
      throw Status.released("ResourceRequestHandle");
    }
  }

  private static final class NativeReference implements Runnable {
    private final MemorySegment handle;
    private final Consumer<MemorySegment> releaser;
    private boolean providerOwned;
    private boolean releaseAccountedFor;

    NativeReference(MemorySegment handle, Consumer<MemorySegment> releaser) {
      this.handle = handle;
      this.releaser = Objects.requireNonNull(releaser, "releaser");
    }

    synchronized void markProviderOwned() {
      providerOwned = true;
    }

    synchronized void markNativeWillRelease() {
      releaseAccountedFor = true;
    }

    void releaseIfOwned() {
      var shouldRelease = false;
      synchronized (this) {
        if (!releaseAccountedFor) {
          releaseAccountedFor = true;
          shouldRelease = true;
        }
      }
      if (shouldRelease) {
        releaser.accept(handle);
      }
    }

    @Override
    public void run() {
      var shouldRelease = false;
      synchronized (this) {
        if (providerOwned && !releaseAccountedFor) {
          releaseAccountedFor = true;
          shouldRelease = true;
        }
      }
      if (shouldRelease) {
        releaser.accept(handle);
      }
    }
  }
}
