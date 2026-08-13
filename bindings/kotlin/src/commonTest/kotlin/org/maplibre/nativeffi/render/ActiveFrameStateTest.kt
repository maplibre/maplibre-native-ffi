package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus

class ActiveFrameStateTest {
  // BND-170.

  @Test
  fun activeFrameRejectsForbiddenSessionOperations(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val state = ActiveFrameState()

      state.beginAcquire()
      val error = assertFailsWith<InvalidStateException> { state.ensureInactive("render") }
      assertEquals(MaplibreStatus.INVALID_STATE, error.status)
      assertEquals(MaplibreStatus.INVALID_STATE.nativeCode, error.nativeStatusCode)
      assertEquals(
        "RenderSessionHandle cannot render while a texture frame is acquired",
        error.diagnostic,
      )
      assertFailsWith<InvalidStateException> { state.beginAcquire() }
    }

  @Test
  fun endingFrameBorrowAllowsLaterOperationsAndAcquisition(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val state = ActiveFrameState()

      state.beginAcquire()
      state.endBorrow()

      state.ensureInactive("render")
      state.beginAcquire()
    }
}
