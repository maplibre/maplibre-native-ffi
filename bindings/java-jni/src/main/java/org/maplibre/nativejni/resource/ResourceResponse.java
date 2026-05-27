package org.maplibre.nativejni.resource;

import java.util.Objects;
import java.util.Optional;

/** Mutable descriptor used to complete a resource provider request. */
public final class ResourceResponse {
  private final ResourceResponseStatus status;
  private ResourceErrorReason errorReason = ResourceErrorReason.NONE;
  private byte[] bytes = new byte[0];
  private String errorMessage;
  private boolean mustRevalidate;
  private Long modifiedUnixMs;
  private Long expiresUnixMs;
  private String etag;
  private Long retryAfterUnixMs;

  private ResourceResponse(ResourceResponseStatus status) {
    this.status = Objects.requireNonNull(status, "status");
  }

  public static ResourceResponse ok(byte[] bytes) {
    return new ResourceResponse(ResourceResponseStatus.OK).bytes(bytes);
  }

  public static ResourceResponse noContent() {
    return new ResourceResponse(ResourceResponseStatus.NO_CONTENT);
  }

  public static ResourceResponse notModified() {
    return new ResourceResponse(ResourceResponseStatus.NOT_MODIFIED);
  }

  public static ResourceResponse error(ResourceErrorReason reason, String message) {
    return new ResourceResponse(ResourceResponseStatus.ERROR)
        .errorReason(reason)
        .errorMessage(message);
  }

  public ResourceResponseStatus status() {
    return status;
  }

  public ResourceErrorReason errorReason() {
    return errorReason;
  }

  public ResourceResponse errorReason(ResourceErrorReason errorReason) {
    this.errorReason = Objects.requireNonNull(errorReason, "errorReason");
    return this;
  }

  public byte[] bytes() {
    return bytes.clone();
  }

  public ResourceResponse bytes(byte[] bytes) {
    this.bytes = bytes == null ? new byte[0] : bytes.clone();
    return this;
  }

  public Optional<String> errorMessage() {
    return Optional.ofNullable(errorMessage);
  }

  public ResourceResponse errorMessage(String errorMessage) {
    this.errorMessage = errorMessage;
    return this;
  }

  public boolean mustRevalidate() {
    return mustRevalidate;
  }

  public ResourceResponse mustRevalidate(boolean mustRevalidate) {
    this.mustRevalidate = mustRevalidate;
    return this;
  }

  public Optional<Long> modifiedUnixMs() {
    return Optional.ofNullable(modifiedUnixMs);
  }

  public ResourceResponse modifiedUnixMs(long modifiedUnixMs) {
    this.modifiedUnixMs = modifiedUnixMs;
    return this;
  }

  public Optional<Long> expiresUnixMs() {
    return Optional.ofNullable(expiresUnixMs);
  }

  public ResourceResponse expiresUnixMs(long expiresUnixMs) {
    this.expiresUnixMs = expiresUnixMs;
    return this;
  }

  public Optional<String> etag() {
    return Optional.ofNullable(etag);
  }

  public ResourceResponse etag(String etag) {
    this.etag = etag;
    return this;
  }

  public Optional<Long> retryAfterUnixMs() {
    return Optional.ofNullable(retryAfterUnixMs);
  }

  public ResourceResponse retryAfterUnixMs(long retryAfterUnixMs) {
    this.retryAfterUnixMs = retryAfterUnixMs;
    return this;
  }
}
