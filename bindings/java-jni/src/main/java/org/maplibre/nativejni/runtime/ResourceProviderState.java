package org.maplibre.nativejni.runtime;

import java.util.Objects;
import java.util.Optional;
import org.bytedeco.javacpp.Pointer;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.resource.ResourceKind;
import org.maplibre.nativejni.resource.ResourceLoadingMethod;
import org.maplibre.nativejni.resource.ResourcePriority;
import org.maplibre.nativejni.resource.ResourceProviderCallback;
import org.maplibre.nativejni.resource.ResourceProviderDecision;
import org.maplibre.nativejni.resource.ResourceRequest;
import org.maplibre.nativejni.resource.ResourceRequest.ByteRange;
import org.maplibre.nativejni.resource.ResourceRequestHandle;
import org.maplibre.nativejni.resource.ResourceStoragePolicy;
import org.maplibre.nativejni.resource.ResourceUsage;

/** Owns runtime-scoped resource provider callback state. */
final class ResourceProviderState implements AutoCloseable {
  private final ResourceProviderCallback callback;
  private final MaplibreNativeC.mln_resource_provider_callback nativeCallback;
  private final MaplibreNativeC.mln_resource_provider provider;
  private boolean closed;

  ResourceProviderState(ResourceProviderCallback callback) {
    this.callback = Objects.requireNonNull(callback, "callback");
    this.nativeCallback =
        new MaplibreNativeC.mln_resource_provider_callback() {
          @Override
          public int call(
              Pointer userData,
              MaplibreNativeC.mln_resource_request request,
              MaplibreNativeC.mln_resource_request_handle handle) {
            ResourceRequestHandle requestHandle = null;
            try {
              requestHandle = new ResourceRequestHandle(InternalAccess.INSTANCE, handle.address());
              var decision =
                  ResourceProviderState.this.callback.handle(
                      resourceRequest(request), requestHandle);
              return requestHandle.finishProviderDecision(
                  InternalAccess.INSTANCE,
                  decision == null ? ResourceProviderDecision.PASS_THROUGH : decision);
            } catch (Throwable exception) {
              return requestHandle == null
                  ? -1
                  : requestHandle.finishProviderException(InternalAccess.INSTANCE);
            }
          }
        };
    this.provider = new MaplibreNativeC.mln_resource_provider();
    provider.size(provider.sizeof());
    provider.callback(nativeCallback);
    provider.user_data(null);
  }

  MaplibreNativeC.mln_resource_provider provider() {
    return provider;
  }

  @Override
  public synchronized void close() {
    if (!closed) {
      closed = true;
      provider.close();
      nativeCallback.close();
    }
  }

  private static ResourceRequest resourceRequest(MaplibreNativeC.mln_resource_request request) {
    return new ResourceRequest(
        JavaCppSupport.cString(request.url()),
        ResourceKind.fromNative(request.kind()),
        request.kind(),
        ResourceLoadingMethod.fromNative(request.loading_method()),
        request.loading_method(),
        ResourcePriority.fromNative(request.priority()),
        request.priority(),
        ResourceUsage.fromNative(request.usage()),
        request.usage(),
        ResourceStoragePolicy.fromNative(request.storage_policy()),
        request.storage_policy(),
        request.has_range()
            ? Optional.of(new ByteRange(request.range_start(), request.range_end()))
            : Optional.empty(),
        request.has_prior_modified()
            ? Optional.of(request.prior_modified_unix_ms())
            : Optional.empty(),
        request.has_prior_expires()
            ? Optional.of(request.prior_expires_unix_ms())
            : Optional.empty(),
        request.prior_etag() == null || request.prior_etag().isNull()
            ? Optional.empty()
            : Optional.of(JavaCppSupport.cString(request.prior_etag())),
        priorData(request));
  }

  private static byte[] priorData(MaplibreNativeC.mln_resource_request request) {
    if (request.prior_data() == null
        || request.prior_data().isNull()
        || request.prior_data_size() == 0) {
      return new byte[0];
    }
    var bytes = new byte[Math.toIntExact(request.prior_data_size())];
    request.prior_data().get(bytes);
    return bytes;
  }
}
