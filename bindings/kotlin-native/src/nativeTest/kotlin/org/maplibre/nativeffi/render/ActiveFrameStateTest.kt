package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus

class ActiveFrameStateTest {
  // BND-170: session operations and nested acquisition fail while a frame is active.

  @Test
  fun activeFrameRejectsForbiddenSessionOperations() {
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
  fun endingFrameBorrowAllowsLaterOperationsAndAcquisition() {
    val state = ActiveFrameState()

    state.beginAcquire()
    state.endBorrow()

    state.ensureInactive("render")
    state.beginAcquire()
  }
}
