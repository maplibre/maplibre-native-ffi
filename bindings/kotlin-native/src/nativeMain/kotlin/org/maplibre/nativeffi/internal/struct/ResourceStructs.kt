package org.maplibre.nativeffi.internal.struct

import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.alloc
import kotlinx.cinterop.ptr
import kotlinx.cinterop.readBytes
import kotlinx.cinterop.toCValues
import org.maplibre.nativeffi.internal.c.mln_resource_request
import org.maplibre.nativeffi.internal.c.mln_resource_response
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceLoadingMethod
import org.maplibre.nativeffi.resource.ResourcePriority
import org.maplibre.nativeffi.resource.ResourceRequest
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceStoragePolicy
import org.maplibre.nativeffi.resource.ResourceUsage

/** Internal materializers and readers for resource callback structs. */
@OptIn(ExperimentalForeignApi::class, ExperimentalUnsignedTypes::class)
internal object ResourceStructs {
  fun resourceRequest(value: mln_resource_request): ResourceRequest {
    val rawKind = value.kind
    val rawLoadingMethod = value.loading_method
    val rawPriority = value.priority
    val rawUsage = value.usage
    val rawStoragePolicy = value.storage_policy
    return ResourceRequest(
      MemoryUtil.copyCString(value.url),
      ResourceKind.fromNative(rawKind),
      rawKind,
      ResourceLoadingMethod.fromNative(rawLoadingMethod),
      rawLoadingMethod,
      ResourcePriority.fromNative(rawPriority),
      rawPriority,
      ResourceUsage.fromNative(rawUsage),
      rawUsage,
      ResourceStoragePolicy.fromNative(rawStoragePolicy),
      rawStoragePolicy,
      if (value.has_range) ResourceRequest.ByteRange(value.range_start, value.range_end) else null,
      if (value.has_prior_modified) value.prior_modified_unix_ms else null,
      if (value.has_prior_expires) value.prior_expires_unix_ms else null,
      optionalCString(value.prior_etag),
      value.prior_data?.readBytes(value.prior_data_size.toInt()) ?: ByteArray(0),
    )
  }

  fun resourceResponse(value: ResourceResponse, scope: MemScope): CPointer<mln_resource_response> {
    val native = scope.alloc<mln_resource_response>()
    native.size = kotlinx.cinterop.sizeOf<mln_resource_response>().toUInt()
    native.status = value.status.nativeValue
    native.error_reason = value.errorReason.nativeValue
    if (value.bytes.isNotEmpty()) {
      native.bytes = value.bytes.toUByteArray().toCValues().getPointer(scope)
      native.byte_count = value.bytes.size.toULong()
    }
    value.errorMessage?.let { native.error_message = MemoryUtil.cString(scope, it) }
    native.must_revalidate = value.mustRevalidate
    value.modifiedUnixMs?.let {
      native.has_modified = true
      native.modified_unix_ms = it
    }
    value.expiresUnixMs?.let {
      native.has_expires = true
      native.expires_unix_ms = it
    }
    value.etag?.let { native.etag = MemoryUtil.cString(scope, it) }
    value.retryAfterUnixMs?.let {
      native.has_retry_after = true
      native.retry_after_unix_ms = it
    }
    return native.ptr
  }

  private fun optionalCString(value: CPointer<ByteVar>?): String? =
    if (value == null) null else MemoryUtil.copyCString(value)
}
