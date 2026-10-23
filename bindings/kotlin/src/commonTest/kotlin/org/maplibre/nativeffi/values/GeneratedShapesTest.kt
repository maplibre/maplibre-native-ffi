package org.maplibre.nativeffi.values

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.assertFailsWithStatus
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.generated.AnimationOptions
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraTransitionEnd
import org.maplibre.nativeffi.generated.CameraTransitionHandler
import org.maplibre.nativeffi.generated.CameraTransitionOutcome
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.CameraUpdateMode
import org.maplibre.nativeffi.generated.FeatureStateSelector
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.MapOptions
import org.maplibre.nativeffi.generated.PremultipliedRgba8Image
import org.maplibre.nativeffi.generated.RenderedQueryGeometry
import org.maplibre.nativeffi.generated.RenderedQueryGeometryData
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.Status
import org.maplibre.nativeffi.generated.StyleExpressionInputs
import org.maplibre.nativeffi.generated.StyleImageOptions
import org.maplibre.nativeffi.generated.writeRenderedQueryGeometry
import org.maplibre.nativeffi.internal.memory.NativeArena
import org.maplibre.nativeffi.internal.memory.readCString
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runtime.awaitCommitted
import org.maplibre.nativeffi.withMap

/**
 * One representative of each generated value shape against the real library. The conversions are
 * common code over each platform's native memory, so every target runs this file.
 */
class GeneratedShapesTest {
  @Test
  fun callerSizedTextReturnsResultsLongerThanItsFirstStorage() {
    GeneratedApi.styleExpressionCreate(
        """["interpolate", ["linear"], ["zoom"], 0, 0, 10, 100]""".encodeToByteArray(),
      )
      .use { zoom ->
        assertEquals("50.0", zoom.evaluate(StyleExpressionInputs(zoom = 5.0)).decodeToString())
      }

    // The result outgrows the call's first storage, so the call runs again.
    val text = "\"" + "a".repeat(1000) + "\""
    GeneratedApi.styleExpressionCreate(text.encodeToByteArray()).use { long ->
      assertEquals(text, long.evaluate().decodeToString())
    }
  }

  @Test
  fun synchronousCallsFillCallerSizedOutputsInInputOrder() = runSuspendTest {
    withMap {
      val projection = GeneratedApi.mapProjectionCreate(map.getSnapshot().view)
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
        projection.close()
      }
    }
  }

  @Test
  fun submittingAnUnknownUnionVariantUsesTheBindingError() {
    NativeArena().use { arena ->
      val geometry = RenderedQueryGeometry(RenderedQueryGeometryData.Unknown(900u, byteArrayOf()))
      assertFailsWithStatus(Status.INVALID_ARGUMENT) { arena.writeRenderedQueryGeometry(geometry) }
    }
  }

  @Test
  fun hostStringsUseViewsAndNativeLentStringsDecode() = runSuspendTest {
    NativeArena().use { arena ->
      assertEquals("native-lent é", readCString(arena.cString("native-lent é")))
    }
    withMap {
      // Native rejects an embedded NUL in the URL view before a C-string consumer.
      assertFailsWithStatus(Status.INVALID_ARGUMENT) { map.setStyleUrl("custom://a\u0000b") }

      // A feature ID crosses as an explicit-length view, so "a\u0000b" and "a" are two IDs.
      val withNul = FeatureStateSelector("source", featureId = "a\u0000b")
      map.setFeatureState(withNul, """{"hover":true}""".encodeToByteArray()).awaitCommitted()
      assertEquals(
        """{"hover":true}""",
        map.getFeatureState(withNul).awaitWithin("the state").decodeToString(),
      )
      assertEquals(
        "{}",
        map
          .getFeatureState(FeatureStateSelector("source", featureId = "a"))
          .awaitWithin("the truncated ID's state")
          .decodeToString(),
      )
    }
  }

  @Test
  fun presenceFieldsRoundTripSetAndAbsentValues() = runSuspendTest {
    withMap {
      val camera = CameraOptions(center = LatLng(12.0, 34.0), zoom = 4.0, bearing = 25.0)
      map.updateCamera(CameraUpdate(camera = camera)).awaitCommitted()
      val snapshot = map.getSnapshot().view.camera
      assertEquals(12.0, assertNotNull(snapshot.center).latitude, 1e-6)
      assertEquals(34.0, assertNotNull(snapshot.center).longitude, 1e-6)
      assertEquals(4.0, assertNotNull(snapshot.zoom), 1e-6)
      // A field the snapshot does not report stays absent rather than reading as zero.
      assertNull(snapshot.anchor)

      // An update that sets only the zoom leaves every other field where it was.
      map.updateCamera(CameraUpdate(camera = CameraOptions(zoom = 6.0))).awaitCommitted()
      val updated = map.getSnapshot().view.camera
      assertEquals(6.0, assertNotNull(updated.zoom), 1e-6)
      assertEquals(25.0, assertNotNull(updated.bearing), 1e-6)

      // A selector's optional state key narrows a removal to one member.
      val selector = FeatureStateSelector("source", featureId = "feature")
      map
        .setFeatureState(selector, """{"hover":true,"radius":2}""".encodeToByteArray())
        .awaitCommitted()
      map.removeFeatureState(selector.copy(stateKey = "hover")).awaitCommitted()
      assertEquals(
        """{"radius":2}""",
        map.getFeatureState(selector).awaitWithin("the state").decodeToString(),
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
      map.setStyleImage("image", image, StyleImageOptions()).awaitCommitted()
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
  fun openEnumsKeepUnknownValuesAnd64BitCarriersKeepTheirHighBits() = runSuspendTest {
    withMap {
      // The binding passes an unknown enum value through unchanged, and native rejects it.
      assertFailsWithStatus(Status.INVALID_ARGUMENT) {
        GeneratedApi.networkSetStatus(org.maplibre.nativeffi.generated.NetworkStatus(900u))
      }

      // Cancelling the largest identity ends the ease that carries it, so the identity crossed
      // whole both times.
      val ended = CompletableDeferred<CameraTransitionEnd>()
      val animation =
        AnimationOptions(
          durationMs = 60000.0,
          transitionId = ULong.MAX_VALUE,
          endHandler = CameraTransitionHandler { ended.complete(it) },
        )
      map
        .updateCamera(
          CameraUpdate(
            mode = CameraUpdateMode.EASE,
            camera = CameraOptions(zoom = 4.0),
            animation = animation,
          ),
        )
        .awaitCommitted()
      map.cancelCameraTransition(ULong.MAX_VALUE).awaitCommitted()
      assertEquals(
        CameraTransitionOutcome.CANCELLED,
        ended.awaitWithin("the end handler").outcome,
      )
    }
  }

  @Test
  fun arrayInputsAreCopiedAtSubmission() = runSuspendTest {
    withMap {
      map
        .updateCamera(CameraUpdate(camera = CameraOptions(center = LatLng(10.0, 20.0), zoom = 3.0)))
        .awaitCommitted()
      val coordinates = mutableListOf(LatLng(10.0, 20.0), LatLng(12.0, 21.0))
      val expected = map.cameraForLatLngs(coordinates.toList(), null).awaitWithin("the camera fit")
      val pending = map.cameraForLatLngs(coordinates, null)
      coordinates.clear()
      assertEquals(expected, pending.awaitWithin("the copied camera fit"))

      val style = EMPTY_STYLE_JSON.encodeToByteArray()
      val loading = map.setStyleJson(style)
      style.fill(0)
      loading.awaitCommitted()
      awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
    }
  }

  // Constructor defaults use the native default function.
  @Test
  fun aRecordBuiltFromItsConstructorDefaultsEqualsTheNativeDefault() {
    assertEquals(GeneratedApi.mapOptionsDefault(), MapOptions())
  }
}
