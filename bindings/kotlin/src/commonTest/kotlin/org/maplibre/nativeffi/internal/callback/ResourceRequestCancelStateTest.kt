package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue

@OptIn(ExperimentalAtomicApi::class)
class ResourceRequestCancelStateTest {
  // BND-198.

  @Test
  fun dispatchContainsHostFailuresAndRunsTheLatestCallback() {
    val registration = ResourceRequestCancelRegistration()
    val state = ResourceRequestCancelState(registration)
    val token = state.token()
    val replacementCalls = AtomicInt(0)

    state.store { throw IllegalStateException("host cancel callback failed") }
    ResourceRequestCancelRegistry.dispatch(token)

    state.store { replacementCalls.addAndFetch(1) }
    ResourceRequestCancelRegistry.dispatch(token)
    assertEquals(1, replacementCalls.load())

    state.store(null)
    ResourceRequestCancelRegistry.dispatch(token)
    assertEquals(1, replacementCalls.load())

    registration.dispose()
  }

  @Test
  fun disposedTokenStopsRoutingAndTokensStayStableWhileRegistered() {
    val registration = ResourceRequestCancelRegistration()
    val state = ResourceRequestCancelState(registration)
    val token = state.token()
    val calls = AtomicInt(0)
    state.store { calls.addAndFetch(1) }

    assertEquals(token, state.token())
    assertTrue(ResourceRequestCancelRegistry.isRegisteredForTesting(token))

    registration.dispose()

    assertFalse(ResourceRequestCancelRegistry.isRegisteredForTesting(token))
    ResourceRequestCancelRegistry.dispatch(token)
    assertEquals(0, calls.load())
  }

  @Test
  fun aCallbackThatDropsItsOwnTokenRunsToCompletion() {
    val registration = ResourceRequestCancelRegistration()
    val state = ResourceRequestCancelState(registration)
    val token = state.token()
    val calls = AtomicInt(0)
    state.store {
      // A host callback that closes its request unregisters the token while dispatch runs.
      registration.dispose()
      calls.addAndFetch(1)
    }

    ResourceRequestCancelRegistry.dispatch(token)

    assertEquals(1, calls.load())
    assertFalse(ResourceRequestCancelRegistry.isRegisteredForTesting(token))
  }
}
