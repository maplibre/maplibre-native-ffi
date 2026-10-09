package org.maplibre.nativeffi.callback

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.generated.CustomGeometrySourceOptions
import org.maplibre.nativeffi.generated.CustomMvtVectorSourceOptions
import org.maplibre.nativeffi.generated.MapMode
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

  @Test
  fun elevenLiveCustomSourcesStayRegistered(): Unit = runSuspendTest {
    // Each live source roots its own callbacks, so eleven sources of each kind run side by side.
    withMap(mapMode = MapMode.STATIC) {
      loadStyle()
      val geometryOptions = CustomGeometrySourceOptions(fetchTile = {})
      val mvtOptions = CustomMvtVectorSourceOptions(fetchTile = {})
      val geometryIds = (1..11).map { "custom-$it" }
      val mvtIds = (1..11).map { "custom-mvt-$it" }
      geometryIds.forEach { map.addCustomGeometrySource(it, geometryOptions).awaitCommitted() }
      mvtIds.forEach { map.addCustomMvtVectorSource(it, mvtOptions).awaitCommitted() }
      (geometryIds + mvtIds).forEach { id ->
        assertNotNull(map.getStyleSourceInfo(id).awaitWithin("source $id"), id)
      }

      // Removing a source leaves the others registered and makes room for the next one.
      map.removeStyleSource("custom-1").awaitCommitted()
      map.removeStyleSource("custom-mvt-1").awaitCommitted()
      map.addCustomGeometrySource("custom-12", geometryOptions).awaitCommitted()
      map.addCustomMvtVectorSource("custom-mvt-12", mvtOptions).awaitCommitted()
      assertNotNull(map.getStyleSourceInfo("custom-12").awaitWithin("source custom-12"))
      assertNotNull(map.getStyleSourceInfo("custom-mvt-12").awaitWithin("source custom-mvt-12"))
      assertNull(map.getStyleSourceInfo("custom-1").awaitWithin("source custom-1"))
      assertNull(map.getStyleSourceInfo("custom-mvt-1").awaitWithin("source custom-mvt-1"))
    }
  }
}
