package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.style.CustomMvtVectorSourceCallback
import org.maplibre.nativeffi.style.CustomMvtVectorSourceOptions

class CustomMvtVectorSourceStateTest {
  @Test
  fun callbacksCopyTileIdsContainFailuresAndStopAfterClosureDuringCallback() {
    val received = mutableListOf<CanonicalTileId>()
    lateinit var state: CustomMvtVectorSourceState
    state =
      CustomMvtVectorSourceState(
        CustomMvtVectorSourceOptions(
          object : CustomMvtVectorSourceCallback {
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

  @Test
  fun trampolinesDispatchByUserDataPastTheJavaCppPool() {
    val fetched = mutableListOf<Pair<Int, CanonicalTileId>>()
    val cancelled = mutableListOf<Pair<Int, CanonicalTileId>>()
    val states =
      List(11) { index ->
        CustomMvtVectorSourceState(
          CustomMvtVectorSourceOptions(
            object : CustomMvtVectorSourceCallback {
              override fun fetchTile(tileId: CanonicalTileId) {
                fetched += index to tileId
              }

              override fun cancelTile(tileId: CanonicalTileId) {
                cancelled += index to tileId
              }
            }
          )
        ) {}
      }
    try {
      states.forEach { state ->
        assertFalse(state.descriptor.fetch_tile().isNull)
        assertFalse(state.descriptor.cancel_tile().isNull)
      }
      MaplibreNativeC.mln_canonical_tile_id().z(4).x(5).y(6).use { tileId ->
        val first = states.first()
        val last = states.last()
        first.descriptor.fetch_tile().call(first.descriptor.user_data(), tileId)
        last.descriptor.cancel_tile().call(last.descriptor.user_data(), tileId)
      }
      assertEquals(listOf(0 to CanonicalTileId(4, 5, 6)), fetched)
      assertEquals(listOf(10 to CanonicalTileId(4, 5, 6)), cancelled)
    } finally {
      states.forEach { it.close() }
    }
  }
}
