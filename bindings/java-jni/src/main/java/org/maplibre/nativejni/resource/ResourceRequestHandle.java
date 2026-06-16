package org.maplibre.nativejni.resource;

import java.lang.ref.Cleaner;
import java.util.Objects;
import java.util.function.Consumer;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.error.MaplibreException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.internal.struct.ResourceStructs;

/**
 * Owned handle for a resource provider request that Java chose to handle.
 *
 * <p>Call {@link #complete(ResourceResponse)} to send a response, or {@link #close()} when a
 * handled request will not receive one. Successful completion releases the native provider
 * reference, so a completed handle rejects further use. Closing is harmless after completion.
 */
public final class ResourceRequestHandle implements AutoCloseable {
  private static final Cleaner CLEANER = Cleaner.create();

  private final long handle;
  private final LongCompleter completer;
  private final NativeReference nativeReference;
  private final Cleaner.Cleanable cleanable;
  private boolean decisionFinalized;
  private boolean closed;
  private boolean completed;

  ResourceRequestHandle(long handle, Consumer<Long> releaser) {
    this(handle, nativeCompleter(), consumerReleaser(releaser));
  }

  ResourceRequestHandle(long handle, LongCompleter completer, Consumer<Long> releaser) {
    this(handle, completer, consumerReleaser(releaser));
  }

  public ResourceRequestHandle(InternalAccess access, long handle) {
    this(internalHandle(access, handle));
  }

  ResourceRequestHandle(long handle) {
    this(
        handle,
        (LongReleaser)
            address ->
                MaplibreNativeC.mln_resource_request_release(
                    JavaCppSupport.resourceRequestHandle(address)));
  }

  private ResourceRequestHandle(long handle, LongReleaser releaser) {
    this(handle, nativeCompleter(), releaser);
  }

  private ResourceRequestHandle(long handle, LongCompleter completer, LongReleaser releaser) {
    if (handle == 0) {
      throw new IllegalArgumentException("Resource request handle is null");
    }
    this.handle = handle;
    this.completer = Objects.requireNonNull(completer, "completer");
    nativeReference = new NativeReference(handle, releaser);
    cleanable = CLEANER.register(this, nativeReference);
  }

  public synchronized void complete(ResourceResponse response) {
    if (completed) {
      throw new InvalidStateException(
          MaplibreStatus.INVALID_STATE.nativeCode(), "ResourceRequestHandle is already completed");
    }
    requireLive();
    try (var nativeResponse =
        ResourceStructs.nativeResourceResponse(Objects.requireNonNull(response))) {
      var status = completer.complete(handle, nativeResponse.response());
      MaplibreException completionFailure = null;
      try {
        Status.check(status);
      } catch (MaplibreException error) {
        completionFailure = error;
      }
      // The C API treats a completion attempt that reaches native as terminal:
      // cancelled/already-completed requests and actor delivery failures reject
      // the response but leave the provider responsible for releasing its handle.
      completed = true;
      closed = true;
      if (decisionFinalized) {
        releaseNative();
        JavaCppSupport.takeThreadDiagnostic();
      }
      if (completionFailure != null) {
        throw completionFailure;
      }
    }
  }

  public synchronized boolean isCancelled() {
    requireLive();
    var outCancelled = new boolean[1];
    Status.check(
        MaplibreNativeC.mln_resource_request_cancelled(
            JavaCppSupport.resourceRequestHandle(handle), outCancelled));
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
    Objects.requireNonNull(access, "access").checkCaller();
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
    Objects.requireNonNull(access, "access").checkCaller();
    return finishProviderException();
  }

  private static long internalHandle(InternalAccess access, long handle) {
    Objects.requireNonNull(access, "access").checkCaller(2);
    return handle;
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

  private static LongReleaser consumerReleaser(Consumer<Long> releaser) {
    Objects.requireNonNull(releaser, "releaser");
    return releaser::accept;
  }

  @FunctionalInterface
  interface LongCompleter {
    int complete(long address, MaplibreNativeC.mln_resource_response response);
  }

  private static LongCompleter nativeCompleter() {
    return (address, response) ->
        MaplibreNativeC.mln_resource_request_complete(
            JavaCppSupport.resourceRequestHandle(address), response);
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
