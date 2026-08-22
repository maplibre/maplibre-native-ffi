package org.maplibre.nativeffi.internal.callback

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogRecord

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

  @Test
  fun callbackKeepsRawSeverityAndEventValues() {
    var copied: LogRecord? = null
    val state =
      LogCallbackState.createForTesting(
        LogCallback {
          copied = it
          true
        }
      )
    try {
      assertEquals(1, state.invoke(991, 992, 7, null))
    } finally {
      state.close()
    }
    assertEquals(991, copied?.severity?.nativeValue)
    assertEquals(992, copied?.event?.nativeValue)
    assertEquals(7, copied?.code)
  }

  @Test
  fun elevenRegistrationsKeepTheSharedThunkCallable() {
    try {
      repeat(11) { index -> Maplibre.setLogCallback(LogCallback { it.code == index.toLong() }) }
    } finally {
      Maplibre.clearLogCallback()
    }
  }
}
