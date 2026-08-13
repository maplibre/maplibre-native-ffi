package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.coroutines.EmptyCoroutineContext
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException
import kotlinx.coroutines.CancellableContinuation
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Runnable
import kotlinx.coroutines.suspendCancellableCoroutine
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus

/** Shared receiver state; platform adapters only bridge native calls and callback lifetime. */
@OptIn(ExperimentalAtomicApi::class)
internal class NotificationDispatcherCore(
  private val drainNative: () -> List<ReadyEndpoint>,
  private val operationCompleted: (Long) -> Boolean,
  private val checkOperation: (Long) -> Unit,
) {
  private val lock = AtomicInt(0)
  private val waiters = mutableMapOf<Long, CancellableContinuation<Unit>>()
  private val completedOperations = mutableSetOf<Long>()
  private val ready = mutableListOf<ReadyEndpoint>()
  private var callback: (() -> Unit)? = null
  private var scheduled = false
  private var drainRequested = false
  private var nativeDrainActive = false
  private var closed = false
  private var terminalError: Throwable? = null

  fun setCallback(value: () -> Unit) {
    withLock { callback = value }
    schedule()
  }

  fun clearCallback() = withLock { callback = null }

  fun drainReady(): List<ReadyEndpoint> = withLock { ready.toList().also { ready.clear() } }

  suspend fun await(operation: Long) {
    val immediate = withLock {
      terminalError?.let {
        return@withLock Result.failure(it)
      }
      if (completedOperations.remove(operation)) Result.success(true) else null
    }
    if (immediate != null) {
      immediate.getOrThrow()
      checkOperation(operation)
      return
    }
    if (operationCompleted(operation)) {
      checkOperation(operation)
      return
    }

    suspendCancellableCoroutine<Unit> { continuation ->
      val registrationError = withLock {
        terminalError?.also {
          return@withLock it
        }
        check(waiters.put(operation, continuation) == null) {
          "operation $operation already has a waiter"
        }
        null
      }
      if (registrationError != null) {
        continuation.resumeWithException(registrationError)
        return@suspendCancellableCoroutine
      }
      continuation.invokeOnCancellation { removeWaiter(operation, continuation) }
      try {
        if (operationCompleted(operation)) {
          if (removeWaiter(operation, continuation)) resumeOperation(operation, continuation)
        } else {
          schedule()
        }
      } catch (error: Throwable) {
        if (removeWaiter(operation, continuation)) continuation.resumeWithException(error)
      }
    }
  }

  fun forget(operation: Long) {
    val waiter = withLock {
      completedOperations.remove(operation)
      waiters.remove(operation)
    }
    waiter?.resumeWithException(
      InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode,
        "operation was closed while awaiting completion",
      )
    )
  }

  fun schedule() {
    val dispatch = withLock {
      if (closed || terminalError != null) return@withLock false
      drainRequested = true
      if (scheduled) false
      else {
        scheduled = true
        true
      }
    }
    if (!dispatch) return
    try {
      Dispatchers.Default.dispatch(EmptyCoroutineContext, Runnable(::drain))
    } catch (error: Throwable) {
      fail(error)
    }
  }

  fun close() {
    withLock {
      callback = null
      drainRequested = false
      closed = true
    }
    while (withLock { nativeDrainActive }) {
      // Native callback clearing prevents new drains; only an entered drain can remain.
    }
  }

  private fun drain() {
    val shouldDrain = withLock {
      if (closed || terminalError != null) {
        scheduled = false
        false
      } else {
        drainRequested = false
        nativeDrainActive = true
        true
      }
    }
    if (!shouldDrain) return

    val endpoints =
      try {
        drainNative()
      } catch (error: Throwable) {
        withLock { nativeDrainActive = false }
        fail(error)
        return
      }

    val resumes = mutableListOf<Pair<Long, CancellableContinuation<Unit>>>()
    val receiverCallback = withLock {
      nativeDrainActive = false
      endpoints.forEach { endpoint ->
        if (endpoint.kind == ReadyEndpoint.Kind.OPERATION) {
          val waiter = waiters.remove(endpoint.id)
          if (waiter == null) completedOperations += endpoint.id
          else resumes += endpoint.id to waiter
        } else {
          ready += endpoint
        }
      }
      callback
    }
    val reschedule = withLock {
      scheduled = false
      drainRequested
    }
    if (reschedule) schedule()
    try {
      receiverCallback?.invoke()
    } catch (_: Throwable) {
      // Scheduling callbacks cannot report failures to native code.
    }
    resumes.forEach { (operation, continuation) -> resumeOperation(operation, continuation) }
  }

  private fun resumeOperation(operation: Long, continuation: CancellableContinuation<Unit>) {
    try {
      checkOperation(operation)
      continuation.resume(Unit)
    } catch (error: Throwable) {
      continuation.resumeWithException(error)
    }
  }

  private fun removeWaiter(operation: Long, continuation: CancellableContinuation<Unit>): Boolean =
    withLock {
      if (waiters[operation] !== continuation) false
      else {
        waiters.remove(operation)
        true
      }
    }

  private fun fail(error: Throwable) {
    val pending = withLock {
      if (terminalError != null) return@withLock emptyList()
      terminalError = error
      scheduled = false
      drainRequested = false
      waiters.values.toList().also { waiters.clear() }
    }
    pending.forEach { it.resumeWithException(error) }
  }

  private inline fun <T> withLock(block: () -> T): T {
    while (!lock.compareAndSet(0, 1)) {
      // Critical sections only update in-memory receiver bookkeeping.
    }
    try {
      return block()
    } finally {
      lock.store(0)
    }
  }
}
