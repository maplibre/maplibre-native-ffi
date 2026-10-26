package org.maplibre.nativeffi.error

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.MaplibreException
import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.internal.status.checkStatus

/** How a failed status becomes a Kotlin exception. */
class StatusMappingTest {
  @Test
  fun aStatusMapsToItsExceptionAndAnUnknownCodeSurvives() {
    for (code in listOf(-1, -6, -127)) {
      val failure =
        assertFailsWith<MaplibreException>("status $code") {
          checkStatus(code) { "diagnostic $code" }
        }
      assertEquals(Status(code), failure.status)
      assertEquals("diagnostic $code", failure.diagnostic)
    }
    assertEquals(
      "INVALID_ARGUMENT: invalid pointer",
      MaplibreException(Status.INVALID_ARGUMENT, "invalid pointer").message,
    )
    assertEquals(
      "Status(-127): from a newer library",
      assertFailsWith<MaplibreException> { checkStatus(-127) { "from a newer library" } }.message,
    )
  }
}
