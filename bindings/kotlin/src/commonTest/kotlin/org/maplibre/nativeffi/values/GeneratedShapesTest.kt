package org.maplibre.nativeffi.values

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.BoundOptions
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.EdgeInsets
import org.maplibre.nativeffi.generated.FeatureStateSelector
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.MapOptions
import org.maplibre.nativeffi.generated.PremultipliedRgba8Image
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.StyleImageOptions
import org.maplibre.nativeffi.generated.StyleLayerVisibility
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runtime.awaitCommitted
import org.maplibre.nativeffi.runtime.awaitFailed
import org.maplibre.nativeffi.withMap

/**
 * One representative of each generated value shape against the real library. The conversions are
 * common code over each platform's native memory, so every target runs this file.
 */
class GeneratedShapesTest {
  @Test
  fun terminatedStringsRejectAnEmbeddedNulAndExplicitLengthsKeepIt(): Unit = runSuspendTest {
    withMap {
      // A URL crosses as a NUL-terminated string, which cannot hold a NUL.
      assertFailsWith<InvalidArgumentException> { map.setStyleUrl("custom://a\u0000b").await() }

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
  fun presenceFieldsRoundTripSetAndAbsentValues(): Unit = runSuspendTest {
    withMap {
      val camera =
        CameraOptions(
          center = LatLng(12.0, 34.0),
          centerAltitude = 123.0,
          padding = EdgeInsets(5.0, 10.0, 15.0, 20.0),
          zoom = 4.0,
          bearing = 25.0,
          pitch = 30.0,
          roll = 7.0,
          fieldOfView = 40.0,
        )
      map.updateCamera(CameraUpdate(camera = camera)).awaitCommitted()
      val snapshot = map.cameraSnapshotGet().camera
      assertEquals(12.0, assertNotNull(snapshot.center).latitude, 1e-6)
      assertEquals(34.0, assertNotNull(snapshot.center).longitude, 1e-6)
      assertEquals(123.0, assertNotNull(snapshot.centerAltitude), 1e-6)
      assertEquals(camera.padding, snapshot.padding)
      assertEquals(4.0, assertNotNull(snapshot.zoom), 1e-6)
      assertEquals(25.0, assertNotNull(snapshot.bearing), 1e-6)
      assertEquals(30.0, assertNotNull(snapshot.pitch), 1e-6)
      assertEquals(7.0, assertNotNull(snapshot.roll), 1e-6)
      assertEquals(40.0, assertNotNull(snapshot.fieldOfView), 1e-6)
      // A field the snapshot does not report stays absent rather than reading as zero.
      assertNull(snapshot.anchor)

      // An update that sets only the zoom leaves every other field where it was.
      map.updateCamera(CameraUpdate(camera = CameraOptions(zoom = 6.0))).awaitCommitted()
      val updated = map.cameraSnapshotGet().camera
      assertEquals(6.0, assertNotNull(updated.zoom), 1e-6)
      assertEquals(25.0, assertNotNull(updated.bearing), 1e-6)

      map.setBounds(BoundOptions(unbounded = true)).awaitCommitted()
      assertTrue(map.snapshotGet().bounds.unbounded)

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
  fun aStridedImageCopiesItsRowsAndReadsBackUnpadded(): Unit = runSuspendTest {
    withMap(mapMode = MapMode.STATIC) {
      loadStyle()
      for (width in listOf(2, 128)) {
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
      }
    }
  }

  @Test
  fun openEnumsKeepUnknownValuesAnd64BitCarriersKeepTheirHighBits(): Unit = runSuspendTest {
    withMap {
      loadStyle("""{"version":8,"sources":{},"layers":[{"id":"bg","type":"background"}]}""")
      // The binding passes an unknown enum value through unchanged, and native rejects it.
      map
        .setLayerVisibility("bg", StyleLayerVisibility(900u))
        .awaitFailed(MaplibreStatus.INVALID_ARGUMENT)

      // A map's event source is its handle, whose top byte carries the handle kind.
      map.setStyleJson(EMPTY_STYLE_JSON.encodeToByteArray()).awaitCommitted()
      val event = awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
      val handle = map.binding.handle().toULong()
      assertTrue(handle shr 56 != 0uL, "a map handle carries its kind in the top byte")
      assertEquals(handle, event.source)
    }
  }

  @Test
  fun arrayInputsAreCopiedAtSubmission(): Unit = runSuspendTest {
    withMap {
      map
        .updateCamera(CameraUpdate(camera = CameraOptions(center = LatLng(10.0, 20.0), zoom = 3.0)))
        .awaitCommitted()
      val coordinates = mutableListOf(LatLng(10.0, 20.0), LatLng(12.0, 21.0))
      val pending = map.pixelsForLatLngs(coordinates)
      coordinates.clear()
      val pixels = pending.awaitWithin("the conversion")
      assertEquals(2, pixels.size)
      val roundTrip = map.latLngsForPixels(pixels).awaitWithin("the inverse conversion")
      assertEquals(10.0, roundTrip[0].latitude, 1e-8)
      assertEquals(21.0, roundTrip[1].longitude, 1e-8)

      val style = EMPTY_STYLE_JSON.encodeToByteArray()
      val loading = map.setStyleJson(style)
      style.fill(0)
      loading.awaitCommitted()
      awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
    }
  }

  // A constructor's defaults come from the header's field annotations, so the record they build
  // matches what the native default function returns.
  @Test
  fun aRecordBuiltFromItsConstructorDefaultsEqualsTheNativeDefault() {
    assertEquals(GeneratedApi.mapOptionsDefault(), MapOptions())
  }
}
