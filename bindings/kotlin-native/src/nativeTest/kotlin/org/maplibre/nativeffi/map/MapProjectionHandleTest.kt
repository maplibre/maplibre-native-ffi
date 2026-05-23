package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.runtime.RuntimeHandle

@OptIn(ExperimentalForeignApi::class)
class MapProjectionHandleTest {
  @Test
  fun projectionOwnsStandaloneSnapshotAndClosesIndependently() {
    val runtime = RuntimeHandle.create()
    try {
      val map = MapHandle.create(runtime, MapOptions().size(64, 64).scaleFactor(1.0))
      val projection = MapProjectionHandle.create(map)

      assertFalse(projection.isClosed())
      map.close()
      projection.close()

      assertTrue(projection.isClosed())
      projection.close()
      assertFailsWith<InvalidStateException> { projection.nativeHandle() }
    } finally {
      runtime.close()
    }
  }
}
