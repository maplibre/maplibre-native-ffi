package org.maplibre.nativeffi.internal.lifecycle

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertIs
import kotlin.test.assertNull
import kotlin.test.assertSame
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.async
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withTimeout
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.NativeErrorException
import org.maplibre.nativeffi.internal.status.Status

class HandleStateCoreTest {
  @Test
  fun borrowedCopyRejectsRetirementWithoutPoisoningRetry() {
    val state = HandleStateCore("Batch", 42)
    var releases = 0
    state.withLive {
      val error =
        assertFailsWith<InvalidStateException> {
          state.retire(
            call = {
              releases++
              CompletableDeferred(Unit)
            }
          )
        }
      assertEquals("Batch is in use", error.diagnostic)
      assertNull(error.nativeStatusCode)
      assertEquals(0, releases)
      assertFalse(state.isReleased())
    }
    state.retire(
      call = {
        releases++
        CompletableDeferred(Unit)
      }
    )
    assertEquals(1, releases)
    assertTrue(state.isReleased())
  }

  @Test
  fun rejectedRetirementCompletesConcurrentWaitersAndPermitsRetry(): Unit = runBlocking {
    val state = HandleStateCore("TestHandle", 0x1234)
    val first = CompletableDeferred<Unit>()
    assertSame(first, state.claimRetirement(first))
    val waiter =
      async(start = CoroutineStart.UNDISPATCHED) {
        runCatching { state.claimRetirement(CompletableDeferred()).await() }.exceptionOrNull()
      }
    val rejection = IllegalStateException("live child prevents close")

    state.rejectRetirement(first, rejection)

    val reported = withTimeout(1000) { waiter.await() }
    assertIs<IllegalStateException>(reported)
    assertEquals(rejection.message, reported.message)
    state.requireLive()
    val retry = CompletableDeferred<Unit>()
    assertSame(retry, state.claimRetirement(retry))
    state.completeClose()
    retry.complete(Unit)
    state.claimRetirement(CompletableDeferred()).await()
    assertTrue(state.isReleased())
  }

  @Test
  fun failedNativeDestroyLeavesHandleLiveAndRetryable() {
    val state = HandleStateCore("TestHandle", 0x1234)
    var attempts = 0

    val failure =
      assertFailsWith<NativeErrorException> {
        state.closeOnce(
          destroy = {
            attempts += 1
            throw Status.exception(MaplibreStatus.NATIVE_ERROR.nativeCode, "destroy failed")
          }
        )
      }

    assertEquals(MaplibreStatus.NATIVE_ERROR, failure.status)
    assertEquals(1, attempts)
    assertFalse(state.isReleased())
    state.requireLive()

    state.closeOnce(destroy = { attempts += 1 })

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
        assertEquals("TestHandle is closing", accessError.diagnostic)
        assertNull(accessError.nativeStatusCode)

        val closeError =
          assertFailsWith<InvalidStateException> { state.closeOnce(destroy = { attempts += 1 }) }
        assertEquals(MaplibreStatus.INVALID_STATE, closeError.status)
        assertEquals("TestHandle is closing", closeError.diagnostic)
      }
    )

    assertEquals(1, attempts)
    assertTrue(state.isReleased())
  }

  @Test
  fun aLeakReportDisposesAndReportsOnlyUnreleasedHandles() {
    val reports = mutableListOf<String>()
    val disposed = mutableListOf<Long>()
    val unreleased =
      HandleStateCore.LeakReport("RuntimeHandle", 0x1234L, reports::add, disposed::add)

    unreleased.report()
    unreleased.report()

    assertEquals(listOf(0x1234L), disposed)
    assertEquals(listOf("Leaked RuntimeHandle native handle 0x1234; close it explicitly."), reports)

    val released = HandleStateCore.LeakReport("MapHandle", 0x5678L, reports::add, disposed::add)
    released.markReleased()
    released.report()

    assertEquals(1, disposed.size)
    assertEquals(1, reports.size)
  }

  @Test
  fun aLeakReportNamesTheDisposalFailure() {
    val reports = mutableListOf<String>()
    HandleStateCore.LeakReport("MapHandle", 0x5678L, reports::add) {
        throw IllegalStateException("the map is closing")
      }
      .report()

    assertEquals(
      listOf(
        "Leaked MapHandle native handle 0x5678; close it explicitly. " +
          "Disposing it failed: the map is closing"
      ),
      reports,
    )
  }

  @Test
  fun aUseStartingAfterCloseBeginsIsRefused() {
    val state = HandleStateCore("TestHandle", 0x1234)
    var refusal: Throwable? = null

    // Runs while closeOnce holds the releasing state, the window a use on another thread
    // would land in.
    state.closeOnce(
      destroy = { refusal = assertFailsWith<InvalidStateException> { state.withLive {} } }
    )

    assertEquals("TestHandle is closing", (refusal as InvalidStateException).diagnostic)
    val closed = assertFailsWith<InvalidStateException> { state.withLive {} }
    assertEquals("TestHandle is closed", closed.diagnostic)
  }

  @Test
  fun adoptingTheZeroHandleIsAnInvalidArgument() {
    val error = assertFailsWith<InvalidArgumentException> { HandleStateCore("TestHandle", 0L) }
    assertEquals("TestHandle handle must not be zero", error.diagnostic)
    assertNull(error.nativeStatusCode)
  }

  @Test
  fun withLiveReturnsTheBlockResultAndPropagatesFailures() {
    val state = HandleStateCore("TestHandle", 0x1234)

    assertEquals(7, state.withLive { 7 })
    assertFailsWith<IllegalStateException> { state.withLive { error("boom") } }

    state.closeOnce(destroy = {})
    assertTrue(state.isReleased())
  }
}
