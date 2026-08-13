package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.staticCFunction
import platform.posix.pthread_create
import platform.posix.pthread_join
import platform.posix.pthread_tVar
import platform.posix.usleep

@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
class OperationHandleLeaseNativeTest {
  @Test
  fun closeWaitsForStartedNativeUse() {
    val runtime = Any()
    val phase = AtomicInt(0)
    val retentionReleases = AtomicInt(0)
    val core =
      OperationHandleCore(runtime, 7L, OperationKind.REGION_CREATE, OperationResultKind.REGION) {
        retentionReleases.addAndFetch(1)
      }

    memScoped {
      val worker = StableRef.create(OperationUse(core, runtime, phase))
      val thread = alloc<pthread_tVar>()
      val status =
        pthread_create(thread.ptr, null, staticCFunction(::useOperation), worker.asCPointer())
      if (status != 0) {
        worker.dispose()
        error("pthread_create failed with status $status")
      }
      waitForPhase(phase, PHASE_USING)
      phase.store(PHASE_CLOSE_STARTED)
      val closer = StableRef.create(OperationClose(core, phase))
      val closeThread = alloc<pthread_tVar>()
      val closeStatus =
        pthread_create(
          closeThread.ptr,
          null,
          staticCFunction(::closeOperation),
          closer.asCPointer(),
        )
      if (closeStatus != 0) {
        closer.dispose()
        error("pthread_create failed with status $closeStatus")
      }
      usleep(10_000U)
      assertEquals(PHASE_CLOSE_STARTED, phase.load())
      phase.store(PHASE_RELEASE_USE)
      pthread_join(thread.ptr[0], null)
      pthread_join(closeThread.ptr[0], null)
    }

    assertEquals(PHASE_CLOSED, phase.load())
    assertEquals(1, retentionReleases.load())
    assertTrue(core.isClosed)
  }
}

@OptIn(ExperimentalAtomicApi::class)
private class OperationUse(
  private val core: OperationHandleCore,
  private val runtime: Any,
  private val phase: AtomicInt,
) {
  fun run() {
    core.withUse(runtime) {
      phase.store(PHASE_USING)
      waitForPhase(phase, PHASE_RELEASE_USE)
    }
  }
}

@OptIn(ExperimentalAtomicApi::class)
private class OperationClose(private val core: OperationHandleCore, private val phase: AtomicInt) {
  fun run() {
    core.beginClose()
    core.finishClose()
    phase.store(PHASE_CLOSED)
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun useOperation(raw: COpaquePointer?): COpaquePointer? {
  val ref = requireNotNull(raw).asStableRef<OperationUse>()
  try {
    ref.get().run()
  } finally {
    ref.dispose()
  }
  return null
}

@OptIn(ExperimentalForeignApi::class)
private fun closeOperation(raw: COpaquePointer?): COpaquePointer? {
  val ref = requireNotNull(raw).asStableRef<OperationClose>()
  try {
    ref.get().run()
  } finally {
    ref.dispose()
  }
  return null
}

@OptIn(ExperimentalAtomicApi::class)
private fun waitForPhase(phase: AtomicInt, expected: Int) {
  repeat(10_000) {
    if (phase.load() == expected) return
    usleep(1_000U)
  }
  error("timed out waiting for phase $expected")
}

private const val PHASE_USING = 1
private const val PHASE_CLOSE_STARTED = 2
private const val PHASE_RELEASE_USE = 3
private const val PHASE_CLOSED = 4
