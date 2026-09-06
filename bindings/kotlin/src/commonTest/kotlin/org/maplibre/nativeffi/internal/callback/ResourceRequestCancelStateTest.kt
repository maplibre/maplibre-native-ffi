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
class ResourceRequestCancelStateTest {
  // BND-198.

  @Test
  fun dispatchRunsTheCallbackOnceAndContainsItsFailure() {
    val registration = ResourceRequestCancelRegistration()
    val state = ResourceRequestCancelState(registration)
    val calls = AtomicInt(0)
    var token = 0L

    val alreadyCancelled =
      state.register({
        calls.addAndFetch(1)
        throw IllegalStateException("host cancel callback failed")
      }) { registered ->
        token = registered
        ResourceRequestCancelSetResult(MaplibreStatus.OK.nativeCode, false)
      }

    assertNull(alreadyCancelled)
    assertTrue(ResourceRequestCancelRegistry.isRegisteredForTesting(token))
    ResourceRequestCancelRegistry.dispatch(token)
    ResourceRequestCancelRegistry.dispatch(token)
    assertEquals(1, calls.load())
    assertFalse(ResourceRequestCancelRegistry.isRegisteredForTesting(token))
    registration.dispose()
  }

  @Test
  fun secondRegistrationFailsAndAFailedNativeCallLeavesTheSlotOpen() {
    val registration = ResourceRequestCancelRegistration()
    val state = ResourceRequestCancelState(registration)
    var rejectedToken = 0L

    assertFailsWith<InvalidStateException> {
      state.register({}) { registered ->
        rejectedToken = registered
        ResourceRequestCancelSetResult(MaplibreStatus.INVALID_STATE.nativeCode, false)
      }
    }
    assertFalse(ResourceRequestCancelRegistry.isRegisteredForTesting(rejectedToken))

    val calls = AtomicInt(0)
    var token = 0L
    state.register({ calls.addAndFetch(1) }) { registered ->
      token = registered
      ResourceRequestCancelSetResult(MaplibreStatus.OK.nativeCode, false)
    }
    val nativeCalls = AtomicInt(0)
    assertFailsWith<InvalidStateException> {
      state.register({}) {
        nativeCalls.addAndFetch(1)
        ResourceRequestCancelSetResult(MaplibreStatus.OK.nativeCode, false)
      }
    }
    assertEquals(0, nativeCalls.load())

    ResourceRequestCancelRegistry.dispatch(token)
    assertEquals(1, calls.load())
    registration.dispose()
  }

  @Test
  fun alreadyCancelledRegistrationHandsTheCallbackBackWithoutRouting() {
    val registration = ResourceRequestCancelRegistration()
    val state = ResourceRequestCancelState(registration)
    val callback = {}
    var token = 0L

    val alreadyCancelled =
      state.register(callback) { registered ->
        token = registered
        ResourceRequestCancelSetResult(MaplibreStatus.OK.nativeCode, true)
      }

    assertSame(callback, alreadyCancelled)
    assertFalse(ResourceRequestCancelRegistry.isRegisteredForTesting(token))
    assertNull(state.take())
  }

  @Test
  fun aCallbackThatDisposesItsOwnRegistrationRunsToCompletion() {
    val registration = ResourceRequestCancelRegistration()
    val state = ResourceRequestCancelState(registration)
    val calls = AtomicInt(0)
    var token = 0L
    state.register({
      // A host callback that closes its request disposes the token while dispatch runs.
      registration.dispose()
      calls.addAndFetch(1)
    }) { registered ->
      token = registered
      ResourceRequestCancelSetResult(MaplibreStatus.OK.nativeCode, false)
    }

    ResourceRequestCancelRegistry.dispatch(token)

    assertEquals(1, calls.load())
    assertFalse(ResourceRequestCancelRegistry.isRegisteredForTesting(token))
  }
}
