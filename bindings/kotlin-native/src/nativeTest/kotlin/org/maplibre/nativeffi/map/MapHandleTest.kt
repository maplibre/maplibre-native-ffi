package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.runtime.RuntimeHandle

class MapHandleTest {
  @Test
  fun closeReleasesMapOnceKeepsRuntimeLiveAndInvalidatesWrapper() {
    val runtime = RuntimeHandle.create()
    try {
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
            scaleFactor = 1.0
          },
        )

      assertFalse(map.isClosed)
      assertEquals(runtime, map.runtime())
      map.close()

      assertTrue(map.isClosed)
      map.close()
      runtime.runOnce()
      assertFailsWith<InvalidStateException> { map.setStyleJson("{}") }
    } finally {
      runtime.close()
    }
  }
}
