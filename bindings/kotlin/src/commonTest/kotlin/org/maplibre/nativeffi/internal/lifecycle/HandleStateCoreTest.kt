package org.maplibre.nativeffi.internal.lifecycle

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.NativeErrorException

class HandleStateCoreTest {
  @Test
  fun failedNativeDestroyLeavesHandleLiveAndRetryable() {
    val state = HandleStateCore("TestHandle", 0x1234)
    var attempts = 0

    val failure =
      assertFailsWith<NativeErrorException> {
        state.closeOnce(
          destroy = {
            attempts += 1
            MaplibreStatus.NATIVE_ERROR.nativeCode
          }
        )
      }

    assertEquals(MaplibreStatus.NATIVE_ERROR, failure.status)
    assertEquals(1, attempts)
    assertFalse(state.isReleased())
    state.requireLive()

    state.closeOnce(
      destroy = {
        attempts += 1
        MaplibreStatus.OK.nativeCode
      }
    )

    assertEquals(2, attempts)
    assertTrue(state.isReleased())

    state.closeOnce(
      destroy = {
        attempts += 1
        error("destroy must not be called after release")
      }
    )

    assertEquals(2, attempts)
  }

  @Test
  fun releasingHandleRejectsPublicAccessAndReentrantRelease() {
    val state = HandleStateCore("TestHandle", 0x1234)
    var attempts = 0

    state.closeOnce(
      destroy = {
        attempts += 1
        val accessError = assertFailsWith<InvalidStateException> { state.requireLive() }
        assertEquals(MaplibreStatus.INVALID_STATE, accessError.status)
        assertEquals("TestHandle is currently releasing", accessError.diagnostic)

        val closeError =
          assertFailsWith<InvalidStateException> {
            state.closeOnce(
              destroy = {
                attempts += 1
                MaplibreStatus.OK.nativeCode
              }
            )
          }
        assertEquals(MaplibreStatus.INVALID_STATE, closeError.status)
        assertEquals("TestHandle is currently releasing", closeError.diagnostic)
        MaplibreStatus.OK.nativeCode
      }
    )

    assertEquals(1, attempts)
    assertTrue(state.isReleased())
  }

  @Test
  fun liveChildrenBlockParentCloseUntilReleased() {
    val state = HandleStateCore("ParentHandle", 0x1234)
    val child = state.retainChild()
    var attempts = 0

    val error =
      assertFailsWith<InvalidStateException> {
        state.closeOnce(
          destroy = {
            attempts += 1
            MaplibreStatus.OK.nativeCode
          }
        )
      }

    assertEquals(MaplibreStatus.INVALID_STATE, error.status)
    assertEquals("ParentHandle has 1 live child handle(s)", error.diagnostic)
    assertEquals(0, attempts)
    assertFalse(state.isReleased())

    child.close()
    child.close()

    state.closeOnce(
      destroy = {
        attempts += 1
        MaplibreStatus.OK.nativeCode
      }
    )

    assertEquals(1, attempts)
    assertTrue(state.isReleased())
  }

  @Test
  fun leakReportReportsOnlyUnreleasedHandles() {
    val reports = mutableListOf<String>()
    val unreleased = HandleStateCore.LeakReport("RuntimeHandle", 0x1234L, reports::add)

    unreleased.report()

    assertEquals(
      listOf(
        "Leaked RuntimeHandle native handle 0x1234; " +
          "close handles explicitly on their owner thread."
      ),
      reports,
    )

    val released = HandleStateCore.LeakReport("MapHandle", 0x5678L, reports::add)
    released.markReleased()
    released.report()

    assertEquals(1, reports.size)
  }
}
