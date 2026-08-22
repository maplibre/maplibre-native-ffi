package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.style.StyleImageOptions

class DynamicSymbolLayerRenderTest {
  // A later symbol-layer add is the path that crashes Android emulator Goldfish
  // unless the native renderer clears retained drawables first. The walk uses
  // public binding APIs so every backend that can attach an owned-texture
  // session exercises the same update.

  @Test
  fun ownedTextureSessionRendersAfterAddingASymbolLayerAndStyleImage() {
    Maplibre.setLogCallback(LogCallback { true })
    Maplibre.setAsyncLogSeverities(emptySet())
    try {
      withOwnedTextureSession(width = 64, height = 64) { runtime, map, owned ->
        val session = owned.session
        map.jumpTo(
          CameraOptions().apply {
            center = LatLng(37.7749, -122.4194)
            zoom = 14.0
          }
        )
        map.setStyleJson(INITIAL_STYLE_JSON.encodeToByteArray())
        map.setStyleImage("bearing", solidIcon(0xFF.toByte(), 0, 0), StyleImageOptions())

        assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE))
        assertEquals(RenderResult.RENDERED, session.renderUpdate().result)

        map.setStyleImage("bearing-accuracy", solidIcon(0, 0xFF.toByte(), 0), StyleImageOptions())
        map.addStyleLayerJson(bearingAccuracyLayer(), "")
        map.setStyleImage("bearing", solidIcon(0, 0, 0xFF.toByte()), StyleImageOptions())

        // The crash lands on the first layer-update after the extra symbol
        // layer exists. A few more frames keep the new drawables in use.
        assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE))
        assertEquals(RenderResult.RENDERED, session.renderUpdate().result)
        repeat(4) { renderIfAvailable(runtime, map, session) }
        assertFalse(session.isClosed)
        assertTrue(map.styleLayerExists("user-bearing"))
        assertTrue(map.styleLayerExists("user-bearingAccuracy"))
        assertTrue(map.styleImageExists("bearing-accuracy"))
      }
    } finally {
      Maplibre.clearLogCallback()
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  private fun solidIcon(red: Byte, green: Byte, blue: Byte): PremultipliedRgba8Image {
    val pixels = ByteArray(ICON_SIZE * ICON_SIZE * 4)
    for (index in pixels.indices step 4) {
      pixels[index] = red
      pixels[index + 1] = green
      pixels[index + 2] = blue
      pixels[index + 3] = 0xFF.toByte()
    }
    return PremultipliedRgba8Image(ICON_SIZE, ICON_SIZE, ICON_SIZE * 4, pixels)
  }

  private fun bearingAccuracyLayer(): ByteArray =
    jsonBytes(
      """
      {
        "id": "user-bearingAccuracy",
        "type": "symbol",
        "source": "point",
        "layout": {
          "icon-image": "bearing-accuracy",
          "icon-allow-overlap": true,
          "icon-ignore-placement": true
        }
      }
      """
    )

  private companion object {
    private const val ICON_SIZE = 16

    /**
     * Background plus one symbol layer so the first render retains drawables. The second symbol
     * layer is added after that frame, which is the Goldfish crash path.
     */
    private const val INITIAL_STYLE_JSON =
      """
      {
        "version": 8,
        "name": "kotlin-dynamic-symbol-layer-test",
        "sources": {
          "point": {
            "type": "geojson",
            "data": {
              "type": "FeatureCollection",
              "features": [
                {
                  "type": "Feature",
                  "id": "user",
                  "geometry": {"type": "Point", "coordinates": [-122.4194, 37.7749]},
                  "properties": {}
                }
              ]
            }
          }
        },
        "layers": [
          {"id": "background", "type": "background", "paint": {"background-color": "#d8f1ff"}},
          {
            "id": "user-bearing",
            "type": "symbol",
            "source": "point",
            "layout": {
              "icon-image": "bearing",
              "icon-allow-overlap": true,
              "icon-ignore-placement": true
            }
          }
        ]
      }
      """
  }
}
