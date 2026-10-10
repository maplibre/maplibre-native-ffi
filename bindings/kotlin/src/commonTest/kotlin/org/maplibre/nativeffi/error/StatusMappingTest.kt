package org.maplibre.nativeffi.error

import kotlin.reflect.KClass
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.generated.FeatureStateSelector
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withMap

/** How a failed status becomes a Kotlin exception. */
class StatusMappingTest {
  @Test
  fun everyStatusMapsToItsExceptionAndAnUnknownCodeSurvives() {
    val table: List<Pair<Int, KClass<out MaplibreException>>> =
      listOf(
        -1 to InvalidArgumentException::class,
        -2 to InvalidStateException::class,
        -3 to WrongThreadException::class,
        -4 to UnsupportedFeatureException::class,
        -5 to NativeErrorException::class,
        -6 to MaplibreException::class,
        -7 to MaplibreException::class,
        -8 to MaplibreException::class,
        -9 to MaplibreException::class,
        -10 to MaplibreException::class,
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

  @Test
  fun aSuccessfulCallNeverReadsItsDiagnostic() {
    Status.check(MaplibreStatus.OK.nativeCode) { error("read the diagnostic of a successful call") }
  }

  @Test
  fun eachFailedCallCarriesItsOwnDiagnostic(): Unit = runSuspendTest {
    withMap {
      val selector = FeatureStateSelector("source", featureId = "feature")
      val notAnObject =
        assertFailsWith<InvalidArgumentException> {
          map.setFeatureState(selector, "[]".encodeToByteArray())
        }
      val empty =
        assertFailsWith<InvalidArgumentException> { map.setFeatureState(selector, ByteArray(0)) }
      assertTrue(notAnObject.diagnostic.isNotEmpty())
      assertTrue(empty.diagnostic.isNotEmpty())
      // A later call reports its own failure rather than an earlier call's.
      assertNotEquals(notAnObject.diagnostic, empty.diagnostic)
    }
  }
}
