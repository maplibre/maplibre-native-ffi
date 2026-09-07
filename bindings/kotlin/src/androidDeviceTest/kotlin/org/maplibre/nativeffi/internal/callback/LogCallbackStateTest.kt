package org.maplibre.nativeffi.internal.callback

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogRecord
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.runtime.runSuspendTest
import org.maplibre.nativeffi.runtime.use

class LogCallbackStateTest {
  @Test
  fun callbackContainsFailuresAndStopsAfterClose() {
    val state = LogCallbackState(LogCallback { true })
    assertEquals(1, state.invoke(1, 0, 0, null))
    state.close()
    assertEquals(0, state.invoke(1, 0, 0, null))
    assertTrue(state.isClosedForTesting())

    val throwing = LogCallbackState(LogCallback { error("contained") })
    assertEquals(0, throwing.invoke(1, 0, 0, null))
    throwing.close()
  }

  @Test
  fun callbackKeepsRawSeverityAndEventValues() {
    var copied: LogRecord? = null
    val state =
      LogCallbackState(
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
  fun elevenRegistrationsKeepTheSharedThunkCallable(): Unit = runSuspendTest {
    var dispatched = -1
    try {
      repeat(11) { index ->
        Maplibre.setLogCallback(
          LogCallback {
            dispatched = index
            true
          }
        )
      }
      // JavaCPP shares one upcall thunk across registrations, so the eleventh must still
      // dispatch to the callback it installed.
      RuntimeHandle.create(RuntimeOptions()).use { runtime ->
        MapHandle.create(
            runtime,
            MapOptions().apply {
              width = 64
              height = 64
            },
          )
          .await()
          .use { map ->
            map
              .setStyleJson(
                """{"version":8,"center":false,"sources":{},"layers":[]}""".encodeToByteArray()
              )
              .await()
            runtime.barrier().await()
          }
      }
      assertEquals(10, dispatched)
    } finally {
      Maplibre.clearLogCallback()
    }
  }
}
