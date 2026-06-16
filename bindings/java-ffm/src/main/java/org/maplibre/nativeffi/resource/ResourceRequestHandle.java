package org.maplibre.nativeffi.resource;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.lang.ref.Cleaner;
import java.util.Objects;
import java.util.function.Consumer;
import org.maplibre.nativeffi.error.InvalidStateException;
import org.maplibre.nativeffi.error.MaplibreException;
import org.maplibre.nativeffi.error.MaplibreStatus;
import org.maplibre.nativeffi.internal.access.InternalAccess;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.convert.NativeValues;
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
  private final NativeComplete completer;
  private final NativeCancelled cancellationChecker;
  private final NativeReference nativeReference;
  private final Cleaner.Cleanable cleanable;
  private boolean decisionFinalized;
  private boolean closed;
  private boolean completed;

  ResourceRequestHandle(MemorySegment handle) {
    this(handle, MapLibreNativeC::mln_resource_request_release);
  }

  public ResourceRequestHandle(InternalAccess access, MemorySegment handle) {
    this(internalHandle(access, handle));
  }

  ResourceRequestHandle(MemorySegment handle, Consumer<MemorySegment> releaser) {
    this(handle, releaser, MapLibreNativeC::mln_resource_request_complete);
  }

  ResourceRequestHandle(
      MemorySegment handle, Consumer<MemorySegment> releaser, NativeComplete completer) {
    this(handle, releaser, completer, ResourceRequestHandle::nativeCancelled);
  }

  ResourceRequestHandle(
      MemorySegment handle,
      Consumer<MemorySegment> releaser,
      NativeComplete completer,
      NativeCancelled cancellationChecker) {
    this.handle = Objects.requireNonNull(handle, "handle");
    if (MemoryUtil.isNull(handle)) {
      throw new IllegalArgumentException("Resource request handle is null");
    }
    this.completer = Objects.requireNonNull(completer, "completer");
    this.cancellationChecker = Objects.requireNonNull(cancellationChecker, "cancellationChecker");
    nativeReference = new NativeReference(handle, releaser);
    cleanable = CLEANER.register(this, nativeReference);
  }

  public synchronized void complete(ResourceResponse response) {
    NativeAccess.ensureLoaded();
    if (completed) {
      throw new InvalidStateException(
          NativeValues.nativeCode(MaplibreStatus.INVALID_STATE),
          "ResourceRequestHandle is already completed");
    }
    requireLive();
    MaplibreException completionFailure = null;
    try {
      try (var arena = Arena.ofConfined()) {
        var nativeStatus =
            completer.complete(
                handle, ResourceStructs.resourceResponse(Objects.requireNonNull(response), arena));
        Status.check(nativeStatus);
      }
    } catch (MaplibreException error) {
      completionFailure = error;
    }
    completed = true;
    closed = true;
    if (decisionFinalized) {
      releaseNative();
    }
    if (completionFailure != null) {
      throw completionFailure;
    }
  }

  public synchronized boolean isCancelled() {
    NativeAccess.ensureLoaded();
    requireLive();
    return cancellationChecker.isCancelled(handle);
  }

  private static boolean nativeCancelled(MemorySegment handle) {
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
      return NativeValues.nativeValue(ResourceProviderDecision.HANDLE);
    }
    markNativeWillRelease();
    return NativeValues.nativeValue(ResourceProviderDecision.PASS_THROUGH);
  }

  public synchronized int finishProviderDecision(
      InternalAccess access, ResourceProviderDecision decision) {
    Objects.requireNonNull(access, "access").checkCaller();
    return finishProviderDecision(decision);
  }

  synchronized int finishProviderException() {
    if (completed) {
      return finishProviderDecision(ResourceProviderDecision.HANDLE);
    }
    markNativeWillRelease();
    return -1;
  }

  public synchronized int finishProviderException(InternalAccess access) {
    Objects.requireNonNull(access, "access").checkCaller();
    return finishProviderException();
  }

  private static MemorySegment internalHandle(InternalAccess access, MemorySegment handle) {
    Objects.requireNonNull(access, "access").checkCaller(2);
    return handle;
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

  @FunctionalInterface
  interface NativeComplete {
    int complete(MemorySegment handle, MemorySegment response);
  }

  @FunctionalInterface
  interface NativeCancelled {
    boolean isCancelled(MemorySegment handle);
  }
}
