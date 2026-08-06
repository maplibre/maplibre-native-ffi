package org.maplibre.nativeffi.internal.wasm

import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.generated.MlnResourceRequest
import org.maplibre.nativeffi.internal.wasm.generated.MlnResourceResponse
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceLoadingMethod
import org.maplibre.nativeffi.resource.ResourcePriority
import org.maplibre.nativeffi.resource.ResourceRequest
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceStoragePolicy
import org.maplibre.nativeffi.resource.ResourceTransformRequest
import org.maplibre.nativeffi.resource.ResourceUsage

/**
 * Reads the resource descriptors native lends a callback, and places the one it takes back.
 *
 * Both directions are copies. A request and its strings are borrowed for the callback's duration
 * only, so everything a host keeps is copied into Kotlin before the callback returns; a response is
 * placed in one scratch block that outlives the call it is passed to and nothing longer.
 *
 * Every offset and width here comes from the generated accessors, so this code names fields.
 */
internal object ResourceMarshal {
  // A response descriptor carries `int64_t` timestamps, so the block it starts in is aligned for
  // them; its strings and bytes have no alignment of their own.
  private const val DESCRIPTOR_ALIGN = 8
  private const val BYTE_ALIGN = 1

  /**
   * Copies the request at [base] into Kotlin.
   *
   * Called on the thread the host lives on, while the thread that produced the request waits, so
   * the descriptor and everything it points at are still valid here.
   */
  fun readRequest(base: HeapPointer): ResourceRequest =
    ResourceRequest(
      requestedUrl = Heap.loadUtf8(MlnResourceRequest.requestedUrl(base)),
      resolvedUrl = Heap.loadUtf8(MlnResourceRequest.resolvedUrl(base)),
      kind = ResourceKind.fromNative(MlnResourceRequest.kind(base)),
      loadingMethod = ResourceLoadingMethod.fromNative(MlnResourceRequest.loadingMethod(base)),
      priority = ResourcePriority.fromNative(MlnResourceRequest.priority(base)),
      usage = ResourceUsage.fromNative(MlnResourceRequest.usage(base)),
      storagePolicy = ResourceStoragePolicy.fromNative(MlnResourceRequest.storagePolicy(base)),
      range =
        if (MlnResourceRequest.hasRange(base)) {
          ResourceRequest.ByteRange(
            MlnResourceRequest.rangeStart(base),
            MlnResourceRequest.rangeEnd(base),
          )
        } else {
          null
        },
      priorModifiedUnixMs =
        if (MlnResourceRequest.hasPriorModified(base)) {
          MlnResourceRequest.priorModifiedUnixMs(base)
        } else {
          null
        },
      priorExpiresUnixMs =
        if (MlnResourceRequest.hasPriorExpires(base)) {
          MlnResourceRequest.priorExpiresUnixMs(base)
        } else {
          null
        },
      // Null and empty mean different things here: no prior ETag at all, against one that is the
      // empty string. Reading the string would collapse them.
      priorEtag = optionalUtf8(MlnResourceRequest.priorEtag(base)),
      priorData =
        readBytes(MlnResourceRequest.priorData(base), MlnResourceRequest.priorDataSize(base)),
    )

  /** Copies the transform request native lends a URL transform callback. */
  fun readTransformRequest(kind: Int, url: HeapPointer): ResourceTransformRequest =
    ResourceTransformRequest(ResourceKind.fromNative(kind), Heap.loadUtf8(url))

  /**
   * Places [response] in scratch, calls [body] with the descriptor, and releases the scratch.
   *
   * The descriptor points at bytes and strings placed in the same block, so the whole response is
   * one acquisition and one release however large its payload. Native copies everything it keeps
   * before the completion call returns, which is what lets the block go at the end of [body].
   *
   * Each of the response's copying properties is read exactly once, because reading one copies the
   * value it holds.
   */
  fun <T> withResponse(response: ResourceResponse, body: (HeapPointer) -> T): T {
    // An unknown reason came from a native value this binding does not recognise, so sending it
    // back would ask native to store a reason it never produced.
    Status.requireArgument(response.errorReason.isKnown) {
      "Unknown resource error reason cannot be used as input: ${response.errorReason.nativeValue}"
    }
    val bytes = response.bytes
    val errorMessage = checkedText(response.errorMessage, "error message")
    val etag = checkedText(response.etag, "ETag")

    var total = HeapArena.aligned(MlnResourceResponse.SIZEOF.toLong(), DESCRIPTOR_ALIGN)
    total = plus(total, Heap.sizeOf(Byte.SIZE_BYTES, bytes.size).toLong())
    errorMessage?.let { total = plus(total, Heap.utf8Size(it).toLong()) }
    etag?.let { total = plus(total, Heap.utf8Size(it).toLong()) }

    return Heap.withScratch(total.toInt()) { scratch ->
      val arena = HeapArena(scratch, total.toInt())
      val base = arena.allocate(MlnResourceResponse.SIZEOF, DESCRIPTOR_ALIGN)
      // The leading size field is how the C API versions a descriptor: it carries the size this
      // binding was generated against so native can tell which fields it may read.
      MlnResourceResponse.setSize(base, MlnResourceResponse.SIZEOF)
      MlnResourceResponse.setStatus(base, response.status.nativeValue)
      MlnResourceResponse.setErrorReason(base, response.errorReason.nativeValue)
      if (bytes.isNotEmpty()) {
        val payload = arena.allocate(Heap.sizeOf(Byte.SIZE_BYTES, bytes.size), BYTE_ALIGN)
        Heap.storeBytes(payload, bytes)
        MlnResourceResponse.setBytes(base, payload)
        MlnResourceResponse.setByteCount(base, bytes.size)
      }
      errorMessage?.let { MlnResourceResponse.setErrorMessage(base, writeText(arena, it)) }
      MlnResourceResponse.setMustRevalidate(base, response.mustRevalidate)
      // An absent timestamp is a flag left clear rather than a sentinel written into the value.
      response.modifiedUnixMs?.let {
        MlnResourceResponse.setHasModified(base, true)
        MlnResourceResponse.setModifiedUnixMs(base, it)
      }
      response.expiresUnixMs?.let {
        MlnResourceResponse.setHasExpires(base, true)
        MlnResourceResponse.setExpiresUnixMs(base, it)
      }
      etag?.let { MlnResourceResponse.setEtag(base, writeText(arena, it)) }
      response.retryAfterUnixMs?.let {
        MlnResourceResponse.setHasRetryAfter(base, true)
        MlnResourceResponse.setRetryAfterUnixMs(base, it)
      }
      body(base)
    }
  }

  private fun optionalUtf8(pointer: HeapPointer): String? =
    if (pointer.address == 0) null else Heap.loadUtf8(pointer)

  private fun readBytes(pointer: HeapPointer, byteCount: Int): ByteArray =
    if (pointer.address == 0) ByteArray(0) else Heap.loadBytes(pointer, byteCount)

  /** Refuses text a C string cannot carry, before it is measured against the block it goes in. */
  private fun checkedText(value: String?, description: String): String? {
    value ?: return null
    Status.requireArgument('\u0000' !in value) { "$description contains embedded NUL" }
    return value
  }

  private fun writeText(arena: HeapArena, value: String): HeapPointer {
    val pointer = arena.allocate(Heap.utf8Size(value), BYTE_ALIGN)
    Heap.storeUtf8(pointer, value)
    return pointer
  }

  /** Adds two measured sizes, refusing a total this target could not address. */
  private fun plus(left: Long, right: Long): Long {
    val total = left + right
    if (total > Int.MAX_VALUE || total < 0) {
      throw Status.invalidArgument("the response is too large to place in the module's heap")
    }
    return total
  }
}
