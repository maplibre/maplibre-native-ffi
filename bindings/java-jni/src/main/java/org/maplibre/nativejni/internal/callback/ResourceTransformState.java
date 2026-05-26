package org.maplibre.nativejni.internal.callback;

import java.util.Objects;
import java.util.concurrent.ConcurrentHashMap;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.resource.ResourceKind;
import org.maplibre.nativejni.resource.ResourceTransformCallback;
import org.maplibre.nativejni.resource.ResourceTransformRequest;

/** Owns runtime-scoped resource transform callback state. */
public final class ResourceTransformState implements AutoCloseable {
  private final ResourceTransformCallback callback;
  private final ConcurrentHashMap<Thread, BytePointer> responseStorages = new ConcurrentHashMap<>();
  private final MaplibreNativeC.mln_resource_transform_callback nativeCallback;
  private final MaplibreNativeC.mln_resource_transform transform;
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
            try {
              closeCurrentResponseStorage();
              var transformed =
                  ResourceTransformState.this.callback.transform(
                      new ResourceTransformRequest(
                          ResourceKind.fromNative(kind), kind, JavaCppSupport.cString(url)));
              if (transformed.isPresent()) {
                var storage = JavaCppSupport.utf8(transformed.get());
                responseStorages.put(Thread.currentThread(), storage);
                response.url(storage);
              } else {
                response.url(null);
              }
              return MaplibreNativeC.MLN_STATUS_OK;
            } catch (Throwable exception) {
              response.url(null);
              return MaplibreNativeC.MLN_STATUS_NATIVE_ERROR;
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

  @Override
  public synchronized void close() {
    if (!closed) {
      closed = true;
      responseStorages.forEach(
          (thread, storage) -> {
            if (responseStorages.remove(thread, storage)) {
              storage.close();
            }
          });
      transform.close();
      nativeCallback.close();
    }
  }

  private void closeCurrentResponseStorage() {
    var storage = responseStorages.remove(Thread.currentThread());
    if (storage != null) {
      storage.close();
    }
  }
}
