package org.maplibre.nativeffi.resource

import cnames.structs.mln_resource_request_handle
import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner
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
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class, ExperimentalNativeApi::class)
public class ResourceRequestHandle
internal constructor(
  private val handle: CPointer<mln_resource_request_handle>,
  private val releaser: (CPointer<mln_resource_request_handle>) -> Unit =
    ::mln_resource_request_release,
) : AutoCloseable {
  private val lock = AtomicInt(0)
  private val nativeReference = NativeReference(handle, releaser)
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(nativeReference) { it.run() }
  private var decisionFinalized = false
  private var closed = false
  private var completed = false

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
      nativeReference.markProviderOwned()
      if (closed) releaseNative()
      ResourceProviderDecision.HANDLE.nativeValue.toUInt()
    } else {
      markNativeWillRelease()
      ResourceProviderDecision.PASS_THROUGH.nativeValue.toUInt()
    }
  }

  private fun markNativeWillRelease() {
    decisionFinalized = true
    nativeReference.markNativeWillRelease()
    closed = true
  }

  private fun releaseNative() {
    nativeReference.releaseIfOwned()
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

  private class NativeReference(
    private val handle: CPointer<mln_resource_request_handle>,
    private val releaser: (CPointer<mln_resource_request_handle>) -> Unit,
  ) {
    private val lock = AtomicInt(0)
    private var providerOwned = false
    private var releaseAccountedFor = false

    fun markProviderOwned() = withLock { providerOwned = true }

    fun markNativeWillRelease() = withLock { releaseAccountedFor = true }

    fun releaseIfOwned() = withLock {
      if (providerOwned && !releaseAccountedFor) {
        releaseAccountedFor = true
        releaser(handle)
      }
    }

    fun run() {
      releaseIfOwned()
    }

    private inline fun <T> withLock(block: () -> T): T {
      while (!lock.compareAndSet(0, 1)) {
        // Resource request cleanup may race with explicit close from another thread.
      }
      try {
        return block()
      } finally {
        lock.store(0)
      }
    }
  }
}
