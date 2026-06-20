package org.maplibre.nativeffi.log

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNull
import kotlin.test.assertTrue
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.cstr
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.staticCFunction
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.callback.LogCallbackState
import platform.posix.pthread_create
import platform.posix.pthread_join
import platform.posix.pthread_tVar
import platform.posix.usleep

@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
class LogCallbackStateTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-120, BND-121, BND-122, BND-123: log callback install, replacement,
  // host-language failure containment, and races.

  @Test
  fun processGlobalLogCallbackCopiesRecordsContainsHostFailuresAndClearsState() {
    val records = mutableListOf<LogRecord>()
    val replacementRecords = mutableListOf<LogRecord>()
    var initialState: LogCallbackState? = null
    var replacementState: LogCallbackState? = null
    try {
      Maplibre.setLogCallback(
        LogCallback { record ->
          records += record
          true
        }
      )
      initialState = LogCallbackState.currentForTesting()
      memScoped {
        assertEquals(1U, initialState?.invoke(1U, 3U, 7L, "hello".cstr.getPointer(this)))
      }
      assertEquals(LogSeverity.INFO, records.single().severity)
      assertEquals(LogSeverity.INFO.nativeValue, records.single().severity.nativeValue)
      assertEquals(LogEvent.PARSE_STYLE, records.single().event)
      assertEquals(LogEvent.PARSE_STYLE.nativeValue, records.single().event.nativeValue)
      assertEquals(7L, records.single().code)
      assertEquals("hello", records.single().message)

      Maplibre.setLogCallback(
        LogCallback { record ->
          replacementRecords += record
          false
        }
      )
      replacementState = LogCallbackState.currentForTesting()
      assertEquals(0U, initialState?.invoke(1U, 3U, 8L, null))
      memScoped {
        assertEquals(0U, replacementState?.invoke(2U, 6U, 9L, "replacement".cstr.getPointer(this)))
      }
      assertEquals(1, records.size)
      assertEquals("replacement", replacementRecords.single().message)

      Maplibre.setLogCallback(LogCallback { throw IllegalStateException("contained") })
      assertEquals(0U, replacementState?.invoke(2U, 6U, 10L, null))
      assertEquals(0U, LogCallbackState.currentForTesting()?.invoke(2U, 6U, 0L, null))
    } finally {
      Maplibre.clearLogCallback()
      assertNull(LogCallbackState.currentForTesting())
      assertEquals(0U, LogCallbackState.invokeCurrent(1U, 3U, 12L, null))
      assertEquals(0U, replacementState?.invoke(2U, 6U, 11L, null))
    }
  }

  @Test
  fun logCallbackCopiesMessageAndPreservesUnknownRawEnums() {
    var record: LogRecord? = null
    try {
      Maplibre.setLogCallback(
        LogCallback {
          record = it
          true
        }
      )
      val state = requireNotNull(LogCallbackState.currentForTesting())

      memScoped { assertEquals(1U, state.invoke(900U, 901U, 12L, "future".cstr.getPointer(this))) }

      assertEquals(LogSeverity(900), record?.severity)
      assertEquals(900, record?.severity?.nativeValue)
      assertEquals(LogEvent(901), record?.event)
      assertEquals(901, record?.event?.nativeValue)
      assertEquals(12L, record?.code)
      assertEquals("future", record?.message)
    } finally {
      Maplibre.clearLogCallback()
    }
  }

  @Test
  fun failedInitialNativeLogInstallClosesReplacementState() {
    var replacementState: LogCallbackState? = null

    val error =
      assertFailsWith<MaplibreException> {
        LogCallbackState.setForTesting(
          LogCallback { true },
          install = { MaplibreStatus.NATIVE_ERROR.nativeCode },
          captureReplacement = { replacementState = it },
        )
      }

    assertEquals(MaplibreStatus.NATIVE_ERROR, error.status)
    assertNull(LogCallbackState.currentForTesting())
    assertTrue(replacementState?.isClosedForTesting() == true)
  }

  @Test
  fun closeDuringLogCallbackAllowsEnteredCallbackAndSuppressesLaterUpcalls() {
    var accepted = 0
    lateinit var state: LogCallbackState
    try {
      Maplibre.setLogCallback(
        LogCallback {
          state.close()
          accepted += 1
          true
        }
      )
      state = requireNotNull(LogCallbackState.currentForTesting())

      assertEquals(1U, state.invoke(1U, 3U, 7L, null))
      assertEquals(1, accepted)
      assertEquals(0U, state.invoke(1U, 3U, 8L, null))
    } finally {
      Maplibre.clearLogCallback()
    }
  }

  @Test
  fun clearDuringLogCallbackRejectsBeforeNativeClear() {
    var clearError: Throwable? = null
    lateinit var state: LogCallbackState
    try {
      Maplibre.setLogCallback(
        LogCallback {
          clearError = assertFailsWith<InvalidStateException> { Maplibre.clearLogCallback() }
          true
        }
      )
      state = requireNotNull(LogCallbackState.currentForTesting())

      assertEquals(1U, state.invoke(1U, 3U, 7L, null))
      assertTrue(clearError is InvalidStateException)
      assertTrue(LogCallbackState.currentForTesting() === state)
    } finally {
      Maplibre.clearLogCallback()
    }
  }

  @Test
  fun setDuringLogCallbackRejectsBeforeNativeInstall() {
    var setError: Throwable? = null
    lateinit var state: LogCallbackState
    try {
      Maplibre.setLogCallback(
        LogCallback {
          setError =
            assertFailsWith<InvalidStateException> { Maplibre.setLogCallback(LogCallback { true }) }
          true
        }
      )
      state = requireNotNull(LogCallbackState.currentForTesting())

      assertEquals(1U, state.invoke(1U, 3U, 7L, null))
      assertTrue(setError is InvalidStateException)
      assertTrue(LogCallbackState.currentForTesting() === state)
    } finally {
      Maplibre.clearLogCallback()
    }
  }

  @Test
  fun concurrentCloseDuringLogCallbackAllowsEnteredCallbackAndSuppressesLaterUpcalls() {
    val phase = AtomicInt(LOG_PHASE_READY)
    val closeStarted = AtomicInt(0)
    val closeReturned = AtomicInt(0)
    val error = AtomicReference<Throwable?>(null)
    val accepted = AtomicInt(0)
    lateinit var state: LogCallbackState
    try {
      Maplibre.setLogCallback(
        LogCallback {
          accepted.addAndFetch(1)
          phase.store(LOG_PHASE_ENTERED)
          waitForLogPhase(phase, LOG_PHASE_RELEASE)
          true
        }
      )
      state = requireNotNull(LogCallbackState.currentForTesting())

      runLogInvokeOnNativeThread(ConcurrentLogInvoke(state, phase, error)) {
        waitForLogPhase(phase, LOG_PHASE_ENTERED)
        runNativeAction(
          NativeAction(error) {
            closeStarted.store(1)
            state.close()
            closeReturned.store(1)
          }
        ) {
          waitForAtomic(closeStarted, 1)
          usleep(50_000U)
          assertEquals(0, closeReturned.load())
          assertEquals(0U, state.invoke(1U, 3U, 8L, null))
          phase.store(LOG_PHASE_RELEASE)
        }
      }

      error.load()?.let { throw it }
      assertEquals(1, accepted.load())
      assertEquals(1, closeReturned.load())
      assertTrue(state.isClosedForTesting())
    } finally {
      phase.store(LOG_PHASE_RELEASE)
      Maplibre.clearLogCallback()
    }
  }

  @Test
  fun logSeverityMasksRejectUnknownInputs() {
    assertEquals(1 shl 1, LogSeverity.INFO.nativeMask)
    kotlin.test.assertFailsWith<InvalidArgumentException> { LogSeverity(900).nativeMask }
  }

  private fun runLogInvokeOnNativeThread(invocation: ConcurrentLogInvoke, block: () -> Unit) {
    runNativeAction(invocation, block)
  }

  private fun runNativeAction(action: NativeRunnable, block: () -> Unit) {
    memScoped {
      val selfRef = StableRef.create(action)
      val thread = alloc<pthread_tVar>()
      val status =
        pthread_create(thread.ptr, null, staticCFunction(::runNativeRunnable), selfRef.asCPointer())
      if (status != 0) {
        selfRef.dispose()
        error("pthread_create failed with status $status")
      }
      try {
        block()
      } finally {
        action.release()
        pthread_join(thread.ptr[0], null)
      }
    }
  }
}

@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
private class ConcurrentLogInvoke(
  private val state: LogCallbackState,
  private val phase: AtomicInt,
  private val error: AtomicReference<Throwable?>,
) : NativeRunnable {
  override fun run() {
    try {
      val result = state.invoke(1U, 3U, 7L, null)
      if (result != 1U) error.store(AssertionError("expected entered log callback to return 1"))
    } catch (throwable: Throwable) {
      error.store(throwable)
    } finally {
      phase.store(LOG_PHASE_FINISHED)
    }
  }

  override fun release() {
    phase.store(LOG_PHASE_RELEASE)
  }
}

@OptIn(ExperimentalAtomicApi::class)
private class NativeAction(
  private val error: AtomicReference<Throwable?>,
  private val block: () -> Unit,
) : NativeRunnable {
  override fun run() {
    try {
      block()
    } catch (throwable: Throwable) {
      error.store(throwable)
    }
  }
}

private interface NativeRunnable {
  fun run()

  fun release() {}
}

@OptIn(ExperimentalForeignApi::class)
private fun runNativeRunnable(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<NativeRunnable>()
  try {
    selfRef.get().run()
  } finally {
    selfRef.dispose()
  }
  return null
}

@OptIn(ExperimentalAtomicApi::class)
private fun waitForLogPhase(phase: AtomicInt, expected: Int) {
  repeat(10_000) {
    if (phase.load() == expected) return
    usleep(1_000U)
  }
  error("timed out waiting for log callback phase $expected")
}

@OptIn(ExperimentalAtomicApi::class)
private fun waitForAtomic(value: AtomicInt, expected: Int) {
  repeat(10_000) {
    if (value.load() == expected) return
    usleep(1_000U)
  }
  error("timed out waiting for atomic value $expected")
}

private const val LOG_PHASE_READY = 0
private const val LOG_PHASE_ENTERED = 1
private const val LOG_PHASE_RELEASE = 2
private const val LOG_PHASE_FINISHED = 3
