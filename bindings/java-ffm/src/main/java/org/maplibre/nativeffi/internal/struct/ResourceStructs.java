package org.maplibre.nativeffi.internal.struct;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.util.Optional;
import org.maplibre.nativeffi.internal.c.mln_resource_request;
import org.maplibre.nativeffi.internal.c.mln_resource_response;
import org.maplibre.nativeffi.internal.convert.NativeValues;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.resource.ResourceRequest;
import org.maplibre.nativeffi.resource.ResourceResponse;

/** Internal materializers and readers for resource callback structs. */
public final class ResourceStructs {
  private ResourceStructs() {}

  public static ResourceRequest resourceRequest(MemorySegment segment) {
    var rawKind = mln_resource_request.kind(segment);
    var rawLoadingMethod = mln_resource_request.loading_method(segment);
    var rawPriority = mln_resource_request.priority(segment);
    var rawUsage = mln_resource_request.usage(segment);
    var rawStoragePolicy = mln_resource_request.storage_policy(segment);
    return new ResourceRequest(
        MemoryUtil.copyCString(mln_resource_request.url(segment)),
        NativeValues.resourceKind(rawKind),
        rawKind,
        NativeValues.resourceLoadingMethod(rawLoadingMethod),
        rawLoadingMethod,
        NativeValues.resourcePriority(rawPriority),
        rawPriority,
        NativeValues.resourceUsage(rawUsage),
        rawUsage,
        NativeValues.resourceStoragePolicy(rawStoragePolicy),
        rawStoragePolicy,
        resourceRange(segment),
        optionalLong(
            mln_resource_request.has_prior_modified(segment),
            mln_resource_request.prior_modified_unix_ms(segment)),
        optionalLong(
            mln_resource_request.has_prior_expires(segment),
            mln_resource_request.prior_expires_unix_ms(segment)),
        optionalString(mln_resource_request.prior_etag(segment)),
        MemoryUtil.copyBytes(
            mln_resource_request.prior_data(segment),
            mln_resource_request.prior_data_size(segment)));
  }

  public static MemorySegment resourceResponse(ResourceResponse response, Arena arena) {
    var segment = mln_resource_response.allocate(arena);
    mln_resource_response.size(segment, (int) mln_resource_response.sizeof());
    mln_resource_response.status(segment, NativeValues.nativeValue(response.status()));
    mln_resource_response.error_reason(segment, NativeValues.nativeValue(response.errorReason()));
    var bytes = response.bytes();
    if (bytes.length > 0) {
      var nativeBytes = arena.allocate(bytes.length);
      MemorySegment.copy(bytes, 0, nativeBytes, ValueLayout.JAVA_BYTE, 0, bytes.length);
      mln_resource_response.bytes(segment, nativeBytes);
      mln_resource_response.byte_count(segment, bytes.length);
    }
    response
        .errorMessage()
        .ifPresent(
            value ->
                mln_resource_response.error_message(
                    segment, MemoryUtil.allocateCString(arena, value)));
    mln_resource_response.must_revalidate(segment, response.mustRevalidate());
    response
        .modifiedUnixMs()
        .ifPresent(
            value -> {
              mln_resource_response.has_modified(segment, true);
              mln_resource_response.modified_unix_ms(segment, value);
            });
    response
        .expiresUnixMs()
        .ifPresent(
            value -> {
              mln_resource_response.has_expires(segment, true);
              mln_resource_response.expires_unix_ms(segment, value);
            });
    response
        .etag()
        .ifPresent(
            value -> mln_resource_response.etag(segment, MemoryUtil.allocateCString(arena, value)));
    response
        .retryAfterUnixMs()
        .ifPresent(
            value -> {
              mln_resource_response.has_retry_after(segment, true);
              mln_resource_response.retry_after_unix_ms(segment, value);
            });
    return segment;
  }

  private static Optional<ResourceRequest.ByteRange> resourceRange(MemorySegment segment) {
    if (!mln_resource_request.has_range(segment)) {
      return Optional.empty();
    }
    return Optional.of(
        new ResourceRequest.ByteRange(
            mln_resource_request.range_start(segment), mln_resource_request.range_end(segment)));
  }

  private static Optional<Long> optionalLong(boolean present, long value) {
    return present ? Optional.of(value) : Optional.empty();
  }

  private static Optional<String> optionalString(MemorySegment value) {
    return MemoryUtil.isNull(value) ? Optional.empty() : Optional.of(MemoryUtil.copyCString(value));
  }
}
