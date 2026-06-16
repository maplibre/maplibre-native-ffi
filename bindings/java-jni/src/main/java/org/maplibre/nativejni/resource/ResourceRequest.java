package org.maplibre.nativejni.resource;

import java.util.Optional;

/** Copied network resource request passed to a runtime resource provider. */
public record ResourceRequest(
    String url,
    ResourceKind kind,
    ResourceLoadingMethod loadingMethod,
    ResourcePriority priority,
    ResourceUsage usage,
    ResourceStoragePolicy storagePolicy,
    Optional<ByteRange> range,
    Optional<Long> priorModifiedUnixMs,
    Optional<Long> priorExpiresUnixMs,
    Optional<String> priorEtag,
    byte[] priorData) {
  public ResourceRequest {
    range = range == null ? Optional.empty() : range;
    priorModifiedUnixMs = priorModifiedUnixMs == null ? Optional.empty() : priorModifiedUnixMs;
    priorExpiresUnixMs = priorExpiresUnixMs == null ? Optional.empty() : priorExpiresUnixMs;
    priorEtag = priorEtag == null ? Optional.empty() : priorEtag;
    priorData = priorData == null ? new byte[0] : priorData.clone();
  }

  @Override
  public byte[] priorData() {
    return priorData.clone();
  }

  public record ByteRange(long start, long end) {}
}
