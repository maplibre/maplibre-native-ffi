package org.maplibre.nativejni.internal.struct;

import java.util.Objects;
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
}
