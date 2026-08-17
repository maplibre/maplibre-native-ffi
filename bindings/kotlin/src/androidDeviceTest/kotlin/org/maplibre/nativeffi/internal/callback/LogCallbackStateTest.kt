package org.maplibre.nativeffi.internal.callback

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.log.LogCallback

class LogCallbackStateTest {
  @Test
  fun callbackContainsFailuresAndStopsAfterClose() {
    val state = LogCallbackState.createForTesting(LogCallback { true })
    assertEquals(1, state.invoke(1, 0, 0, null))
    state.close()
    assertEquals(0, state.invoke(1, 0, 0, null))
    assertTrue(state.isClosedForTesting())

    val throwing = LogCallbackState.createForTesting(LogCallback { error("contained") })
    assertEquals(0, throwing.invoke(1, 0, 0, null))
    throwing.close()
  }

  @Test
  fun nativeRegistrationReplacesAndClearsOwnedState() {
    Maplibre.setLogCallback(LogCallback { true })
    Maplibre.setLogCallback(LogCallback { false })
    Maplibre.clearLogCallback()
    Maplibre.clearLogCallback()
  }
}
