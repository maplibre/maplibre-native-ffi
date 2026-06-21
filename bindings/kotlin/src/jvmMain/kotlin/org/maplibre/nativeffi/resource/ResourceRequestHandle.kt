package org.maplibre.nativeffi.resource

import java.lang.foreign.MemorySegment
import java.lang.ref.Cleaner
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status

/** Owned JVM FFM handle for a resource provider request. */
public actual class ResourceRequestHandle internal constructor(private val handle: MemorySegment) :
  AutoCloseable {
  private val core = ResourceRequestHandleCore { NativeAccess.releaseResourceRequest(handle) }
  @Suppress("unused")
  private val cleanable = CLEANER.register(this, Runnable { core.releaseIfOwned() })

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

  private companion object {
    private val CLEANER = Cleaner.create()
    private const val UNKNOWN_DECISION: Int = -1
  }
}
