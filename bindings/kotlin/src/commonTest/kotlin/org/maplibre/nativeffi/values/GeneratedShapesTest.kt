package org.maplibre.nativeffi.values

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertSame
import org.maplibre.nativeffi.CameraOptions
import org.maplibre.nativeffi.CameraUpdate
import org.maplibre.nativeffi.CameraUpdateMode
import org.maplibre.nativeffi.FeatureId
import org.maplibre.nativeffi.FeatureStateSelector
import org.maplibre.nativeffi.FrameDemandFlag
import org.maplibre.nativeffi.LatLng
import org.maplibre.nativeffi.LogSeverityFlag
import org.maplibre.nativeffi.MapMode
import org.maplibre.nativeffi.MapProjectionHandle
import org.maplibre.nativeffi.OfflineRegionDefinition
import org.maplibre.nativeffi.PremultipliedRgba8Image
import org.maplibre.nativeffi.Status
import org.maplibre.nativeffi.StyleExpressionHandle
import org.maplibre.nativeffi.StyleExpressionInputs
import org.maplibre.nativeffi.StyleImageOptions
import org.maplibre.nativeffi.UrlMatchFlag
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.memory.readCString
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.setNetworkStatus
import org.maplibre.nativeffi.withMap
import org.maplibre.nativeffi.writeOfflineRegionDefinition

/**
 * One representative of each generated value shape against the real library. The conversions are
 * common code over each platform's native memory, so every target runs this file.
 */
class GeneratedShapesTest {
  @Test
  fun callerSizedTextReturnsResultsLongerThanItsFirstStorage() {
    StyleExpressionHandle.create("""["interpolate", ["linear"], ["zoom"], 0, 0, 10, 100]""").use {
      zoom ->
      assertEquals("50.0", zoom.evaluate(StyleExpressionInputs(zoom = 5.0)))
    }

    // The result outgrows the call's first storage, so the call runs again.
    val text = "\"" + "a".repeat(1000) + "\""
    StyleExpressionHandle.create(text).use { long -> assertEquals(text, long.evaluate()) }
  }

  @Test
  fun synchronousCallsFillCallerSizedOutputsInInputOrder() = runSuspendTest {
    withMap {
      val projection = MapProjectionHandle.create(map.getSnapshot().view)
      try {
        val coordinates = listOf(LatLng(0.0, 0.0), LatLng(10.0, 20.0), LatLng(-12.0, -30.0))
        val points = projection.pixelsForLatLngs(coordinates)
        assertEquals(coordinates.map(projection::pixelForLatLng), points)
        assertEquals(points.map(projection::latLngForPixel), projection.latLngsForPixels(points))
        assertEquals(
          points.map(projection::latLngForPixelUnwrapped),
          projection.latLngsForPixelsUnwrapped(points),
        )
        assertEquals(emptyList(), projection.pixelsForLatLngs(emptyList()))
        assertEquals(emptyList(), projection.latLngsForPixels(emptyList()))
        assertEquals(emptyList(), projection.latLngsForPixelsUnwrapped(emptyList()))
      } finally {
        projection.release()
      }
    }
  }

  @Test
  fun submittingAnUnknownUnionVariantIsAnIllegalArgument() {
    NativeArena().use { arena ->
      val definition = OfflineRegionDefinition.Unknown(900u, byteArrayOf())
      assertFailsWith<IllegalArgumentException> { arena.writeOfflineRegionDefinition(definition) }
    }
  }

  @Test
  fun hostTextWithAnEmbeddedNulIsRejectedAndNativeLentStringsDecode() = runSuspendTest {
    NativeArena().use { arena ->
      assertEquals("native-lent é", readCString(arena.cString("native-lent é")))
    }
    withMap {
      // Text crosses as a view with an explicit length, and native rejects an embedded NUL rather
      // than reading the text up to it.
      assertFailsWithStatus(Status.INVALID_ARGUMENT) { map.setStyleUrl("custom://a\u0000b") }
      val withNul = FeatureStateSelector("source", featureId = FeatureId.StringValue("a\u0000b"))
      assertFailsWithStatus(Status.INVALID_ARGUMENT) {
        map.setFeatureState(withNul, """{"hover":true}""")
      }
    }
  }

  @Test
  fun presenceFieldsRoundTripSetAndAbsentValues() = runSuspendTest {
    withMap {
      val camera = CameraOptions(center = LatLng(12.0, 34.0), zoom = 4.0, bearing = 25.0)
      map.updateCamera(CameraUpdate(camera = camera)).await()
      val view = map.getSnapshot().view
      assertEquals(12.0, view.camera.center.latitude, 1e-6)
      assertEquals(34.0, view.camera.center.longitude, 1e-6)
      assertEquals(4.0, view.camera.zoom, 1e-6)

      // An update that sets only the zoom leaves every other field where it was.
      map.updateCamera(CameraUpdate(camera = CameraOptions(zoom = 6.0))).await()
      val updated = map.getSnapshot().view.camera
      assertEquals(6.0, updated.zoom, 1e-6)
      assertEquals(25.0, updated.bearing, 1e-6)

      // A selector's optional state key narrows a removal to one member.
      val selector = FeatureStateSelector("source", featureId = FeatureId.UintValue(7u))
      map.setFeatureState(selector, """{"hover":true,"radius":2}""").await()
      map.removeFeatureState(selector.copy(stateKey = "hover")).await()
      assertEquals("""{"radius":2}""", map.getFeatureState(selector).awaitWithin("the state"))
      // A numeric ID crosses in its own union member, which native reads as the same feature's
      // text.
      assertEquals(
        """{"radius":2}""",
        map
          .getFeatureState(selector.copy(featureId = FeatureId.StringValue("7")))
          .awaitWithin("the state by text"),
      )
    }
  }

  @Test
  fun aStridedImageCopiesItsRowsAndReadsBackUnpadded() = runSuspendTest {
    withMap(mapMode = MapMode.STATIC) {
      loadStyle()
      val width = 2
      val rowBytes = width * 4
      val stride = rowBytes + 8
      val pixels = ByteArray(stride * width) { 99 }
      val expected = ByteArray(rowBytes * width)
      for (y in 0 until width) {
        for (x in 0 until width) {
          val color = byteArrayOf(x.toByte(), y.toByte(), 0, 255.toByte())
          color.copyInto(pixels, y * stride + x * 4)
          color.copyInto(expected, y * rowBytes + x * 4)
        }
      }
      val image = PremultipliedRgba8Image(width.toUInt(), width.toUInt(), stride.toUInt(), pixels)
      map.setStyleImage("image", image, StyleImageOptions()).await()
      // The submission copies the rows out of the padded stride, and the readback is unpadded.
      val copied = map.getStyleImage("image").awaitWithin("the image")
      assertContentEquals(expected, copied?.pixels)
      // A record compares and hashes its byte arrays by content, not by identity.
      val equal = copied?.copy(pixels = expected.copyOf())
      assertEquals(copied, equal)
      assertEquals(copied.hashCode(), equal.hashCode())
    }
  }

  @Test
  fun anOpenEnumPassesAnUnknownValueThroughToNative() = runSuspendTest {
    // The binding passes the value through unchanged, and native rejects it.
    assertFailsWithStatus(Status.INVALID_ARGUMENT) {
      setNetworkStatus(org.maplibre.nativeffi.NetworkStatus(900u))
    }
  }

  @Test
  fun arrayInputsAreCopiedAtSubmission() = runSuspendTest {
    withMap {
      map
        .updateCamera(CameraUpdate(camera = CameraOptions(center = LatLng(10.0, 20.0), zoom = 3.0)))
        .await()
      val coordinates = mutableListOf(LatLng(10.0, 20.0), LatLng(12.0, 21.0))
      val expected = map.cameraForLatLngs(coordinates.toList(), null).awaitWithin("the camera fit")
      val pending = map.cameraForLatLngs(coordinates, null)
      coordinates.clear()
      assertEquals(expected, pending.awaitWithin("the copied camera fit"))

      loadStyle()
      val pixels = byteArrayOf(0, 255.toByte(), 0, 255.toByte())
      val marking = map.setStyleImage("marker", PremultipliedRgba8Image(1u, 1u, 4u, pixels))
      pixels.fill(0)
      marking.await()
      assertContentEquals(
        byteArrayOf(0, 255.toByte(), 0, 255.toByte()),
        map.getStyleImage("marker").awaitWithin("the marker")?.pixels,
      )
    }
  }

  @Test
  fun flagSetsNameTheirSingleBitsAndKeepUnknownBits() {
    assertEquals("NONE", FrameDemandFlag.NONE.toString())
    assertEquals(
      "FORCE | WAIT_FOR_UPDATE",
      (FrameDemandFlag.FORCE or FrameDemandFlag.WAIT_FOR_UPDATE).toString(),
    )
    assertEquals("INFO | WARNING | ERROR", LogSeverityFlag.ALL.toString())
    assertEquals("GLOB | 0x10", UrlMatchFlag(0x11u).toString())
  }

  @Test
  fun ofSharesKnownConstantsAndKeepsUnknownValues() {
    assertSame(CameraUpdateMode.FLY, CameraUpdateMode.of(CameraUpdateMode.FLY_VALUE))
    assertSame(FrameDemandFlag.FORCE, FrameDemandFlag.of(FrameDemandFlag.FORCE_VALUE))
    val unknown = CameraUpdateMode.of(99u)
    assertEquals(99u, unknown.rawValue)
    assertEquals(CameraUpdateMode(99u), unknown)
    assertEquals(CameraUpdateMode(99u).hashCode(), unknown.hashCode())
    assertEquals("CameraUpdateMode(99)", unknown.toString())
  }
}
