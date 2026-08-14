package org.maplibre.nativeffi.resource

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.NativeResourceRequest
import org.maplibre.nativeffi.internal.lifecycle.UnreachableActions
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status

/** Owned JVM FFM handle for a resource provider request. */
public actual class ResourceRequestHandle
internal constructor(
  private val handle: NativeResourceRequest,
  private val completer: (NativeResourceRequest, ResourceResponse) -> Int =
    NativeAccess::completeResourceRequest,
  private val cancellationChecker: (NativeResourceRequest) -> Boolean =
    NativeAccess::isResourceRequestCancelled,
  releaser: (NativeResourceRequest) -> Unit = NativeAccess::releaseResourceRequest,
) : AutoCloseable {
  private val core = ResourceRequestHandleCore(ReleaseNativeRequest(handle, releaser))

  init {
    UnreachableActions.register(this, CloseWhenUnreachableAction(core))
  }

  public actual fun complete(response: ResourceResponse) {
    NativeAccess.ensureLoaded()
    val operation = core.beginComplete()
    var reachedNative = false
    try {
      val nativeStatus = completer(handle, response).also { reachedNative = true }
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
    return core.withLiveHandle { cancellationChecker(handle) }
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
   * Request handles carry no thread affinity, so the cleanup thread may reclaim one. This holds the
   * ownership state alone; holding the wrapper would keep it reachable and suppress every reclaim.
   */
  private class CloseWhenUnreachableAction(private val core: ResourceRequestHandleCore) : Runnable {
    override fun run() {
      core.close()
    }
  }

  private class ReleaseNativeRequest(
    private val handle: NativeResourceRequest,
    private val releaser: (NativeResourceRequest) -> Unit,
  ) : () -> Unit {
    override fun invoke() {
      releaser(handle)
    }
  }

  private companion object {
    private const val UNKNOWN_DECISION: Int = -1
  }
}
