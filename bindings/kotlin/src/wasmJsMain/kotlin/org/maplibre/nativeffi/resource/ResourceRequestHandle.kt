package org.maplibre.nativeffi.resource

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.NativeResourceRequest
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.BrowserModule
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.NativeCall
import org.maplibre.nativeffi.internal.wasm.ResourceMarshal

/**
 * Owned browser handle for a resource provider request.
 *
 * These three entry points run on whichever thread already holds the handle. The C API documents a
 * request handle as free of owner-thread affinity -- completion may come from any thread, and
 * cancellation may be read from any thread -- so placing them on the runtime's owner thread would
 * add a park and a resume to a call that needs neither. It is also what makes them usable from
 * inside a provider callback, which runs on a stack that cannot park at all.
 *
 * **A browser host closes its handles.** Kotlin/Wasm has no finalization, so the reclaim that the
 * JVM and Kotlin/Native bindings perform once a wrapper becomes unreachable has no equivalent here.
 * A handle that is neither completed nor closed keeps MapLibre waiting for a response for as long
 * as the page lives.
 */
public actual class ResourceRequestHandle
internal constructor(private val request: NativeResourceRequest) : AutoCloseable {
  private val core = ResourceRequestHandleCore { releaseRequest() }

  public actual fun complete(response: ResourceResponse) {
    val operation = core.beginComplete()
    var reachedNative = false
    try {
      val nativeStatus =
        ResourceMarshal.withResponse(response) { descriptor ->
          // Set once the call has come back rather than as the descriptor is handed to it. What
          // sits between the two is page-side work that native never sees: resolving the entry
          // point, and acquiring the block the arguments are packed into, which an exhausted heap
          // refuses. A failure there is a completion that never reached C, and the request is still
          // the host's one chance to answer -- so it has to stay answerable rather than be spent.
          NativeCall.call(
              "mln_resource_request_complete",
              COMPLETE_SLOTS,
              { slots ->
                slots.setLong(0, request.raw)
                slots.setPointer(1, descriptor)
              },
              { Heap.loadInt(it) },
            )
            .also { reachedNative = true }
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
      Status.check(
        NativeCall.call(
          "mln_resource_request_cancelled",
          CANCELLED_SLOTS,
          { slots ->
            slots.setLong(0, request.raw)
            slots.setPointer(1, outCancelled)
          },
          { Heap.loadInt(it) },
        )
      )
      Heap.loadByte(outCancelled) != 0.toByte()
    }
  }

  public actual override fun close() {
    core.close()
  }

  /** Reports the decision the provider callback returns, and releases a handle it gave up. */
  internal fun finishProviderDecision(decision: ResourceProviderDecision): Int =
    core.finishProviderDecision(decision).nativeValue

  /**
   * Reports the decision to use when the provider callback failed.
   *
   * The unknown value is what the C API turns into a provider error response, which is the outcome
   * for a request the host neither served nor passed through.
   */
  internal fun finishProviderException(): Int =
    core.finishProviderException()?.nativeValue ?: UNKNOWN_DECISION

  private fun releaseRequest() {
    // A request handle is not owner-affine and is deliberately not one of the handles a shutdown
    // refuses to leave open: releasing it needs no particular thread, and a shutdown released the
    // module the native request lived inside, so there is nothing left to release. Calling native
    // here would report the binding's own module-released failure from inside a close, which is
    // worse than doing nothing: the close core has already marked this handle released by the time
    // it runs, so the failure could not be acted on and would only break a `finally`.
    if (!BrowserModule.isLoaded()) return
    NativeCall.call(
      "mln_resource_request_release",
      RELEASE_SLOTS,
      { it.setLong(0, request.raw) },
      {},
    )
  }

  internal companion object {
    /** Wraps the request handle a provider callback was given. */
    fun fromNative(request: NativeResourceRequest): ResourceRequestHandle =
      ResourceRequestHandle(request)

    private const val COMPLETE_SLOTS: Int = 2
    private const val CANCELLED_SLOTS: Int = 2
    private const val RELEASE_SLOTS: Int = 1
    private const val BOOLEAN_BYTES: Int = 1
    private const val UNKNOWN_DECISION: Int = -1
  }
}
