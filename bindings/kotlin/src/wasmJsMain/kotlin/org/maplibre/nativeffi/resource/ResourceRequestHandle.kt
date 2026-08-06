package org.maplibre.nativeffi.resource

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.NativeResourceRequest
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.ResourceMarshal
import org.maplibre.nativeffi.internal.wasm.generated.mln_resource_request_cancelled
import org.maplibre.nativeffi.internal.wasm.generated.mln_resource_request_complete
import org.maplibre.nativeffi.internal.wasm.generated.mln_resource_request_release

/**
 * Owned browser handle for a resource provider request.
 *
 * A request reaches host code from the ring drain, already claimed by the route that matched it, so
 * this handle owns the native request from the moment it is built. Kotlin/Wasm has no finalization:
 * a handle that is neither completed nor closed keeps MapLibre waiting for a response for as long
 * as the page lives.
 */
public actual class ResourceRequestHandle
private constructor(private val request: NativeResourceRequest) : AutoCloseable {
  private val core = ResourceRequestHandleCore { mln_resource_request_release(request.raw) }

  public actual fun complete(response: ResourceResponse) {
    val operation = core.beginComplete()
    var reachedNative = false
    try {
      val nativeStatus =
        ResourceMarshal.withResponse(response) { descriptor ->
          // Set once the call has come back, because acquiring the block the response is placed in
          // can fail on an exhausted heap. That is a completion that never reached C, and the
          // request is still the host's one chance to answer.
          mln_resource_request_complete(request.raw, descriptor.address).also {
            reachedNative = true
          }
        }
      val nativeFailure =
        if (nativeStatus == MaplibreStatus.OK.nativeCode) null else Status.exception(nativeStatus)
      // Marked completed whatever native answered, because a rejected completion has still used up
      // the request's one chance to be answered.
      operation.markCompleted()
      nativeFailure?.let { throw it }
    } catch (error: Throwable) {
      if (reachedNative) {
        operation.markCompleted()
      } else {
        operation.markNotReachedNative()
      }
      throw error
    } finally {
      operation.close()
    }
  }

  public actual fun isCancelled(): Boolean = core.withLiveHandle {
    Heap.withScratch(BOOLEAN_BYTES) { outCancelled ->
      Status.check(mln_resource_request_cancelled(request.raw, outCancelled.address))
      Heap.loadByte(outCancelled) != 0.toByte()
    }
  }

  public actual override fun close() {
    core.close()
  }

  internal companion object {
    /**
     * Wraps the request a queued provider route claimed.
     *
     * The route decided handled ownership before the request reached host code, so the native
     * request belongs to this wrapper and completing or closing it is what releases the request.
     */
    fun forQueuedRequest(request: NativeResourceRequest): ResourceRequestHandle =
      ResourceRequestHandle(request).also {
        it.core.finishProviderDecision(ResourceProviderDecision.HANDLE)
      }

    private const val BOOLEAN_BYTES: Int = 1
  }
}
