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
import org.maplibre.nativeffi.internal.c.mln_resource_response
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.ResourceStructs

/** Owned handle for a resource provider request that Kotlin chose to handle. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class, ExperimentalNativeApi::class)
public class ResourceRequestHandle
internal constructor(
  private val handle: CPointer<mln_resource_request_handle>,
  private val completer:
    (CPointer<mln_resource_request_handle>, CPointer<mln_resource_response>) -> Int =
    ::mln_resource_request_complete,
  private val cancellationChecker:
    (CPointer<mln_resource_request_handle>, CPointer<BooleanVar>) -> Int =
    { requestHandle, outCancelled ->
      mln_resource_request_cancelled(requestHandle, outCancelled)
    },
  private val releaser: (CPointer<mln_resource_request_handle>) -> Unit =
    ::mln_resource_request_release,
) : AutoCloseable {
  private val state = AtomicInt(0)
  private val completion = AtomicInt(COMPLETION_OPEN)
  private val decisionFinalized = AtomicInt(0)
  private val nativeReference = NativeReference(handle, releaser)
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(nativeReference) { it.run() }

  public fun complete(response: ResourceResponse) {
    if (!completion.compareAndSet(COMPLETION_OPEN, COMPLETION_RUNNING)) {
      throw InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode,
        "ResourceRequestHandle is already completed",
      )
    }
    try {
      retainLive()
    } catch (error: Throwable) {
      completion.store(COMPLETION_OPEN)
      throw error
    }
    var reachedNative = false
    try {
      val nativeStatus = memScoped {
        val nativeResponse = ResourceStructs.resourceResponse(response, this)
        reachedNative = true
        completer(handle, nativeResponse)
      }
      val nativeFailure =
        if (nativeStatus == MaplibreStatus.OK.nativeCode) null else Status.exception(nativeStatus)
      completion.store(COMPLETION_DONE)
      markClosed()
      nativeFailure?.let { throw it }
    } catch (error: Throwable) {
      if (reachedNative) {
        completion.store(COMPLETION_DONE)
        markClosed()
      } else {
        completion.store(COMPLETION_OPEN)
      }
      throw error
    } finally {
      releaseBorrow()
    }
  }

  public fun isCancelled(): Boolean = withLiveHandle {
    memScoped {
      val outCancelled = alloc<BooleanVar>()
      outCancelled.value = false
      Status.check(cancellationChecker(handle, outCancelled.ptr))
      outCancelled.value
    }
  }

  override fun close() {
    markClosed()
  }

  internal fun finishProviderDecision(decision: ResourceProviderDecision): UInt {
    if (!decisionFinalized.compareAndSet(0, 1))
      return ResourceProviderDecision.HANDLE.nativeValue.toUInt()
    return if (
      completion.load() != COMPLETION_OPEN || decision == ResourceProviderDecision.HANDLE
    ) {
      nativeReference.markProviderOwned()
      tryReleaseNative()
      ResourceProviderDecision.HANDLE.nativeValue.toUInt()
    } else {
      nativeReference.markNativeWillRelease()
      markClosed()
      ResourceProviderDecision.PASS_THROUGH.nativeValue.toUInt()
    }
  }

  internal fun finishProviderException(): UInt {
    if (!decisionFinalized.compareAndSet(0, 1))
      return ResourceProviderDecision.HANDLE.nativeValue.toUInt()
    return if (completion.load() != COMPLETION_OPEN) {
      nativeReference.markProviderOwned()
      tryReleaseNative()
      ResourceProviderDecision.HANDLE.nativeValue.toUInt()
    } else {
      nativeReference.markNativeWillRelease()
      markClosed()
      UInt.MAX_VALUE
    }
  }

  private inline fun <T> withLiveHandle(block: () -> T): T {
    retainLive()
    try {
      return block()
    } finally {
      releaseBorrow()
    }
  }

  private fun retainLive() {
    while (true) {
      val current = state.load()
      if (current and CLOSED_FLAG != 0) throw Status.released("ResourceRequestHandle")
      val active = current and ACTIVE_MASK
      check(active < ACTIVE_MASK) { "too many active ResourceRequestHandle operations" }
      if (state.compareAndSet(current, current + 1)) return
    }
  }

  private fun releaseBorrow() {
    while (true) {
      val current = state.load()
      val active = current and ACTIVE_MASK
      check(active > 0) { "ResourceRequestHandle operation count underflow" }
      val next = current - 1
      if (state.compareAndSet(current, next)) {
        if (next and CLOSED_FLAG != 0 && next and ACTIVE_MASK == 0) tryReleaseNative()
        return
      }
    }
  }

  private fun markClosed() {
    while (true) {
      val current = state.load()
      if (current and CLOSED_FLAG != 0) {
        tryReleaseNative()
        return
      }
      val next = current or CLOSED_FLAG
      if (state.compareAndSet(current, next)) {
        if (next and ACTIVE_MASK == 0) tryReleaseNative()
        return
      }
    }
  }

  private fun tryReleaseNative() {
    val current = state.load()
    if (current and CLOSED_FLAG != 0 && current and ACTIVE_MASK == 0) {
      nativeReference.releaseIfOwned()
    }
  }

  private companion object {
    private const val CLOSED_FLAG = Int.MIN_VALUE
    private const val ACTIVE_MASK = Int.MAX_VALUE
    private const val COMPLETION_OPEN = 0
    private const val COMPLETION_RUNNING = 1
    private const val COMPLETION_DONE = 2
  }

  private class NativeReference(
    private val handle: CPointer<mln_resource_request_handle>,
    private val releaser: (CPointer<mln_resource_request_handle>) -> Unit,
  ) {
    private val state = AtomicInt(STATE_PENDING)

    fun markProviderOwned() {
      while (true) {
        when (state.load()) {
          STATE_PENDING -> if (state.compareAndSet(STATE_PENDING, STATE_PROVIDER_OWNED)) return
          else -> return
        }
      }
    }

    fun markNativeWillRelease() {
      while (true) {
        val current = state.load()
        if (current == STATE_RELEASE_ACCOUNTED) return
        if (state.compareAndSet(current, STATE_RELEASE_ACCOUNTED)) return
      }
    }

    fun releaseIfOwned() {
      if (state.compareAndSet(STATE_PROVIDER_OWNED, STATE_RELEASE_ACCOUNTED)) {
        releaser(handle)
      }
    }

    fun run() {
      releaseIfOwned()
    }

    private companion object {
      private const val STATE_PENDING = 0
      private const val STATE_PROVIDER_OWNED = 1
      private const val STATE_RELEASE_ACCOUNTED = 2
    }
  }
}
