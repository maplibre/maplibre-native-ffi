package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNull
import kotlin.test.assertSame
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus

@OptIn(ExperimentalAtomicApi::class)
class DecisionCancelStateTest {
  @Test
  fun dispatchRunsTheCallbackOnceAndContainsItsFailure() {
    val registration = DecisionCancelRegistration()
    val state = DecisionCancelState("TestOwner", registration)
    val calls = AtomicInt(0)
    var token = 0L

    val alreadyCancelled =
      state.register({
        calls.addAndFetch(1)
        throw IllegalStateException("host cancel callback failed")
      }) { registered ->
        token = registered
        DecisionCancelSetResult(MaplibreStatus.OK.nativeCode, false)
      }

    assertNull(alreadyCancelled)
    assertTrue(DecisionCancelRegistry.isRegisteredForTesting(token))
    DecisionCancelRegistry.dispatch(token)
    DecisionCancelRegistry.dispatch(token)
    assertEquals(1, calls.load())
    assertFalse(DecisionCancelRegistry.isRegisteredForTesting(token))
    registration.dispose()
  }

  @Test
  fun secondRegistrationFailsAndAFailedNativeCallLeavesTheSlotOpen() {
    val registration = DecisionCancelRegistration()
    val state = DecisionCancelState("TestOwner", registration)
    var rejectedToken = 0L

    assertFailsWith<InvalidStateException> {
      state.register({}) { registered ->
        rejectedToken = registered
        DecisionCancelSetResult(MaplibreStatus.INVALID_STATE.nativeCode, false)
      }
    }
    assertFalse(DecisionCancelRegistry.isRegisteredForTesting(rejectedToken))

    val calls = AtomicInt(0)
    var token = 0L
    state.register({ calls.addAndFetch(1) }) { registered ->
      token = registered
      DecisionCancelSetResult(MaplibreStatus.OK.nativeCode, false)
    }
    val nativeCalls = AtomicInt(0)
    assertFailsWith<InvalidStateException> {
      state.register({}) {
        nativeCalls.addAndFetch(1)
        DecisionCancelSetResult(MaplibreStatus.OK.nativeCode, false)
      }
    }
    assertEquals(0, nativeCalls.load())

    DecisionCancelRegistry.dispatch(token)
    assertEquals(1, calls.load())
    registration.dispose()
  }

  @Test
  fun alreadyCancelledRegistrationHandsTheCallbackBackWithoutRouting() {
    val registration = DecisionCancelRegistration()
    val state = DecisionCancelState("TestOwner", registration)
    val callback = {}
    var token = 0L

    val alreadyCancelled =
      state.register(callback) { registered ->
        token = registered
        DecisionCancelSetResult(MaplibreStatus.OK.nativeCode, true)
      }

    assertSame(callback, alreadyCancelled)
    assertFalse(DecisionCancelRegistry.isRegisteredForTesting(token))
    assertNull(state.take())
  }

  @Test
  fun aCallbackThatDisposesItsOwnRegistrationRunsToCompletion() {
    val registration = DecisionCancelRegistration()
    val state = DecisionCancelState("TestOwner", registration)
    val calls = AtomicInt(0)
    var token = 0L
    state.register({
      // A host callback that closes its request disposes the token while dispatch runs.
      registration.dispose()
      calls.addAndFetch(1)
    }) { registered ->
      token = registered
      DecisionCancelSetResult(MaplibreStatus.OK.nativeCode, false)
    }

    DecisionCancelRegistry.dispatch(token)

    assertEquals(1, calls.load())
    assertFalse(DecisionCancelRegistry.isRegisteredForTesting(token))
  }
}
