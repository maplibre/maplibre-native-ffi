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
  fun okStatusReturnsNormally() {
    Status.check(MaplibreStatus.OK.nativeCode)
  }

  @Test
  fun nonOkStatusesThrowMappedExceptionTypes() {
    assertFailsWith<InvalidArgumentException> { Status.check(-1) }
    assertFailsWith<InvalidStateException> { Status.check(-2) }
    assertFailsWith<WrongThreadException> { Status.check(-3) }
    assertFailsWith<UnsupportedFeatureException> { Status.check(-4) }
    assertFailsWith<NativeErrorException> { Status.check(-5) }
  }

  @Test
  fun thrownExceptionCarriesNativeStatusCodeAndCopiedDiagnostic() {
    val exception = assertFailsWith<InvalidArgumentException> { Status.check(-1) }

    assertEquals(MaplibreStatus.INVALID_ARGUMENT, exception.status)
    assertEquals(-1, exception.nativeStatusCode)
    assertEquals(Status.currentDiagnostic(), exception.diagnostic)
  }
}
