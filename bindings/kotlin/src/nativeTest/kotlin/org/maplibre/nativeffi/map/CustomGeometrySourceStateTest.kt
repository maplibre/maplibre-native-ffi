package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.cValue
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.internal.c.mln_canonical_tile_id
import org.maplibre.nativeffi.style.CustomGeometrySourceCallback
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions

@OptIn(ExperimentalForeignApi::class)
class CustomGeometrySourceStateTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun callbacksCopyTileIdsContainHostFailuresAndStopAfterClose() {
    val received = mutableListOf<CanonicalTileId>()
    val state =
      CustomGeometrySourceState(
        CustomGeometrySourceOptions(
          object : CustomGeometrySourceCallback {
            override fun fetchTile(tileId: CanonicalTileId) {
              received += tileId
              throw IllegalStateException("contained")
            }

            override fun cancelTile(tileId: CanonicalTileId) {
              received += tileId
            }
          }
        )
      )
    val tile =
      cValue<mln_canonical_tile_id> {
        z = 1U
        x = 2U
        y = 3U
      }

    state.fetch(tile)
    state.cancel(tile)
    state.close()
    state.fetch(tile)

    assertEquals(listOf(CanonicalTileId(1, 2, 3), CanonicalTileId(1, 2, 3)), received)
  }

  @Test
  fun closeDuringInFlightCallbackCompletesAfterCallbackAndSuppressesLaterUpcalls() {
    val received = mutableListOf<CanonicalTileId>()
    lateinit var state: CustomGeometrySourceState
    state =
      CustomGeometrySourceState(
        CustomGeometrySourceOptions(
          object : CustomGeometrySourceCallback {
            override fun fetchTile(tileId: CanonicalTileId) {
              received += tileId
              state.close()
            }

            override fun cancelTile(tileId: CanonicalTileId) {
              received += tileId
            }
          }
        )
      )
    val tile =
      cValue<mln_canonical_tile_id> {
        z = 4U
        x = 5U
        y = 6U
      }

    state.fetch(tile)
    state.cancel(tile)
    state.close()

    assertEquals(listOf(CanonicalTileId(4, 5, 6)), received)
  }
}
