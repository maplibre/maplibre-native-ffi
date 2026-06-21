package org.maplibre.nativeffi.error

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertIs

class MaplibreExceptionTest {
  @Test
  fun knownNativeCodesMapToStableStatusCategories() {
    assertEquals(MaplibreStatus.OK, MaplibreStatus.fromNative(0))
    assertEquals(MaplibreStatus.INVALID_ARGUMENT, MaplibreStatus.fromNative(-1))
    assertEquals(MaplibreStatus.INVALID_STATE, MaplibreStatus.fromNative(-2))
    assertEquals(MaplibreStatus.WRONG_THREAD, MaplibreStatus.fromNative(-3))
    assertEquals(MaplibreStatus.UNSUPPORTED, MaplibreStatus.fromNative(-4))
    assertEquals(MaplibreStatus.NATIVE_ERROR, MaplibreStatus.fromNative(-5))
    assertEquals(MaplibreStatus(-127), MaplibreStatus.fromNative(-127))
  }

  @Test
  fun statusFactoryCreatesStableExceptionSubtypes() {
    assertIs<InvalidArgumentException>(
      MaplibreException.forStatus(MaplibreStatus.INVALID_ARGUMENT, -1, "bad argument")
    )
    assertIs<InvalidStateException>(
      MaplibreException.forStatus(MaplibreStatus.INVALID_STATE, -2, "bad state")
    )
    assertIs<WrongThreadException>(
      MaplibreException.forStatus(MaplibreStatus.WRONG_THREAD, -3, "wrong thread")
    )
    assertIs<UnsupportedFeatureException>(
      MaplibreException.forStatus(MaplibreStatus.UNSUPPORTED, -4, "unsupported")
    )
    assertIs<NativeErrorException>(
      MaplibreException.forStatus(MaplibreStatus.NATIVE_ERROR, -5, "native error")
    )
  }

  @Test
  fun exceptionsCarryNativeCodeStatusAndDiagnostic() {
    val exception = InvalidArgumentException(-1, "invalid pointer")

    assertEquals(MaplibreStatus.INVALID_ARGUMENT, exception.status)
    assertEquals(-1, exception.nativeStatusCode)
    assertEquals("invalid pointer", exception.diagnostic)
    assertEquals("INVALID_ARGUMENT (-1): invalid pointer", exception.message)
  }
}
