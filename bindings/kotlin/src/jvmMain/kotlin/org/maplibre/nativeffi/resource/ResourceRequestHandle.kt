package org.maplibre.nativeffi.resource

import java.lang.foreign.MemorySegment
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.UnreachableActions
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status

/** Owned JVM FFM handle for a resource provider request. */
public actual class ResourceRequestHandle internal constructor(private val handle: MemorySegment) :
  AutoCloseable {
  private val core = ResourceRequestHandleCore(ReleaseNativeRequest(handle))

  init {
    UnreachableActions.register(this, ReleaseIfOwnedAction(core))
  }

  public actual fun complete(response: ResourceResponse) {
    NativeAccess.ensureLoaded()
    val operation = core.beginComplete()
    var reachedNative = false
    try {
      val nativeStatus =
        NativeAccess.completeResourceRequest(handle, response).also { reachedNative = true }
      val nativeFailure =
        if (nativeStatus == MaplibreStatus.OK.nativeCode) null else Status.exception(nativeStatus)
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

  public actual fun isCancelled(): Boolean {
    NativeAccess.ensureLoaded()
    return core.withLiveHandle { NativeAccess.isResourceRequestCancelled(handle) }
  }

  public actual override fun close() {
    core.close()
  }

  internal fun finishProviderDecision(decision: ResourceProviderDecision): Int =
    core.finishProviderDecision(decision).nativeValue

  internal fun finishProviderException(): Int =
    core.finishProviderException()?.nativeValue ?: UNKNOWN_DECISION

  /**
   * Releases the native request once the wrapper becomes unreachable.
   *
   * Request handles carry no owner-thread affinity, so reclaiming one from the cleanup thread stays
   * within the cleanup-hook contract that keeps runtime, map, projection, and render-session
   * handles on their owner thread. [ResourceRequestHandleCore] releases only when the provider
   * still owns the request, so an explicit `close()` or completion keeps this a no-op.
   *
   * This holds the ownership state alone. Holding the wrapper would keep it reachable from the
   * cleanup registry and suppress every reclaim.
   */
  private class ReleaseIfOwnedAction(private val core: ResourceRequestHandleCore) : Runnable {
    override fun run() {
      core.releaseIfOwned()
    }
  }

  /** Native release that holds the handle segment alone, keeping the wrapper collectable. */
  private class ReleaseNativeRequest(private val handle: MemorySegment) : () -> Unit {
    override fun invoke() {
      NativeAccess.releaseResourceRequest(handle)
    }
  }

  private companion object {
    private const val UNKNOWN_DECISION: Int = -1
  }
}
