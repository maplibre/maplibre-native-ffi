package org.maplibre.nativeffi.error

import kotlin.reflect.KClass
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.internal.status.Status

/** How a failed status becomes a Kotlin exception. */
class StatusMappingTest {
  @Test
  fun aStatusMapsToItsExceptionAndAnUnknownCodeSurvives() {
    val table: List<Pair<Int, KClass<out MaplibreException>>> =
      listOf(
        -1 to InvalidArgumentException::class,
        -6 to MaplibreException::class,
        -127 to MaplibreException::class,
      )
    for ((code, type) in table) {
      val failure =
        assertFailsWith<MaplibreException>("status $code") {
          Status.check(code) { "diagnostic $code" }
        }
      assertTrue(type.isInstance(failure), "status $code maps to $type, got ${failure::class}")
      assertEquals(MaplibreStatus(code), failure.status)
      assertEquals(code, failure.nativeStatusCode)
      assertEquals("diagnostic $code", failure.diagnostic)
    }
    assertEquals(
      "INVALID_ARGUMENT (-1): invalid pointer",
      InvalidArgumentException(-1, "invalid pointer").message,
    )
    // An error the binding raises has no native status to show.
    assertEquals("INVALID_STATE: MapHandle is closed", Status.closed("MapHandle").message)
    assertEquals(
      "MaplibreStatus(-127) (-127): from a newer library",
      assertFailsWith<MaplibreException> { Status.check(-127) { "from a newer library" } }.message,
    )
  }
}
