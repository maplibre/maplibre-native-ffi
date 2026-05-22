package org.maplibre.nativejni.resource;

import java.lang.foreign.MemorySegment;
import java.lang.ref.Cleaner;
import java.util.Objects;
import java.util.function.Consumer;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.bridge.RuntimeNative;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.internal.struct.ResourceStructs;

/** Owned handle for a resource provider request that Java chose to handle. */
public final class ResourceRequestHandle implements AutoCloseable {
  private static final Cleaner CLEANER = Cleaner.create();

  private final long handle;
  private final NativeReference nativeReference;
  private final Cleaner.Cleanable cleanable;
  private boolean decisionFinalized;
  private boolean closed;
  private boolean completed;

  public ResourceRequestHandle(InternalAccess access, MemorySegment handle) {
    this(address(handle));
    Objects.requireNonNull(access, "access");
  }

  ResourceRequestHandle(MemorySegment handle) {
    this(address(handle));
  }

  ResourceRequestHandle(MemorySegment handle, Consumer<MemorySegment> releaser) {
    this(address(handle), address -> releaser.accept(MemorySegment.ofAddress(address)));
    Objects.requireNonNull(releaser, "releaser");
  }

  ResourceRequestHandle(long handle) {
    this(handle, RuntimeNative::mln_resource_request_release);
  }

  private ResourceRequestHandle(long handle, LongReleaser releaser) {
    if (handle == 0) {
      throw new IllegalArgumentException("Resource request handle is null");
    }
    this.handle = handle;
    nativeReference = new NativeReference(handle, releaser);
    cleanable = CLEANER.register(this, nativeReference);
  }

  public synchronized void complete(ResourceResponse response) {
    if (completed) {
      throw new InvalidStateException(
          MaplibreStatus.INVALID_STATE.nativeCode(), "ResourceRequestHandle is already completed");
    }
    requireLive();
    Status.check(
        RuntimeNative.mln_resource_request_complete(
            handle, ResourceStructs.resourceResponse(Objects.requireNonNull(response))));
    completed = true;
    closed = true;
    if (decisionFinalized) {
      releaseNative();
    }
  }

  public synchronized boolean isCancelled() {
    requireLive();
    var outCancelled = new boolean[1];
    Status.check(RuntimeNative.mln_resource_request_cancelled(handle, outCancelled));
    return outCancelled[0];
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

  private static long address(MemorySegment handle) {
    return Objects.requireNonNull(handle, "handle").address();
  }

  @FunctionalInterface
  private interface LongReleaser {
    void release(long address);
  }

  private static final class NativeReference implements Runnable {
    private final long handle;
    private final LongReleaser releaser;
    private boolean providerOwned;
    private boolean releaseAccountedFor;

    NativeReference(long handle, LongReleaser releaser) {
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
        releaser.release(handle);
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
        releaser.release(handle);
      }
    }
  }
}
