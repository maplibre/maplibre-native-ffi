package org.maplibre.nativeffi.callback

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.generated.CustomGeometrySourceOptions
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runtime.awaitCommitted
import org.maplibre.nativeffi.withMap

/** Custom source callbacks, which the map roots while native holds them. */
class CustomSourceCallbackTest {
  @Test
  fun aSourceCallbackIsRootedUntilNativeReleasesItsSource(): Unit = runSuspendTest {
    withMap {
      loadStyle()
      val roots = map.bindingCallbacks
      val base = roots.rootCountForTesting()
      val options = CustomGeometrySourceOptions(fetchTile = {})
      map.addCustomGeometrySource("removed", options).awaitCommitted()
      map.addCustomGeometrySource("replaced", options).awaitCommitted()
      assertEquals(base + 2, roots.rootCountForTesting())

      // Removing a source and replacing the style each release what they drop, by the time the
      // next barrier completes.
      map.removeStyleSource("removed").awaitCommitted()
      runtime.barrier().awaitWithin("the barrier after the removal")
      assertEquals(base + 1, roots.rootCountForTesting())
      loadStyle()
      runtime.barrier().awaitWithin("the barrier after the style")
      assertEquals(base, roots.rootCountForTesting())

      // A registration the closed map rejects on the calling thread roots nothing.
      map.release().awaitWithin("the map release")
      assertFailsWith<InvalidStateException> {
        map.addCustomGeometrySource("rejected", options).await()
      }
      assertEquals(base, roots.rootCountForTesting())
    }
  }
}
