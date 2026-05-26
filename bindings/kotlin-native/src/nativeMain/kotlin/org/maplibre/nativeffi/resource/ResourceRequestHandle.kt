package org.maplibre.nativeffi.resource

import cnames.structs.mln_resource_request_handle
import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.value
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_resource_request_cancelled
import org.maplibre.nativeffi.internal.c.mln_resource_request_complete
import org.maplibre.nativeffi.internal.c.mln_resource_request_release
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.ResourceStructs

/** Owned handle for a resource provider request that Kotlin chose to handle. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class)
public class ResourceRequestHandle
internal constructor(
  private val handle: CPointer<mln_resource_request_handle>,
  private val releaser: (CPointer<mln_resource_request_handle>) -> Unit =
    ::mln_resource_request_release,
) : AutoCloseable {
  private val lock = AtomicInt(0)
  private var decisionFinalized = false
  private var closed = false
  private var completed = false
  private var releaseAccountedFor = false
  private var providerOwned = false

  public fun complete(response: ResourceResponse) {
    withLock {
      if (completed) {
        throw InvalidStateException(
          MaplibreStatus.INVALID_STATE.nativeCode,
          "ResourceRequestHandle is already completed",
        )
      }
      requireLive()
      memScoped {
        Status.check(
          mln_resource_request_complete(handle, ResourceStructs.resourceResponse(response, this))
        )
      }
      completed = true
      closed = true
      if (decisionFinalized) releaseNative()
    }
  }

  public fun isCancelled(): Boolean = withLock {
    requireLive()
    memScoped {
      val outCancelled = alloc<BooleanVar>()
      Status.check(mln_resource_request_cancelled(handle, outCancelled.ptr))
      outCancelled.value
    }
  }

  override fun close() {
    withLock {
      if (closed) return
      closed = true
      if (decisionFinalized) releaseNative()
    }
  }

  internal fun finishProviderDecision(decision: ResourceProviderDecision): UInt = withLock {
    finishProviderDecisionLocked(decision)
  }

  internal fun finishProviderException(): UInt = withLock {
    if (completed) return@withLock finishProviderDecisionLocked(ResourceProviderDecision.HANDLE)
    markNativeWillRelease()
    UInt.MAX_VALUE
  }

  private fun finishProviderDecisionLocked(decision: ResourceProviderDecision): UInt {
    return if (completed || decision == ResourceProviderDecision.HANDLE) {
      decisionFinalized = true
      providerOwned = true
      if (closed) releaseNative()
      ResourceProviderDecision.HANDLE.nativeValue.toUInt()
    } else {
      markNativeWillRelease()
      ResourceProviderDecision.PASS_THROUGH.nativeValue.toUInt()
    }
  }

  private fun markNativeWillRelease() {
    decisionFinalized = true
    releaseAccountedFor = true
    closed = true
  }

  private fun releaseNative() {
    if (providerOwned && !releaseAccountedFor) {
      releaseAccountedFor = true
      releaser(handle)
    }
    closed = true
  }

  private fun requireLive() {
    if (closed) throw Status.released("ResourceRequestHandle")
  }

  private inline fun <T> withLock(block: () -> T): T {
    while (!lock.compareAndSet(0, 1)) {
      // Spin briefly; native resource request callbacks may complete from any thread.
    }
    try {
      return block()
    } finally {
      lock.store(0)
    }
  }
}
