package org.maplibre.nativejni.internal.struct;

import java.util.Objects;
import org.bytedeco.javacpp.BytePointer;
import org.bytedeco.javacpp.Pointer;
import org.maplibre.nativejni.internal.javacpp.JavaCppSupport;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.internal.status.Status;
import org.maplibre.nativejni.resource.ResourceResponse;

/** Internal materializers for resource request, response, and transform values. */
public final class ResourceStructs {
  private ResourceStructs() {}

  public record ResourceResponseValue(
      int status,
      int errorReason,
      byte[] bytes,
      String errorMessage,
      boolean mustRevalidate,
      Long modifiedUnixMs,
      Long expiresUnixMs,
      String etag,
      Long retryAfterUnixMs) {
    public ResourceResponseValue {
      bytes = bytes == null ? new byte[0] : bytes.clone();
    }

    @Override
    public byte[] bytes() {
      return bytes.clone();
    }
  }

  public static ResourceResponseValue resourceResponse(ResourceResponse response) {
    Objects.requireNonNull(response, "response");
    response.errorMessage().ifPresent(value -> Status.checkNoEmbeddedNul(value, "error message"));
    response.etag().ifPresent(value -> Status.checkNoEmbeddedNul(value, "ETag"));
    return new ResourceResponseValue(
        response.status().nativeValue(),
        response.errorReason().nativeValue(),
        response.bytes(),
        response.errorMessage().orElse(null),
        response.mustRevalidate(),
        response.modifiedUnixMs().orElse(null),
        response.expiresUnixMs().orElse(null),
        response.etag().orElse(null),
        response.retryAfterUnixMs().orElse(null));
  }

  public static NativeResourceResponseScope nativeResourceResponse(ResourceResponse response) {
    return new NativeResourceResponseScope(resourceResponse(response));
  }

  public static final class NativeResourceResponseScope implements AutoCloseable {
    private final MaplibreNativeC.mln_resource_response response;
    private final BytePointer bytes;
    private final BytePointer errorMessage;
    private final BytePointer etag;

    private NativeResourceResponseScope(ResourceResponseValue value) {
      response = new MaplibreNativeC.mln_resource_response();
      response.size(response.sizeof());
      response.status(value.status());
      response.error_reason(value.errorReason());
      var byteArray = value.bytes();
      bytes = new BytePointer(Math.max(byteArray.length, 1));
      if (byteArray.length > 0) {
        bytes.put(byteArray);
        response.bytes(bytes);
        response.byte_count(byteArray.length);
      }
      errorMessage = JavaCppSupport.utf8(value.errorMessage());
      etag = JavaCppSupport.utf8(value.etag());
      response.error_message(errorMessage);
      response.must_revalidate(value.mustRevalidate());
      if (value.modifiedUnixMs() != null) {
        response.has_modified(true);
        response.modified_unix_ms(value.modifiedUnixMs());
      }
      if (value.expiresUnixMs() != null) {
        response.has_expires(true);
        response.expires_unix_ms(value.expiresUnixMs());
      }
      response.etag(etag);
      if (value.retryAfterUnixMs() != null) {
        response.has_retry_after(true);
        response.retry_after_unix_ms(value.retryAfterUnixMs());
      }
    }

    public MaplibreNativeC.mln_resource_response response() {
      return response;
    }

    @Override
    public void close() {
      close(response);
      close(bytes);
      close(errorMessage);
      close(etag);
    }

    private static void close(Pointer pointer) {
      if (pointer != null) {
        pointer.close();
      }
    }
  }
}
