package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions

class CustomGeometrySourceStateTest {
  @Test
  fun callbacksCopyTileIdsContainFailuresAndStopAfterClosureDuringCallback(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val received = mutableListOf<CanonicalTileId>()
      lateinit var state: CustomGeometrySourceState
      state =
        CustomGeometrySourceState(
          CustomGeometrySourceOptions(
            object : CustomGeometrySourceCallback {
              override fun fetchTile(tileId: CanonicalTileId) {
                received += tileId
                state.close()
                throw IllegalStateException("contained")
              }
            }
          )
        ) {}

      state.fetchTileForTesting(CanonicalTileId(4, 5, 6))
      state.fetchTileForTesting(CanonicalTileId(7, 8, 9))

      assertEquals(listOf(CanonicalTileId(4, 5, 6)), received)
      assertTrue(state.isClosedForTesting())
    }
}
