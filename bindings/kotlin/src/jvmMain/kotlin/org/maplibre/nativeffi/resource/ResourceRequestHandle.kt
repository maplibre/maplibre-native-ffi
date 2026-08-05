package org.maplibre.nativeffi.resource

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.NativeResourceRequest
import org.maplibre.nativeffi.internal.lifecycle.UnreachableActions
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status

/** Owned JVM FFM handle for a resource provider request. */
public actual class ResourceRequestHandle
internal constructor(private val handle: NativeResourceRequest) : AutoCloseable {
  private val core = ResourceRequestHandleCore(ReleaseNativeRequest(handle))

  init {
    UnreachableActions.register(this, CloseWhenUnreachableAction(core))
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
   * Request handles carry no owner-thread affinity, so the cleanup thread may reclaim one. This
   * holds the ownership state alone; holding the wrapper would keep it reachable and suppress every
   * reclaim.
   */
  private class CloseWhenUnreachableAction(private val core: ResourceRequestHandleCore) : Runnable {
    override fun run() {
      core.close()
    }
  }

  /** Native release that holds the handle segment alone, keeping the wrapper collectable. */
  private class ReleaseNativeRequest(private val handle: NativeResourceRequest) : () -> Unit {
    override fun invoke() {
      NativeAccess.releaseResourceRequest(handle)
    }
  }

  private companion object {
    private const val UNKNOWN_DECISION: Int = -1
  }
}
