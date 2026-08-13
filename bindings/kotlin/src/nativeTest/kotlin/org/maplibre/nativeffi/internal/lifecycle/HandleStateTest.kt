package org.maplibre.nativeffi.internal.lifecycle

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
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
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import platform.posix.pthread_create
import platform.posix.pthread_join
import platform.posix.pthread_tVar
import platform.posix.usleep

@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
class HandleStateTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-040, BND-041, BND-046, BND-048.

  @Test
  fun concurrentReleaseDuringNativeDestroyRejectsSecondCloseAndDestroysOnce(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val handle = SyntheticHandles.map()
      val state = HandleState("TestHandle", handle)
      val phase = AtomicInt(0)
      val concurrentCloseError = AtomicReference<Throwable?>(null)
      var attempts = 0

      runConcurrentClose(ConcurrentHandleClose(state, phase, concurrentCloseError)) {
        state.closeOnce {
          attempts += 1
          phase.store(PHASE_RELEASING)
          waitForPhase(phase, PHASE_CONCURRENT_CLOSE_FINISHED)
          MaplibreStatus.OK.nativeCode
        }
      }

      val error = concurrentCloseError.load()
      assertTrue(error is InvalidStateException)
      assertEquals(MaplibreStatus.INVALID_STATE, error.status)
      assertEquals("TestHandle is currently releasing", error.diagnostic)
      assertEquals(1, attempts)
      assertTrue(state.isReleased())
    }

  private fun runConcurrentClose(close: ConcurrentHandleClose, block: () -> Unit) {
    memScoped {
      val selfRef = StableRef.create(close)
      val thread = alloc<pthread_tVar>()
      val status =
        pthread_create(
          thread.ptr,
          null,
          staticCFunction(::closeHandleOnNativeThread),
          selfRef.asCPointer(),
        )
      if (status != 0) {
        selfRef.dispose()
        error("pthread_create failed with status $status")
      }
      try {
        block()
      } finally {
        pthread_join(thread.ptr[0], null)
      }
    }
  }
}

@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
private class ConcurrentHandleClose(
  private val state: HandleState<NativeMap>,
  private val phase: AtomicInt,
  private val error: AtomicReference<Throwable?>,
) {
  fun run() {
    try {
      waitForPhase(phase, PHASE_RELEASING)
      state.closeOnce { error("concurrent destroy must not be called") }
    } catch (throwable: Throwable) {
      error.store(throwable)
    } finally {
      phase.store(PHASE_CONCURRENT_CLOSE_FINISHED)
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun closeHandleOnNativeThread(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<ConcurrentHandleClose>()
  try {
    selfRef.get().run()
  } finally {
    selfRef.dispose()
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

private const val PHASE_RELEASING = 1
private const val PHASE_CONCURRENT_CLOSE_FINISHED = 2
