package org.maplibre.nativeffi.internal.status

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.memScoped
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.NativeErrorException
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.error.WrongThreadException
import org.maplibre.nativeffi.internal.c.mln_network_status_set
import org.maplibre.nativeffi.internal.memory.MemoryUtil

@OptIn(ExperimentalForeignApi::class)
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

  @Test
  fun nativeStatusConversionCapturesDiagnosticImmediately() {
    val exception =
      assertFailsWith<InvalidArgumentException> { Status.check(mln_network_status_set(999_999U)) }

    assertEquals(MaplibreStatus.INVALID_ARGUMENT, exception.status)
    assertTrue(exception.diagnostic.contains("network status"))
  }

  @Test
  fun nullTerminatedStringsRejectEmbeddedNul() {
    memScoped { assertFailsWith<IllegalArgumentException> { MemoryUtil.cString(this, "a\u0000b") } }
  }
}
