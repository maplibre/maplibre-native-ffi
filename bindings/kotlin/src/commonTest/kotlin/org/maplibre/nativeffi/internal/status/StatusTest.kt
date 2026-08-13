package org.maplibre.nativeffi.internal.status

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.NativeErrorException
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.error.WrongThreadException

class StatusTest {
  @Test
  fun okStatusReturnsNormally(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest { Status.check(MaplibreStatus.OK.nativeCode) }

  @Test
  fun nonOkStatusesThrowMappedExceptionTypes(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      assertFailsWith<InvalidArgumentException> { Status.check(-1) }
      assertFailsWith<InvalidStateException> { Status.check(-2) }
      assertFailsWith<WrongThreadException> { Status.check(-3) }
      assertFailsWith<UnsupportedFeatureException> { Status.check(-4) }
      assertFailsWith<NativeErrorException> { Status.check(-5) }
    }

  @Test
  fun unknownNativeStatusPreservesRawStatusCode(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val exception = Status.exception(-127)

      assertEquals(MaplibreStatus(-127), exception.status)
      assertEquals(-127, exception.nativeStatusCode)
    }

  @Test
  fun bindingOwnedDiagnosticsDoNotReadNativeDiagnostic(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val released = Status.released("TestHandle")
      val invalidState = Status.invalidState("bad state")
      val liveChildren =
        Status.liveChildren("ParentHandle", listOf("RenderSessionHandle", "RenderSessionHandle"))
      val invalidArgument = Status.invalidArgument("bad argument")

      assertEquals(MaplibreStatus.INVALID_STATE, released.status)
      assertEquals("TestHandle is already closed", released.diagnostic)
      assertEquals(MaplibreStatus.INVALID_STATE, invalidState.status)
      assertEquals("bad state", invalidState.diagnostic)
      assertEquals(MaplibreStatus.INVALID_STATE, liveChildren.status)
      assertEquals(
        "ParentHandle has 2 live child handle(s): RenderSessionHandle x2",
        liveChildren.diagnostic,
      )
      assertEquals(MaplibreStatus.INVALID_ARGUMENT, invalidArgument.status)
      assertEquals("bad argument", invalidArgument.diagnostic)
    }
}
