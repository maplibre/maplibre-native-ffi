package org.maplibre.nativejni.internal.callback;

import java.nio.charset.StandardCharsets;
import java.util.Objects;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.resource.ResourceKind;
import org.maplibre.nativejni.resource.ResourceTransformCallback;
import org.maplibre.nativejni.resource.ResourceTransformRequest;

/** Owns runtime-scoped resource transform callback state. */
public final class ResourceTransformState implements AutoCloseable {
  private final ResourceTransformCallback callback;
  private final MaplibreNativeC.mln_resource_transform_callback nativeCallback;
  private final MaplibreNativeC.mln_resource_transform transform;
  private final Object callbackLock = new Object();
  private final ThreadLocal<Integer> callbackDepth = ThreadLocal.withInitial(() -> 0);

  private int activeCallbacks;
  private boolean closeRequested;
  private boolean closed;

  public ResourceTransformState(ResourceTransformCallback callback) {
    this.callback = Objects.requireNonNull(callback, "callback");
    this.nativeCallback =
        new MaplibreNativeC.mln_resource_transform_callback() {
          @Override
          public int call(
              Pointer userData,
              int kind,
              BytePointer url,
              MaplibreNativeC.mln_resource_transform_response response) {
            if (!ResourceTransformState.this.enterCallback()) {
              response.url(null);
              return MaplibreNativeC.MLN_STATUS_OK;
            }
            try {
              var transformed =
                  ResourceTransformState.this.callback.transform(
                      new ResourceTransformRequest(
                          ResourceKind.fromNative(kind), JavaCppSupport.cString(url)));
              response.url(null);
              if (transformed.isPresent() && !transformed.get().isEmpty()) {
                if (transformed.get().indexOf('\0') >= 0) {
                  return MaplibreNativeC.MLN_STATUS_OK;
                }
                return setResponseUrl(response, transformed.get());
              }
              return MaplibreNativeC.MLN_STATUS_OK;
            } catch (Throwable exception) {
              response.url(null);
              return MaplibreNativeC.MLN_STATUS_NATIVE_ERROR;
            } finally {
              ResourceTransformState.this.exitCallback();
            }
          }
        };
    this.transform = new MaplibreNativeC.mln_resource_transform();
    transform.size(transform.sizeof());
    transform.callback(nativeCallback);
    transform.user_data(null);
  }

  public MaplibreNativeC.mln_resource_transform transform() {
    return transform;
  }

  public boolean isClosed() {
    synchronized (callbackLock) {
      return closed;
    }
  }

  private boolean enterCallback() {
    synchronized (callbackLock) {
      if (closeRequested || closed) {
        return false;
      }
      activeCallbacks++;
      callbackDepth.set(callbackDepth.get() + 1);
      return true;
    }
  }

  private void exitCallback() {
    synchronized (callbackLock) {
      var depth = callbackDepth.get() - 1;
      if (depth == 0) {
        callbackDepth.remove();
      } else {
        callbackDepth.set(depth);
      }
      activeCallbacks--;
      if (activeCallbacks == 0) {
        callbackLock.notifyAll();
      }
    }
  }

  @Override
  public void close() {
    var interrupted = false;
    var closeNative = false;
    synchronized (callbackLock) {
      if (callbackDepth.get() > 0) {
        throw Status.callbackReentry("Resource transform");
      }
      while (closeRequested && !closed) {
        try {
          callbackLock.wait();
        } catch (InterruptedException exception) {
          interrupted = true;
        }
      }
      if (closed) {
        if (interrupted) {
          Thread.currentThread().interrupt();
        }
        return;
      }
      closeRequested = true;
      while (activeCallbacks > 0) {
        try {
          callbackLock.wait();
        } catch (InterruptedException exception) {
          interrupted = true;
        }
      }
      closeNative = true;
    }
    try {
      if (closeNative) {
        transform.close();
        nativeCallback.close();
      }
    } finally {
      if (closeNative) {
        synchronized (callbackLock) {
          closed = true;
          callbackLock.notifyAll();
        }
      }
      if (interrupted) {
        Thread.currentThread().interrupt();
      }
    }
  }

  private static int setResponseUrl(
      MaplibreNativeC.mln_resource_transform_response response, String value) {
    var bytes = value.getBytes(StandardCharsets.UTF_8);
    try (var storage = new BytePointer(bytes.length)) {
      storage.put(bytes);
      return MaplibreNativeC.mln_resource_transform_response_set_url(
          response, storage, bytes.length);
    }
  }
}
